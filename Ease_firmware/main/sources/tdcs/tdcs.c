#include "tdcs.h"
#include "time.h"

static const uint16_t Sinex[] = {0x0000,
                                 0x0006,
                                 0x000D,
                                 0x0013,
                                 0x001A,
                                 0x0020,
                                 0x0027,
                                 0x002D,
                                 0x0033,
                                 0x003A,
                                 0x0040,
                                 0x0046,
                                 0x004D,
                                 0x0053,
                                 0x0059,
                                 0x0060,
                                 0x0066,
                                 0x006C,
                                 0x0072,
                                 0x0079,
                                 0x007F,
                                 0x0085,
                                 0x008B,
                                 0x0091,
                                 0x0097,
                                 0x009D,
                                 0x00A3,
                                 0x00A9,
                                 0x00AF,
                                 0x00B4,
                                 0x00BA,
                                 0x00C0,
                                 0x00C6,
                                 0x00CB,
                                 0x00D1,
                                 0x00D6,
                                 0x00DC,
                                 0x00E1,
                                 0x00E6,
                                 0x00EC,
                                 0x00F1,
                                 0x00F6,
                                 0x00FB,
                                 0x0100,
                                 0x0105,
                                 0x010A,
                                 0x010F,
                                 0x0114,
                                 0x0119,
                                 0x011D,
                                 0x0122,
                                 0x0126,
                                 0x012B,
                                 0x012F,
                                 0x0134,
                                 0x0138,
                                 0x013C,
                                 0x0140,
                                 0x0144,
                                 0x0148,
                                 0x014C,
                                 0x014F,
                                 0x0153,
                                 0x0157,
                                 0x015A,
                                 0x015E,
                                 0x0161,
                                 0x0164,
                                 0x0167,
                                 0x016A,
                                 0x016D,
                                 0x0170,
                                 0x0173,
                                 0x0176,
                                 0x0178,
                                 0x017B,
                                 0x017D,
                                 0x0180,
                                 // 0x0182, 0x0184,
                                 // 0x0186, 0x0188, 0x018A, 0x018B, 0x018D,
                                 // 0x018F, 0x0190, 0x0191, 0x0193, 0x0194,
                                 // 0x0195, 0x0196, 0x0197, 0x0198, 0x0198,
                                 // 0x0199, 0x0199, 0x019A, 0x019A, 0x019E,
                                 // 0x019E, 0x019E, 0x019A, 0x019A, 0x0199,
                                 // 0x0199, 0x0198, 0x0198, 0x0197, 0x0196,
                                 // 0x0195, 0x0194, 0x0193, 0x0191, 0x0190,
                                 // 0x018F, 0x018D, 0x018B, 0x018A, 0x0188,
                                 // 0x0186, 0x0184,
                                 0x0182,
                                 0x0180,
                                 0x017D,
                                 0x017B,
                                 0x0178,
                                 0x0176,
                                 0x0173,
                                 0x0170,
                                 0x016D,
                                 0x016A,
                                 0x0167,
                                 0x0164,
                                 0x0161,
                                 0x015E,
                                 0x015A,
                                 0x0157,
                                 0x0153,
                                 0x014F,
                                 0x014C,
                                 0x0148,
                                 0x0144,
                                 0x0140,
                                 0x013C,
                                 0x0138,
                                 0x0134,
                                 0x012F,
                                 0x012B,
                                 0x0126,
                                 0x0122,
                                 0x011D,
                                 0x0119,
                                 0x0114,
                                 0x010F,
                                 0x010A,
                                 0x0105,
                                 0x0100,
                                 0x00FB,
                                 0x00F6,
                                 0x00F1,
                                 0x00EC,
                                 0x00E6,
                                 0x00E1,
                                 0x00DC,
                                 0x00D6,
                                 0x00D1,
                                 0x00CB,
                                 0x00C6,
                                 0x00C0,
                                 0x00BA,
                                 0x00B4,
                                 0x00AF,
                                 0x00A9,
                                 0x00A3,
                                 0x009D,
                                 0x0097,
                                 0x0091,
                                 0x008B,
                                 0x0085,
                                 0x007F,
                                 0x0079,
                                 0x0072,
                                 0x006C,
                                 0x0066,
                                 0x0060,
                                 0x0059,
                                 0x0053,
                                 0x004D,
                                 0x0046,
                                 0x0040,
                                 0x003A,
                                 0x0033,
                                 0x002D,
                                 0x0027,
                                 0x0020,
                                 0x001A,
                                 0x0013,
                                 0x000D,
                                 0x0006};

