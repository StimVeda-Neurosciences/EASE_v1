#include "eeg.h"
#include "eeg_regs.h"

#include "esp_log.h"

////////////////////////// driver file for spi and gpio
#include "driver/spi_master.h"
#include "driver/spi_common.h"

#include "driver/gpio.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#define TAG "EEG"

#define EEG_DATA_Q_LEN (10 * 20) /// 10 * times the size of max data that can be sent via ble

static uint8_t eeg_data_q_stoarge_space[EEG_DATA_Q_LEN * EEG_DATA_SAMPLE_LEN];
static StaticQueue_t eeg_q_buffer;

static QueueHandle_t eeg_q_handle = NULL;

static bool eeg_ic_inited = 0;

///////////// tdcs spi handle
static spi_device_handle_t vspi_handle = NULL;

/// @brief get the device ID
/// @param  void
/// @return device id
static uint8_t get_DeviceID(void);

/////// send value to the reg
static void send_ads(uint8_t reg_addr, uint8_t data)
{

    uint8_t tx_buf[5] = {_SDATAC, (reg_addr + _WREG), 0x00, data, _RDATAC};
    ////////// spi transaction related data
    spi_transaction_t ads_trans = {
      .tx_buffer = tx_buf,
      .length = sizeof(tx_buf) * onebyte,
      .rx_buffer = NULL,
      .rxlength = 0,
      // .flags = SPI_TRANS_USE_TXDATA
    };
    gpio_set_level(PIN_EEG_CS, 0); ///////chip en
    spi_device_polling_transmit(vspi_handle, &ads_trans);
    gpio_set_level(PIN_EEG_CS, 1); ///////// chip dis
    delay(1);
}

//////////// get the value back from the reg
static uint8_t get_ads(uint8_t reg_addr)
{
    uint8_t tx_buf[5] = {_SDATAC, (reg_addr + _RREG), 0x00, 0x00, _RDATAC};
    uint8_t rx_buf[5] = {0};

    ////////// spi transaction related data
    spi_transaction_t ads_trans = {.tx_buffer = tx_buf,
                                   .length = sizeof(tx_buf) * onebyte,
                                   .rx_buffer = rx_buf,
                                   .rxlength = sizeof(rx_buf) * onebyte};

    gpio_set_level(PIN_EEG_CS, 0); ///////// chip enable
    spi_device_polling_transmit(vspi_handle, &ads_trans);
    gpio_set_level(PIN_EEG_CS, 1); /////////////// chip disable
    delay(1);

    return rx_buf[3];
}

static void ads_cmd(uint8_t cd)
{
    uint8_t tx_buff[1] = {cd};
    // ////////// spi transaction related data
    spi_transaction_t ads_trans = {
      .tx_buffer = tx_buff,
      .length = sizeof(tx_buff) * onebyte,
      .rx_buffer = NULL,
      .rxlength = 0,
      // .addr = cd

      // .flags = SPI_TRANS_USE_TXDATA

    };
    gpio_set_level(PIN_EEG_CS, 0); /////// chip enable
    spi_device_polling_transmit(vspi_handle, &ads_trans);
    gpio_set_level(PIN_EEG_CS, 1); /////////// chip disable
    delay(0.5);
}

static void IRAM_ATTR gpio_isr_hand(void* param)
{
    /////////// high task wokn = false so to avoid context switching
    BaseType_t high_task_awoken = pdFALSE;

    // len must be 15
    uint8_t rx_buff[15] = {0};
    uint8_t tx_buff[15] = {0};
    ////////// spi transaction related data
    spi_transaction_t ads_tr = {
      .tx_buffer = tx_buff,
      .length = sizeof(tx_buff) * onebyte,
      .rx_buffer = rx_buff,
      .rxlength = sizeof(rx_buff) * onebyte,
    };

    gpio_set_level(PIN_EEG_CS, 0); //// chip enable
    spi_device_polling_transmit(vspi_handle, &ads_tr);
    gpio_set_level(PIN_EEG_CS, 1); /////// chip disable

    xQueueSendFromISR(eeg_q_handle, &rx_buff[3], &high_task_awoken);

    ///////// if a context switch is needed then call this function
    // if (high_task_awoken == pdTRUE)
    // {
    //     portYIELD_FROM_ISR();
    // }
}

