#include "system.h"



extern void bt_controller_off(void);




//////////////// message buffer data
static uint8_t gen_task_msg_buffer_storage[msg_bfr_size];

StaticMessageBuffer_t gen_task_msg_struct;
///////// message buffer handle
MessageBufferHandle_t gtsk_bfr_handle = NULL;

/////////// static queue for error code

static QueueHandle_t err_q_handle = NULL;

static uint8_t err_codes_buffer[error_queue_length * error_q_item_Size];

static StaticQueue_t error_code_static_queue;

////////////////// static queue for status

static QueueHandle_t status_q_handle = NULL;

static uint8_t stat_codes_buffer[status_queue_length * status_q_item_size];

static StaticQueue_t status_code_static_queue;

//////////// all the initialization functions here

static void msg_buff_create(void)
{

    // define a message buffer here
    gtsk_bfr_handle = xMessageBufferCreateStatic(msg_bfr_size, gen_task_msg_buffer_storage, &gen_task_msg_struct);
    /// @brief  check if the buffer handler is null
    configASSERT(gtsk_bfr_handle);
}

//////////error codes
static void create_err_queue(void)
{
    err_q_handle = xQueueCreateStatic(error_queue_length, error_q_item_Size, err_codes_buffer, &error_code_static_queue);
    configASSERT(err_q_handle);
}

static void create_sts_queue(void)
{
    status_q_handle = xQueueCreateStatic(status_queue_length, status_q_item_size, stat_codes_buffer, &status_code_static_queue);
    // if(status_q_handle == NULL)assert(0);
    configASSERT(status_q_handle);
}

////////////////////// send the error codes

void send_err_code(uint32_t code)
{
    
    xQueueOverwrite(err_q_handle, &code);
    
}

void send_stats_code(uint8_t code)
{
    
    xQueueOverwrite(status_q_handle, &code);
    
}

uint8_t get_no_item_err_q(void)
{
    return (uint8_t)uxQueueMessagesWaiting(err_q_handle);
}

uint8_t get_no_item_stat_q(void)
{
    return (uint8_t)uxQueueMessagesWaiting(status_q_handle);
}

uint32_t pop_err_q(void)
{
    uint32_t recv = 0;
    if (xQueueReceive(err_q_handle, &recv, wait_for_q_msg) == pdPASS)
        return recv;
    return 0;
}

uint8_t pop_stat_q(void)
{
    uint8_t recv = 0;
    if (xQueueReceive(status_q_handle, &recv, wait_for_q_msg) == pdPASS)
        return recv;
    return 0;
}

void reset_err_q(void)
{
    xQueueReset(err_q_handle);
}
void reset_status_q(void)
{
    xQueueReset(status_q_handle);
}

void reset_msg_buffer(void)
{
    ////////// clear the message buffer
    xMessageBufferReset(gtsk_bfr_handle);
}

void pop_msg_buff(uint8_t *buff, uint16_t size)
{
    xMessageBufferReceive(gtsk_bfr_handle, buff, size, wait_for_msg_buffer);
}

void push_msg_buff(uint8_t * buff, uint16_t size)
{
    xMessageBufferSend(gtsk_bfr_handle, buff, size , wait_for_msg_buffer);
}
bool is_msgbuff_empty(void)
{
    return xMessageBufferIsEmpty(gtsk_bfr_handle);
}
void taskdelete(TaskHandle_t *handle)
{
    if (*handle != NULL)
    {
        vTaskDelete(*handle);
        *handle = NULL;
    }
}


/////////////////////////////////////////////////////////////
////////////// deinits functions for properly deinit the overall functioanlities 
extern void tdcs_deinit(void);

extern void ads_deint(void);


////////////////////////// shutdown the system
void system_shutdown(void)
{
    ////////// other functions to sleep the ICS
    //////// deinit the tdcs module
    tdcs_deinit();
    ////////// deinit the ads module
    ads_deint();
    // // turn off all the wakeup domain source 
    esp_sleep_pd_config(ESP_PD_DOMAIN_MAX,  ESP_PD_OPTION_OFF);

    // turn off the ble hardware 
    bt_controller_off();
    ////////// put the esp in sleep modes
    esp_deep_sleep_start();
}
/////////////////////////////////////////////////////////////////////////////////////////
//*************************************************************************************************
//////*********************************** ADC regarding API***************************************************
//*************************************************************************************************

// calibration charcteristics
static esp_adc_cal_characteristics_t adc_chars = {0};

static const adc_channel_t channel = ADC_CHANNEL_7; // GPIO35 if ADC1, GPIO14 if ADC2
static const adc_bits_width_t width = ADC_WIDTH_BIT_12;

static const adc_atten_t atten = ADC_ATTEN_DB_11;
static const adc_unit_t unit = ADC_UNIT_1;