///////////// tdcs spi handle
static spi_device_handle_t hspi_handle;

static uint64_t d = 0;

static volatile uint8_t flag = 0;

///////// these are used in inline function so we canot init this to static
////////// used to store the tacs mode
static uint8_t mode = 0;

static uint64_t time_till_waveform_run;

static uint64_t prev_milli_;

static uint16_t max_set_current;

static uint64_t sample_delay_for_ramp = 0;

/////////// not make it static as it cause error in inline function
// static uint64_t prev_milli_curr_imp = 0;
////////////////////////// value to store the set current value
static uint16_t set_curr = 0;

/////// send value to the reg
static void send_tdc(uint8_t reg_addr, uint16_t data)
{
    /////////// we are sending msb first and then lsb
    uint8_t tx_dat[] = {(data >> 8), (0xff & data)};

    ////////// spi transaction related data
    spi_transaction_t tdc_trans = {
      .addr = reg_addr,
      .tx_buffer = tx_dat,
      .length = 2 * onebyte,
      // .flags = SPI_TRANS_USE_TXDATA

    };
    ////// first transmit the data and then latch it
    spi_device_polling_transmit(hspi_handle, &tdc_trans);
    gpio_set_level(tdcs_ss_pin, 0);
    delay(0.5);
    gpio_set_level(tdcs_ss_pin, 1);
}

//////////// get the value back from the reg
static uint16_t get_tdc(uint8_t reg_addr)
{
    uint8_t recv_buff[2] = {0};

    /////// first sending data of the read reg
    uint8_t tx_buff[] = {0x00, reg_addr};
    ////////// spi transaction related data
    spi_transaction_t tdc_trans = {.addr = reg_read, .tx_buffer = tx_buff, .length = 2 * onebyte};
    spi_device_polling_transmit(hspi_handle, &tdc_trans);
    gpio_set_level(tdcs_ss_pin, 0);
    delay(1);
    gpio_set_level(tdcs_ss_pin, 1);

    //////// we are ready to recieve the data
    spi_transaction_t recv_trans = {.addr = NOP, .rx_buffer = recv_buff, .rxlength = 2 * onebyte};

    gpio_set_level(tdcs_ss_pin, 0);
    delay(1);
    spi_device_polling_transmit(hspi_handle, &recv_trans);
    gpio_set_level(tdcs_ss_pin, 1);

    return ((uint16_t) recv_buff[1] | (uint16_t) (recv_buff[0] << 8));
}

///////// tdcs alarm interrupt pin

static void IRAM_ATTR tdcs_alarm_irq_hand(void)
{
    /////////// high task wokn = false so to avoid context switching
    // BaseType_t high_task_awoken = pdFALSE;

    // /////// check the status register to find the cause of interrupt
    // uint16_t status = get_tdc(STATUS_reg);
    // //////// check anything
    // if((status>>8) & 0x04 )
    flag = 1;
    /////////// if a context switch is needed then call this function
    // if( high_task_awoken == pdTRUE)
    // {
    //     taskYIELD_FROM_ISR ();
    // }
    /////// call the api to stop anything
}