static void init_ads_pin(void)
{

    /////////////////////init the ads enable pin
    gpio_set_direction(PIN_EEG_IC_EN, GPIO_MODE_OUTPUT);
    ////////////////// reset pin of ads
    gpio_set_direction(PIN_EEG_IC_RESET, GPIO_MODE_OUTPUT);
    /////////////////// chip select pin of ads
    gpio_set_direction(PIN_EEG_CS, GPIO_MODE_OUTPUT);
    //////////////////////// data ready pin of ads

    static const gpio_config_t io_config = {
      .intr_type = GPIO_INTR_NEGEDGE,
      .pin_bit_mask = (1ULL << (PIN_EEG_DRDY_INTR)),
      .mode = GPIO_MODE_INPUT,
      .pull_up_en = 1,
      .pull_down_en = 0,

    };
    gpio_config(&io_config);

    gpio_set_direction(PIN_EEG_DRDY_INTR, GPIO_MODE_INPUT);
    //////////////// set the pin to floating
    gpio_set_pull_mode(PIN_EEG_DRDY_INTR, GPIO_PULLUP_ONLY);
    gpio_pullup_en(PIN_EEG_DRDY_INTR);

    ////////// set the pin to high
    gpio_set_level(PIN_EEG_IC_RESET, 1);
    ///// set the pin to high
    gpio_set_level(PIN_EEG_CS, 1);
    //////// set the level to 0
    gpio_set_level(PIN_EEG_IC_EN, 0);

    ///////////// init the intrupt routine
    ///////// set the intr type to falling edge
    // gpio_set_intr_type(PIN_EEG_DRDY_INTR, );
    ///////// add the isr
    gpio_isr_handler_add(PIN_EEG_DRDY_INTR, gpio_isr_hand, NULL);
    ///////////////// dont trigger the interrupt after configure the eeg ic
    gpio_intr_disable(PIN_EEG_DRDY_INTR);
}

/////////////// init the tdcs for coomunication  and configure the tdc ic
static void vspi_init(void)
{

    spi_bus_config_t bus_conf = {
      .miso_io_num = PIN_EEG_MISO,
      .mosi_io_num = PIN_EEG_MOSI,
      .sclk_io_num = PIN_EEG_SCK,
      .quadwp_io_num = -1,
      .quadhd_io_num = -1,
      .max_transfer_sz = PIN_EEG_MAX_BUFF_SIZE,
      // .data0_io_num =-1,
      // .data1_io_num =-1,
      // .data2_io_num =-1,
      // .data3_io_num =-1,
      // .data4_io_num =-1,
      // .data5_io_num =-1,
      // .data6_io_num =-1,
      // .data7_io_num =-1,

    };

    ///////spi device interface
    spi_device_interface_config_t bus_interface = {// .command_bits = ads_cmd_bits,
                                                   // .address_bits = ads_addr_bits,
                                                   .command_bits = 0,
                                                   .address_bits = 0,
                                                   // .dummy_bits       = tdc_dummy_bits,
                                                   .mode = ads_mode,
                                                   // .duty_cycle_pos   = tdc_duty_cycle_pos,
                                                   // .cs_ena_posttrans = tdc_cs_ena_osttrans,
                                                   // .cs_ena_pretrans  = tdc_ena_prettrrans,
                                                   .clock_speed_hz = ads_Sclk,
                                                   .flags = ads_flags,
                                                   // .input_delay_ns   = tdc_input_delay_ns,
                                                   .spics_io_num = ads_CS_soft,
                                                   .queue_size = ads_spi_queue_size,
                                                   .pre_cb = NULL,
                                                   .post_cb = NULL};

    assert(!spi_bus_initialize(SPI3_HOST, &bus_conf, SPI_DMA_DISABLED));
    assert(!spi_bus_add_device(SPI3_HOST, &bus_interface, &vspi_handle));
    assert(vspi_handle != NULL);
}

/// @brief initialise the eeg driver
/// @param  void
void eeg_driver_init(void)
{
    ESP_LOGW(TAG, "eeg drv init");
    //////// init the ads pins , power supply and intrrupt
    init_ads_pin();
    ////// init the vspi hardware
    vspi_init();

    // init a queue
    eeg_q_handle = xQueueCreateStatic(EEG_DATA_Q_LEN, EEG_DATA_SAMPLE_LEN, &eeg_data_q_stoarge_space, &eeg_q_buffer);
    assert(eeg_q_handle != NULL);
}