static void check_efuse(void)
{
    // Check if TP is burned into eFuse
    if (esp_adc_cal_check_efuse(ESP_ADC_CAL_VAL_EFUSE_TP) == ESP_OK)
    {
        printf("eFuse Two Point: Supported\n");
    }
    else
    {
        printf("eFuse Two Point: NOT supported\n");
    }
    // Check Vref is burned into eFuse
    if (esp_adc_cal_check_efuse(ESP_ADC_CAL_VAL_EFUSE_VREF) == ESP_OK)
    {
        printf("eFuse Vref: Supported\n");
    }
    else
    {
        printf("eFuse Vref: NOT supported\n");
    }
}

static void print_char_val_type(esp_adc_cal_value_t val_type)
{
    if (val_type == ESP_ADC_CAL_VAL_EFUSE_TP)
    {
        printf("Characterized using Two Point Value\n");
    }
    else if (val_type == ESP_ADC_CAL_VAL_EFUSE_VREF)
    {
        printf("Characterized using eFuse Vref\n");
    }
    else
    {
        printf("Characterized using Default Vref\n");
    }
}

static void calib_adc(void)
{
    gpio_set_direction(GPIO_NUM_35, GPIO_MODE_INPUT);
    gpio_set_pull_mode(GPIO_NUM_35, GPIO_FLOATING);
    // Check if Two Point or Vref are burned into eFuse
    adc_power_acquire();
    check_efuse();

    // Configure ADC

    adc1_config_width(width);
    adc1_config_channel_atten(channel, atten);

    // Characterize ADC
    esp_adc_cal_value_t val_type = esp_adc_cal_characterize(unit, atten, width, DEFAULT_VREF, &adc_chars);

    print_char_val_type(val_type);
}

uint32_t adc_get_vol(void)
{
    // printf("Raw: reading is %d , voltage is mv %f \r\n", adc_reading, (adc_reading *0.598f));

    return esp_adc_cal_raw_to_voltage(adc1_get_raw(channel), &adc_chars);
}

uint32_t get_act_current(void)
{
    uint32_t i_value = 0;
    for(int i=0; i< ADC_SAMPLING_POINTS; i++)
    {

        i_value += adc_get_vol()/Resistance;
    }
    return i_value/ADC_SAMPLING_POINTS;
    
}
////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////
//////////////// system initialization here

void put_color(uint8_t color)
{
    switch (color)
    {
    case RED_COLOR:
        gpio_set_level(red_led_pin, 1);
        gpio_set_level(green_led_pin, 0);
        gpio_set_level(blue_led_pin, 0);
        break;

    case GREEN_COLOR:
        gpio_set_level(red_led_pin, 0);
        gpio_set_level(green_led_pin, 1);
        gpio_set_level(blue_led_pin, 0);
        break;

    case BLUE_COLOR:
        gpio_set_level(red_led_pin, 0);
        gpio_set_level(green_led_pin, 0);
        gpio_set_level(blue_led_pin, 1);
        break;

    case ORANGE_COLOR:
        gpio_set_level(red_led_pin, 1);
        gpio_set_level(green_led_pin, 0);
        gpio_set_level(blue_led_pin, 1);

        break;
    case YELLOW_COLOR:
        gpio_set_level(red_led_pin, 1);
        gpio_set_level(green_led_pin, 1);
        gpio_set_level(blue_led_pin, 0);

        break;

    case NO_COLOR:
        gpio_set_level(red_led_pin, 0);
        gpio_set_level(green_led_pin, 0);
        gpio_set_level(blue_led_pin, 0);

        break;
    case WHITE_COLOR:    
        gpio_set_level(red_led_pin, 1);
        gpio_set_level(green_led_pin,1);
        gpio_set_level(blue_led_pin, 1);
        break;
        
    case PURPLE_COLOR:    
        gpio_set_level(red_led_pin, 1);
        gpio_set_level(green_led_pin,0);
        gpio_set_level(blue_led_pin, 1);
        break;

    default:
        gpio_set_level(red_led_pin, 0);
        gpio_set_level(green_led_pin, 0);
        gpio_set_level(blue_led_pin, 0);

        break;
    }
}

void system_init(void)
{
     // install gpio isr service
    gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT);
      const gpio_config_t leds_cfg =
    {
        .intr_type = GPIO_INTR_DISABLE,
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask = lights_pin,
        .pull_down_en =0,
        .pull_up_en =0,
    };

    gpio_config(&leds_cfg);
    // @note not use this functions as they are not able to touch IO MUX register 

    // /// @brief ////// init the system leds
    // gpio_set_direction(red_led_pin, GPIO_MODE_OUTPUT);
    // gpio_set_direction(green_led_pin, GPIO_MODE_OUTPUT);
    // gpio_set_direction(blue_led_pin, GPIO_MODE_OUTPUT);

    put_color(NO_COLOR);

    //////// init all the system parameters
    calib_adc();
    create_err_queue();
    create_sts_queue();
    msg_buff_create();
    func_timer_init();
   
}