static void init_tdcs_gpio(void)
{
    //////////////init the gpio pins
    gpio_set_direction(tdcs_ss_pin, GPIO_MODE_OUTPUT);
    gpio_set_direction(tdcs_boost_en_pin, GPIO_MODE_OUTPUT);

    //////////// the d1 and d2 for tacs
    gpio_set_direction(tdcs_d1_pin, GPIO_MODE_OUTPUT);
    gpio_set_direction(tdcs_d2_pin, GPIO_MODE_OUTPUT);

    //////// turn off the gpio pins d1 and d2
    gpio_set_level(tdcs_d1_pin, 0);
    gpio_set_level(tdcs_d2_pin, 0);

    /////////// set the pin as interrupt for tdcs ic
    gpio_set_direction(tdcs_over_curr_intr_pin, GPIO_MODE_INPUT);
    ////////////// set the pin to high
    // gpio_set_pull_mode(tdcs_over_curr_intr_pin, GPIO_PULLUP_ONLY);

    //////////// set the interrupt type to falling edge
    gpio_set_intr_type(tdcs_over_curr_intr_pin, GPIO_INTR_NEGEDGE);
    ///////// disable the interrupt as it is only useful when required
    gpio_intr_disable(tdcs_over_curr_intr_pin);

    /////// add the isr handler
    gpio_isr_handler_add(tdcs_over_curr_intr_pin, tdcs_alarm_irq_hand, NULL);
}