/// @brief deinitialise the eeg driver
/// @param  void
void eeg_driver_deinit(void)
{
    ESP_LOGW(TAG, "deinitng EEG driver");
    gpio_isr_handler_remove(PIN_EEG_DRDY_INTR);

    spi_bus_remove_device(vspi_handle);
    spi_bus_free(VSPI_HOST);
    vQueueDelete(eeg_q_handle);
}

/// @brief this is to verify that ads ic is present and working
/// @param  void
/// @return true /false
uint32_t eeg_verify_component(void)
{
    uint8_t ret = ESP_OK;

    /// @brief ///// enable the ads module
    gpio_set_level(PIN_EEG_IC_EN, 1);
    delay(300);
    ads_cmd(_WAKEUP);
    eeg_reset_ic(soft_reset);

    delay(200);
    // get device id
    if (get_DeviceID() != ADS_DEVICE_ID)
    {
        ret = ERR_EEG_HARDWARE_FAULT;
    }

    ///////// put the ads in power down mode
    ads_cmd(_STANDBY);
    ads_cmd(_SDATAC);

    /// @brief ///// disable the ads module
    gpio_set_level(PIN_EEG_IC_EN, 0);

    return ret;
}

/// @brief start the EEG reading  with below param
/// @param  rate
/// @param  reading_type
/// @param taskhandle
/// @return succ/failure
uint32_t eeg_start_reading(uint8_t rate, uint8_t reading_type, void** Q_handle)
{

    /// @brief ///// enable the ads module
    gpio_set_level(PIN_EEG_IC_EN, 1);
    delay(300);
    ads_cmd(_WAKEUP);
    eeg_reset_ic(soft_reset);
    delay(250);

    // get device id
    if (get_DeviceID() != ADS_DEVICE_ID)
    {
        *Q_handle = NULL;
        ///////// put the ads in power down mode
        ads_cmd(_STANDBY);
        ads_cmd(_SDATAC);

        /// @brief ///// disable the ads module
        gpio_set_level(PIN_EEG_IC_EN, 0);
        return ERR_EEG_HARDWARE_FAULT;
    }

    //////////// set the ads data rate
    if (rate == 2)
        send_ads(CONFIG1, 0xD5); // No Daisy, Osc. Clock output disabled, 250Hz
    else
        send_ads(CONFIG1, 0xD6);

    /////////////// init for normal operations
    if (reading_type == READING_EEG_ONLY)
    {
        send_ads(CONFIG2, 0XD5); // Generate test signal externally
        send_ads(CONFIG3, 0XEC); // Internal ref buff enabled,bias measurment open
        send_ads(CONFIG4, 0X00); // Continuous conversion mode, Leadoff comparator OFF

        // channel pins = 50h=gain 12, 00h= gain 1, 81h=POWER down
        send_ads(CH1SET, 0x50); // init channel with normal eletrode, 8X gain, SRB2 open
        send_ads(CH2SET, 0x50); // init channel with normal eletrode, 8X gain, SRB2 open
        send_ads(CH3SET, 0x50); // init channel with normal eletrode, 8X gain, SRB2 open
        send_ads(CH4SET, 0x50); // init channel with normal eletrode, 8X gain, SRB2 open
        // send_ads(CH5SET, 0x40); // init channel with normal eletrode, 8X gain, SRB2 open
        // send_ads(CH6SET, 0x40); // init channel with normal eletrode, 8X gain, SRB2 open
        // send_ads(CH7SET, 0x40); // init channel with normal eletrode, 8X gain, SRB2 open
        // send_ads(CH8SET, 0x40); // init channel with normal eletrode, 8X gain, SRB2 open

        send_ads(BIAS_SENSP, 0X0F); //
        send_ads(BIAS_SENSN, 0X00); //
    }
    //////////// init for impedance tracking
    else
    {
        send_ads(CONFIG2, 0XC1); // Generate test signal externally
        send_ads(CONFIG3, 0XEC); // Internal ref buff enabled,bias measurment open
        send_ads(CONFIG4, 0X02); // Continuous conversion mode, Leadoff comparator ON

        // Set current, frequency and threshold:
        // send_ads(LOFF, 0xA6); // AC lead-off detection at +-6nA, 7.8Hz, 80% threshold
        send_ads(LOFF, 0xA7); //// AC lead-off detection at +-24nA, fdr/4 , 80% threshold
        // channel pins = 50h=gain 12, 00h= gain 1, 81h=POWER down
        send_ads(CH1SET, 0x10); // init channel with normal eletrode, 1X gain, SRB2 open
        send_ads(CH2SET, 0x10); // init channel with normal eletrode, 1X gain, SRB2 open
        send_ads(CH3SET, 0x10); // init channel with normal eletrode, 1x gain, SRB2 open
        send_ads(CH4SET, 0x10); // init channel with normal eletrode, 1X gain, SRB2 open
        // send_ads(CH5SET, 0x81); // init channel with normal eletrode, 1X gain, SRB2 open
        // send_ads(CH6SET, 0x81); // init channel with normal eletrode, 1x gain, SRB2 open
        // send_ads(CH7SET, 0x00); // init channel with normal eletrode, 1X gain, SRB2 open
        // send_ads(CH8SET, 0x00); // init channel with normal eletrode, 1X gain, SRB2 open

        send_ads(BIAS_SENSP, 0X0F); // Switch ON for all chnl
        send_ads(BIAS_SENSN, 0X00); // Switch ON for all chnl

        // enable current source on one channel (not on its complimentary)--> Pg30 DATASHEET
        send_ads(LOFF_SENSP, 0X0F);
        // ads_wreg(LOFF_SENSN,0X00);
    }

    send_ads(MISC1, 0X20); // Connect SRB1 to all INxN of channel

    ads_cmd(_START); // Start ads conversion
    delay(1);
    ads_cmd(_RDATAC); // Start continuous conversion mode

    // rest the eeg q
    xQueueReset(eeg_q_handle);

    eeg_ic_inited = true;
    delay(20);
    /////////////// enable the intr
    gpio_intr_enable(PIN_EEG_DRDY_INTR);

    *Q_handle = eeg_q_handle;
    return ESP_OK;
}