/////////////// init the tdcs for coomunication  and configure the tdc ic
void tdcs_bus_init(void)
{
    init_tdcs_gpio();

    spi_bus_config_t bus_conf = {
      .miso_io_num = tdcs_miso_pin,
      .mosi_io_num = tdcs_mosi_pin,
      .sclk_io_num = tdcs_sclk_pin,
      .quadwp_io_num = -1,
      .quadhd_io_num = -1, //// set the pins to -1 that are not in used
      .max_transfer_sz = tdcs_max_xfr_size,
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
    spi_device_interface_config_t bus_interface = {// .command_bits     = tdc_cmd_bits,
                                                   .address_bits = tdc_addr_bits,
                                                   // .dummy_bits       = tdc_dummy_bits,
                                                   .mode = tdc_mode,
                                                   // .duty_cycle_pos   = tdc_duty_cycle_pos,
                                                   // .cs_ena_posttrans = tdc_cs_ena_posttrans,
                                                   // .cs_ena_pretrans  = tdc_ena_prettrrans,
                                                   .clock_speed_hz = tdc_Sclk,
                                                   .flags = tdc_flags,
                                                   // .input_delay_ns   = tdc_input_delay_ns,
                                                   .spics_io_num = tdc_CS_soft,
                                                   .queue_size = tdc_queue_size,
                                                   .pre_cb = NULL,
                                                   .post_cb = NULL};

    assert(!spi_bus_initialize(SPI2_HOST, &bus_conf, SPI_DMA_DISABLED));
    assert(!spi_bus_add_device(SPI2_HOST, &bus_interface, &hspi_handle));
    ////
    ////////////////// code to init the TDC IC

    gpio_set_level(tdcs_boost_en_pin, 0);
}

/// @brief  this is to verify the tdcs process
/// @param  void
/// @return succ/failure
uint8_t tdcs_verify_process(void)
{
    uint8_t ret = ESP_OK;
    set_curr = 0;
    delay(100);
    gpio_set_level(tdcs_boost_en_pin, 1);

    send_tdc(WRITE_RESET_REG, 0x0001);      // Reset registers to default
    send_tdc(NOP, 0x0000);                  // NOP operation
    send_tdc(WRITE_CONTROL_REG, 0x1006);    // o/p enable, o/p->0-20mA, current setting res enable
    send_tdc(WRITE_CONF_REG, 0x0020);       // disable HART, watchdog, error-check, calliberation
    send_tdc(WRITE_DAC_GAIN_CALIB, 0x8000); // Gain of 1, default is 0.5
    send_tdc(DAC_DATA, 0x10);               /// no matter what the last 4 bit values are always 0

    if (get_tdc(DAC_data_reg) != 0x10)
    {
        ret = err_tDCS_hardware_not_wroking;
    }

    send_tdc(WRITE_RESET_REG, 0x0001); // Reset registers to default
    send_tdc(NOP, 0x0000);             // NOP operation

    send_tdc(DAC_data_reg, 0);
    delay(50);
    gpio_set_level(tdcs_boost_en_pin, 0);
    delay(20);
    return ret;
}

void tdcs_bus_deinit(void)
{
    if (hspi_handle != NULL)
    {
        assert(!spi_bus_remove_device(hspi_handle));
        assert(!spi_bus_free(SPI2_HOST));

        hspi_handle = NULL;
    }
}

void tdcs_init(uint8_t waveform_type, uint16_t amplitude, uint32_t frequency, uint32_t time_till_run)
{
    // Switch ON boost and wait till power supply settles
    //////////// turn on the tdcs ic
    gpio_set_level(tdcs_boost_en_pin, 1);
    set_curr = 0;
    delay(100);
    mode = ramp_up;

    //////////// check if the device id is correct . if not send errorcode and change status back to idle

    send_tdc(WRITE_RESET_REG, 0x0001);      // Reset registers to default
    send_tdc(NOP, 0x0000);                  // NOP operation
    send_tdc(WRITE_CONTROL_REG, 0x1006);    // o/p enable, o/p->0-20mA, current setting res enable
    send_tdc(WRITE_CONF_REG, 0x0020);       // disable HART, watchdog, error-check, calliberation
    send_tdc(WRITE_DAC_GAIN_CALIB, 0x8000); // Gain of 1, default is 0.5
    send_tdc(DAC_DATA, 0x0000);

    //////// turn on the gpio pins d1 and low the gpio d2
    gpio_set_level(tdcs_d1_pin, 0);
    gpio_set_level(tdcs_d2_pin, 1);

    gpio_intr_enable(tdcs_over_curr_intr_pin);
    delay(40);
    ///////////// make the flag ==0
    flag = 0;

    // //////// save the pervious millis
    // prev_milli_ = millis();
    /////////// the max allowed current is set at prior
    max_set_current = amplitude;
    ////////// configure the waveforms

    switch (waveform_type)
    {
        case tac_tdcs_prot:
            /// /////// init the tdcs protocol paramters  aka the global variables

            // /// save the sample delay for ramp
            sample_delay_for_ramp = sampl_delay(frequency, amplitude);
            /// //// time till the constant current should applied
            time_till_waveform_run = time_till_run;
            break;

        case tac_ramp_fun_bi:
            d = (2500000 / (amplitude * frequency));

            break;

        case tac_ramp_fun_uni:
            d = (5000000 / (amplitude * frequency));
            break;

        case tac_sine_wave:
            d = ((10000 / frequency) - time_taken_spi); /////////// here the SPI transaction delay is subtracted

            break;

        case tac_square_bi:
            set_Current(max_set_current); ///////// this line is redundnat here as the current values is not changing but still used.
            d = 500000 / frequency;
            break;

        case tac_square_uni:
            d = 500000 / frequency;

            break;

        default:
            break;
    }

    set_Current(0);
    //   printf("flag val is %d,prev_milli%lld, currentmilli %lld,sampledelayforramp
    //   %lld\r\n",flag,prev_milli_,millis(),sample_delay_for_ramp);
}

void tdcs_deinit(void)
{
    // set_Current(0);
    gpio_intr_disable(tdcs_over_curr_intr_pin);
    flag = 0;

    prev_milli_ = 0;
    d = 0;

    /////// set the current to 0

    send_tdc(WRITE_RESET_REG, 0x0001); // Reset registers to default
    send_tdc(NOP, 0x0000);             // NOP operation

    /// write 0 current ;
    send_tdc(DAC_DATA, 0x00);
    set_curr = 0;
    mode = none;

    delay(10);
    //////////////////// used for ramp time in tdcs function only
    sample_delay_for_ramp = 0;

    //////// turn off the gpio pins d1 and d2
    gpio_set_level(tdcs_d1_pin, 0);
    gpio_set_level(tdcs_d2_pin, 0);

    gpio_set_level(tdcs_boost_en_pin, 0);
}

void set_Current(uint16_t current)
{
    if (current > 19990)
        current = 19990;
    // set_curr = current;
    //////// get the current value (20/4096)
    current *= 3.2852f;
    send_tdc(DAC_DATA, (uint16_t) current); /// 32 dec
}

void incremt(void)
{
    set_curr += 10;
    set_Current(set_curr);
}

void decrement(void)
{
    set_curr -= 10;
    set_Current(set_curr);
}

///////////// toogle function
static void toogle(void)
{
    static uint8_t state = 0; /////////// 0 is redundant here not neccesary but is for signify that the value is 0

    state = !state;
    gpio_set_level(tdcs_d1_pin, !state);
    gpio_set_level(tdcs_d2_pin, state);
}

////////////// configure the sine wave
void sine_wave()
{

    for (uint16_t i = 0; i < 156; i++)
    {
        send_tdc(DAC_DATA, (Sinex[i] << 4));
        delay_microsec(d);
    }
    toogle();
    for (uint16_t i = 0; i < 156; i++)
    {
        send_tdc(DAC_DATA, (Sinex[i] << 4));
        delay_microsec(d);
    }
    // for(uint16_t i=0; i<180; i++)
    //   {
    //    set_Current((amp *sinf(ang_freq * freq * i)));
    //     delay_microsec(d);
    // }
    //  for(uint16_t i=179; i>0; i--)
    //   {
    //    set_Current((amp *sinf(ang_freq  * freq * i)));
    //     delay_microsec(d);

    //   }
}

////////////////// configure the ramp function give time in seconds
void ramp_fun_uni()
{

    if (d < 1000)
    {

        d = (d <= 50) ? (51) : (d); ///// check whether the d reached below 50 and then set it to 50
        for (uint32_t i = 1; i < max_set_current; i += 10)
        {
            set_Current(i);
            delay_microsec(d - time_taken_spi);
        }
        for (int i = max_set_current; i > 0; i -= 10)
        {
            set_Current(i);
            delay_microsec(d - time_taken_spi);
        }
    } else
    {
        static int am = 0;
        static uint8_t flag = 0;
        if ((millis() - prev_milli_) > d)
        {
            prev_milli_ = millis();
            if (flag == 0)
            {
                am += 10;
                set_Current(am);
                if (am > max_set_current)
                {
                    flag = 1;
                    am = max_set_current;
                }
            } else
            {
                am -= 10;
                set_Current(am);
                if (am < 0)
                {
                    flag = 0;
                    am = 0;
                }
            }
        }
    }
}

void ramp_fun_bi()
{

    if (d < 1000)
    {
        d = (d <= 50) ? (51) : (d); ///// check whether the d reached below 50 and then set it to 50
        for (uint32_t i = 1; i < max_set_current; i += 10)
        {
            set_Current(i);
            delay_microsec(d - time_taken_spi);
        }
        for (int i = max_set_current; i > 0; i -= 10)
        {
            set_Current(i);
            delay_microsec(d - time_taken_spi);
        }
        toogle();
    } else
    {
        static int am = 0;
        static uint8_t flag = 0;

        if ((millis() - prev_milli_) > d)
        {
            prev_milli_ = millis();
            if (flag == 0)
            {
                am += 10;
                set_Current(am);
                if (am > max_set_current)
                {
                    flag = 1;
                    am = max_set_current;
                }
            } else
            {
                am -= 10;
                set_Current(am);
                if (am < 0)
                {
                    flag = 0;
                    am = 0;
                }
            }
        }
    }
}

void square_uni()
{
    static uint8_t flag = 0;

    if (d < 1000) ////// if the delay is less than 1msec then use the micros func
    {
        d = (d <= 50) ? (51) : (d); ///// check whether the d reached below 50 and then set it to 50
        set_Current(max_set_current);
        delay_microsec(d - time_taken_spi);
        set_Current(1);
        delay_microsec(d - time_taken_spi);
    } else
    {
        if ((millis() - prev_milli_) > (d / 1000))
        {
            prev_milli_ = millis();
            if (flag == 0)
            {
                set_Current(max_set_current);
                flag = 1;
            } else
            {
                flag = 0;
                set_Current(1);
            }
        }
    }
}

void square_bi()
{

    if (d < 1000)
    {
        d = (d <= 50) ? (51) : (d); ///// check whether the d reached below 50 and then set it to 50
        delay_microsec(d - time_taken_spi);
        toogle();
    } else
    {
        if ((millis() - prev_milli_) > (d / 1000))
        {
            prev_milli_ = millis();
            toogle();
        }
    }
}

/// @brief check the tdcs protection mechanisms here
/// @param  void
/// @return err/ok
uint32_t check_tdcs_protection(void)
{
    //// check overcurrent here
    if (flag == 1)
    {
        flag = 0;
        return err_tDCS_overcurrent_hardware_trigerred;
    }

    uint32_t measured_current = get_act_current();

    //// check open electrodes here
    if (set_curr > TDCS_IMPEDANCE_CURRENT_THRESHOLD_VALUE)
    {
        if (measured_current < TDCS_ELECTRODE_OPEN_CIRCUIT_VLAUE)
        {
            return err_tDCS_electrodes_are_open;
        }
    }

    /// check the software based overcurrent
    if (measured_current > TDCS_ELECTRODES_OVERCURRENT_LIMIT)
    {
        return err_tDCS_overcurrent_software_trigerred;
    }

    /// if everything is true then
    return ESP_OK;
}

extern esp_err_t esp_ble_send_notif_tdcs_curr(uint8_t*, uint16_t);

///////////////// to run the tdcs
////// if the function is inline then no static variables are allowed in it .
void run_tdcs(void)
{

    if (mode == ramp_up || mode == none)
    {
        set_curr += 10; /////0.01 ma increment
        set_Current(set_curr);

        //////////// send the current to ble
        current_imp measured_imp = {0};
        measured_imp.set_curent = set_curr;
        measured_imp.measured_current = get_act_current();

        ///////// send the measured and set crrent
        esp_ble_send_notif_tdcs_curr(u8(measured_imp), s(measured_imp));
        printf("rampup set_current %d measure%ld\r\n", measured_imp.set_curent, measured_imp.measured_current);

        if (set_curr >= max_set_current)
        {
            mode = constant;
            prev_milli_ = millis();
        }
    }
    //////// comment
    ////////////mode is constant cureent
    else if (mode == constant)
    {
        //////////// send the current to ble
        current_imp measured_imp = {0};
        measured_imp.set_curent = set_curr;
        measured_imp.measured_current = get_act_current();

        ///////// send the measured and set crrent
        esp_ble_send_notif_tdcs_curr(u8(measured_imp), s(measured_imp));
        printf("constant set_current %d measure%ld\r\n", measured_imp.set_curent, measured_imp.measured_current);

        if ((millis() - prev_milli_) >= time_till_waveform_run)
        {
            mode = ramp_down;
        }
    } else if (mode == ramp_down)
    {

        set_curr -= 10;

        //////////// send the current to ble
        current_imp measured_imp = {0};
        measured_imp.set_curent = set_curr;
        measured_imp.measured_current = get_act_current();

        ///////// send the measured and set crrent
        esp_ble_send_notif_tdcs_curr(u8(measured_imp), s(measured_imp));
        printf("rampdown set_current %d measure%ld\r\n", measured_imp.set_curent, measured_imp.measured_current);

        set_Current(set_curr);

        if (set_curr <= 10)
        {
            mode = complete;
        }
    }
}

/// @brief abort the tdcs process
/// @param
void abort_tdcs(void)
{
    mode = ramp_down;
}

/// @brief is the tdcs procedure complete or not
/// @return true /false
uint8_t is_tdcs_complete(void)
{
    return (mode == complete) ? (1) : (0);
}

/// @brief get the delay time for te tdcs process
/// @param  void
/// @return delay time
uint32_t tdcs_get_delay_Time(void)
{
    return sample_delay_for_ramp;
}

void read_tdc_reg(void)
{
    printf("reading the tdcs registers \r\n");
    printf("control reg %x, conf reg  %x, status reg  %x, dac data reg  %x\r\n",
           get_tdc(CONTROl_reg),
           get_tdc(CONF_reg),
           get_tdc(STATUS_reg),
           get_tdc(DAC_data_reg));
}

void help(void)
{
    printf(" this is the test code and these are the function supported \r\n");
    printf(" press h for help \r\n press c to set current \r\n press b for sine wave \r\n press i  for increment by 0.1ma \r\n");
    printf("press d for decrement 0.1ma \r\n press r for ramp wave \r\n\
    press e to stop anything immediately \r\n press s for square unidirectional wave \r\n press t for tdcs \r\n press a for abort \r\n press m for reading the tdcs register  \r\n");
    printf(" please note that if the set current and set freq message is displayed then if you start typing make sure to type the whole "
           "value in 5 seconds otherwise LOL can happen :< .\r\n");
}