/// @brief stop the ic reading and maybe powerdown the ic
/// @param  void
/// @return
void eeg_stop_reading(void)
{
    /////////////////// disable the interrupt on the pin
    gpio_intr_disable(PIN_EEG_DRDY_INTR);

    eeg_reset_ic(soft_reset);
    ads_cmd(_STOP);
    ///////// put the ads in power down mode
    ads_cmd(_STANDBY);
    ads_cmd(_SDATAC);

    /// @brief ///// disable the ads module
    gpio_set_level(PIN_EEG_IC_EN, 0);

    eeg_ic_inited = false;

    // reset the Q
    xQueueReset(eeg_q_handle);
}

/// @brief read the EEG data from the IC, must be called only after start_reading
/// @param buff
/// @param len
void eeg_read_data(uint8_t* buff, uint16_t len)
{
    // len must be 15
    // uint8_t rx_buff[15] = {0};
    // uint8_t tx_buff[15] = {0};
    // ////////// spi transaction related data
    // spi_transaction_t ads_tr =
    //     {
    //         .tx_buffer = tx_buff,
    //         .length = sizeof(tx_buff) * onebyte,
    //         .rx_buffer = rx_buff,
    //         .rxlength = sizeof(rx_buff) * onebyte,
    //     };
    if (!eeg_ic_inited)
        return;

    spi_transaction_t ads_tr = {
      .rx_buffer = buff,
      .rxlength = len * onebyte,
    };

    gpio_set_level(PIN_EEG_CS, 0); //// chip enable
    spi_device_polling_transmit(vspi_handle, &ads_tr);
    gpio_set_level(PIN_EEG_CS, 1); /////// chip disable
}

/// @brief reset the EEG ic
/// @param reset_type
void eeg_reset_ic(uint8_t reset_type)
{
    if (reset_type == soft_reset)
    {
        delay(10);
        ads_cmd(_RESET);
        delay(10);
    } else if (reset_type == hard_reset)
    {
        gpio_set_level(PIN_EEG_IC_RESET, 1);
        delay(10);
        gpio_set_level(PIN_EEG_IC_RESET, 0);
        delay(10);
    }
}

/// @brief get the device ID
/// @param  void
/// @return device id
static uint8_t get_DeviceID(void)
{
    return get_ads(ID);
}

/// @brief read all the IC register and print it
/// @param  void
void eeg_read_Allreg(void)
{

    get_DeviceID();
    printf("CONFIG1:  %x\r\n", get_ads(CONFIG1));
    printf("CONFIG2:  %x\r\n", get_ads(CONFIG2));
    printf("CONFIG3:  %x\r\n", get_ads(CONFIG3));
    printf("CONFIG4:  %x\r\n", get_ads(CONFIG4));

    // LOFF Settings
    printf("LOFF: %X \r\n", get_ads(LOFF));

    // CHANNEL
    printf("CH1SET:   %x \r\n", get_ads(CH1SET));
    printf("CH2SET:   %x \r\n", get_ads(CH2SET));
    printf("CH3SET:   %x \r\n", get_ads(CH3SET));
    printf("CH4SET:   %x \r\n", get_ads(CH4SET));
    printf("CH5SET:   %x \r\n", get_ads(CH5SET));
    printf("CH6SET:   %x \r\n", get_ads(CH6SET));
    printf("CH7SET:   %x \r\n", get_ads(CH7SET));
    printf("CH8SET:   %x \r\n", get_ads(CH8SET));

    // BIAS SENSE
    printf("BIAS_SEN   %x \r\n", get_ads(BIAS_SENSP));
    printf("BIAS_SEN   %x\r\n ", get_ads(BIAS_SENSN));

    // LEAD-OFF RELATED
    printf("LOFF_SEN   %x\r\n", get_ads(LOFF_SENSP));
    printf("LOFF_SEN   %x\r\n", get_ads(LOFF_SENSN));
    printf("LOFF_FLI   %x\r\n", get_ads(LOFF_FLIP));
    printf("LOFF_STA   %x\r\n", get_ads(LOFF_STATP));
    printf("LOFF_STATN: %x\r\n", get_ads(LOFF_STATN));

    // MISCLL
    printf("MISC1: %x\r\n", get_ads(MISC1));
}

/// @brief convert input data to float
/// @param  input data
/// @return float value
float eeg_data_to_float(uint8_t* arr)
{

    /////////// make sure that first byte is the MSB and then LSB
    int num = 0;
    if (arr[0] & 0x80) ////// if my msb is 1 that is it is negative number
    {
        ////// first clear the msb from there
        // arr[0] &= 0x7F;
        num = (0xff << 24) | (arr[0] << 16) | (arr[1] << 8) | (arr[2]);
        num = ~(num - 1);
        num = -num;
    } else //////////// if is 0 then positive number
    {
        num = (arr[0] << 16) | (arr[1] << 8) | (arr[2]);
    }
    return (float) (num * scale_factor);
}

/// @brief this function is used to plot one sample data
/// @param arr
void eeg_plot_data(uint8_t* arr)
{
    int num = 0;
    //////////// this algo is only for one sample data
    for (int i = 0; i < 4; i++)
    {

        num = (arr[i * 3] << 16) | (arr[i * 3 + 1] << 8) | (arr[i * 3 + 2]);
        num &= 0x00FFFFFF;
        if (arr[i * 3] & 0x80) ////// if my msb is 1 that is it is negative number
        {
            ////// first clear the msb from there
            // arr[0] &= 0x7F;
            num |= (0xff << 24);
            num = ~(num - 1);
            num = -num;
        }
        if (i == 3)
        {
            printf("%f", (float) (scale_factor * num));
        } else
        {
            printf("%f,", (float) (scale_factor * num));
        }
    }
    // printf("\r\n");
}

/// @brief convert input data to int
/// @param buff
void inline eeg_data_to_int(uint8_t* arr)
{

    int num = 0;
    //////////// this algo is only for one sample data
    for (int i = 0; i < 4; i++)
    {

        num = (arr[i * 3] << 16) | (arr[i * 3 + 1] << 8) | (arr[i * 3 + 2]);
        num &= 0x00FFFFFF;
        if (arr[i * 3] & 0x80) ////// if my msb is 1 that is it is negative number
        {
            ////// first clear the msb from there
            // arr[0] &= 0x7F;
            num |= (0xff << 24);
            // num = ~(num - 1);
            // num = -num;
        }
        if (i == 3)
        {
            printf("%d", num);
        } else
        {
            printf("%d,", num);
        }
    }
    printf("\r\n");
}