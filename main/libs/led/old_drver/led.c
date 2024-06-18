#include "led.h"

#include "driver/ledc.h"

#include "driver/gpio.h"

#include "esp_attr.h"
#include "esp_err.h"
#include "esp_log.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

#define LED_DRIVER_FREQ (1 * 1000 * 5) // 50 KHZ
#define LED_TIMER_RESOLUTION LEDC_TIMER_13_BIT
#define LEDC_MODE LEDC_HIGH_SPEED_MODE
#define LEDC_TIMER LEDC_TIMER_0


#define RED_COLOR_CHANNEL LEDC_CHANNEL_0
#define RED_COLOR_PIN PIN_RED_LED

#define BLUE_COLOR_CHANNEL LEDC_CHANNEL_1
#define BLUE_COLOR_PIN PIN_BLUE_LED

#define GREEN_COLOR_CHANNEL LEDC_CHANNEL_2
#define GREEN_COLOR_PIN PIN_GREEN_LED

////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////

// semaphore handle 
SemaphoreHandle_t s_handle;

/// @brief ledc isr
/// @param param
static IRAM_ATTR bool ledc_fade_callback(const ledc_cb_param_t *param, void *uesr_args)
{
    portBASE_TYPE high_task_awoken = pdFALSE;
    
    if (param->event == LEDC_FADE_END_EVT) {
    xSemaphoreGiveFromISR(s_handle,&high_task_awoken);
    }
    return high_task_awoken;
}

/// @brief initialise the led driver
/// @param  void
void led_driver_init(void)
{
    // configure the timer
    static const ledc_timer_config_t led_timer_config =
        {
            .timer_num = LEDC_TIMER,
            .duty_resolution = LED_TIMER_RESOLUTION,
            .clk_cfg = LEDC_AUTO_CLK,
            .freq_hz = LED_DRIVER_FREQ,
            .speed_mode = LEDC_MODE,
        };
    esp_err_t err = ledc_timer_config(&led_timer_config);
    assert((err == 0));

    // configure the channel
    static const ledc_channel_config_t led0_channel_config =
        {
            .channel = RED_COLOR_CHANNEL,
            .gpio_num = RED_COLOR_PIN,
            .intr_type = LEDC_INTR_FADE_END,
            .speed_mode = LEDC_MODE,
            .timer_sel = LEDC_TIMER,
            .hpoint = 0,
            .duty = 0,
        };
    err = ledc_channel_config(&led0_channel_config);
    assert((err == 0));

    static const ledc_channel_config_t led1_channel_config =
        {
            .channel = BLUE_COLOR_CHANNEL,
            .gpio_num = BLUE_COLOR_PIN,
            .intr_type = LEDC_INTR_FADE_END,
            .speed_mode = LEDC_MODE,
            .timer_sel = LEDC_TIMER,
            .hpoint = 0,
            .duty = 0,
        };
    err = ledc_channel_config(&led1_channel_config);
    assert((err == 0));

    static const ledc_channel_config_t led2_channel_config =
        {
            .channel = GREEN_COLOR_CHANNEL,
            .gpio_num = GREEN_COLOR_PIN,
            .intr_type = LEDC_INTR_FADE_END,
            .speed_mode = LEDC_MODE,
            .timer_sel = LEDC_TIMER,
            .hpoint = 0,
            .duty = 0,
        };
    err = ledc_channel_config(&led2_channel_config);
    assert((err == 0));

    // configure the fade interrupts
    ledc_fade_func_install(ESP_INTR_FLAG_IRAM);

    ledc_cbs_t callback = {.fade_cb = ledc_fade_callback};
    ledc_cb_register(LEDC_MODE,RED_COLOR_CHANNEL, &callback,NULL);
    ledc_cb_register(LEDC_MODE,GREEN_COLOR_CHANNEL,&callback,NULL);
    ledc_cb_register(LEDC_MODE,BLUE_COLOR_CHANNEL,&callback,NULL);

    // create a biary semaphore 
    s_handle =  xSemaphoreCreateCounting(3,0);
    for(int i=0; i<3; i++)
    {

    xSemaphoreGive(s_handle);
    }
}
/// @brief deinit the led driver
/// @param  void
void led_driver_deinit(void)
{
    ledc_set_duty(LEDC_MODE,RED_COLOR_CHANNEL,)
    ledc_timer_pause(LEDC_MODE,LEDC_TIMER);
    ledc_fade_func_uninstall();
}

/// @brief put the color on the led
/// @param color
void led_driver_put_color(led_color_struct_t color)
{

    ledc_set_duty(LEDC_MODE, RED_COLOR_CHANNEL, color.red);
    ledc_set_duty(LEDC_MODE, GREEN_COLOR_CHANNEL, color.green);
    ledc_set_duty(LEDC_MODE, BLUE_COLOR_CHANNEL, color.blue);

    ledc_update_duty(LEDC_MODE, RED_COLOR_CHANNEL);
    ledc_update_duty(LEDC_MODE, GREEN_COLOR_CHANNEL);
    ledc_update_duty(LEDC_MODE, BLUE_COLOR_CHANNEL);
}

/// @brief no color on the led
/// @param
void led_driver_no_color(void)
{
  
    ledc_stop(LEDC_MODE, RED_COLOR_CHANNEL, 0);
    ledc_stop(LEDC_MODE, BLUE_COLOR_CHANNEL, 0);
    ledc_stop(LEDC_MODE, GREEN_COLOR_CHANNEL, 0);
}

/// @brief put the color with fade on the led
/// @param color
/// @param fade_time
/// @param fade_mode
void led_driver_put_color_fade(led_color_struct_t color,  uint32_t fade_time, uint8_t fade_mode)
{   
        // take the binarty semaphore 
    for(int i=0; i<3;i++)
    {

    xSemaphoreTake(s_handle,2000);
    }

    /// set the fade with time 
    ledc_set_fade_with_time(LEDC_MODE, RED_COLOR_CHANNEL, color.red,fade_time);
    ledc_set_fade_with_time(LEDC_MODE, GREEN_COLOR_CHANNEL, color.green,fade_time);
    ledc_set_fade_with_time(LEDC_MODE, BLUE_COLOR_CHANNEL, color.blue,fade_time);

    
    ledc_fade_start(LEDC_MODE,RED_COLOR_CHANNEL,LEDC_FADE_NO_WAIT);
    ledc_fade_start(LEDC_MODE,GREEN_COLOR_CHANNEL,LEDC_FADE_NO_WAIT);
    ledc_fade_start(LEDC_MODE,BLUE_COLOR_CHANNEL,LEDC_FADE_NO_WAIT);

}

/// @brief put the custom fade on the led
/// @param color
/// @param in_fade_Time in ms 
/// @param out_dafe_time
void led_driver_put_custom_fade(led_color_struct_t color, uint32_t in_fade_Time, uint32_t out_fade_time)
{
    for(int i=0; i<3; i++)
    {
        xSemaphoreTake(s_handle,10000);
    }
    /// set the fade with time 
    ledc_set_fade_with_time(LEDC_MODE, RED_COLOR_CHANNEL, color.red,in_fade_Time);
    ledc_set_fade_with_time(LEDC_MODE, GREEN_COLOR_CHANNEL, color.green,in_fade_Time);
    ledc_set_fade_with_time(LEDC_MODE, BLUE_COLOR_CHANNEL, color.blue,in_fade_Time);

    
    ledc_fade_start(LEDC_MODE,RED_COLOR_CHANNEL,LEDC_FADE_NO_WAIT);
    ledc_fade_start(LEDC_MODE,GREEN_COLOR_CHANNEL,LEDC_FADE_NO_WAIT);
    ledc_fade_start(LEDC_MODE,BLUE_COLOR_CHANNEL,LEDC_FADE_NO_WAIT);

   
    for(int i=0; i<3; i++)
    {
        xSemaphoreTake(s_handle,10000);
    }

    ledc_set_fade_with_time(LEDC_MODE, RED_COLOR_CHANNEL, 0,1);
    ledc_set_fade_with_time(LEDC_MODE, GREEN_COLOR_CHANNEL, 0,1);
    ledc_set_fade_with_time(LEDC_MODE, BLUE_COLOR_CHANNEL, 0,1);

    
    ledc_fade_start(LEDC_MODE,RED_COLOR_CHANNEL,LEDC_FADE_NO_WAIT);
    ledc_fade_start(LEDC_MODE,GREEN_COLOR_CHANNEL,LEDC_FADE_NO_WAIT);
    ledc_fade_start(LEDC_MODE,BLUE_COLOR_CHANNEL,LEDC_FADE_NO_WAIT);

    
    /// set the fade with time 
    // ledc_set_fade_with_time(LEDC_MODE, RED_COLOR_CHANNEL, 0,out_fade_time);
    // ledc_set_fade_with_time(LEDC_MODE, GREEN_COLOR_CHANNEL, 0,out_fade_time);
    // ledc_set_fade_with_time(LEDC_MODE, BLUE_COLOR_CHANNEL, 0,out_fade_time);

    
    // ledc_fade_start(LEDC_MODE,RED_COLOR_CHANNEL,LEDC_FADE_NO_WAIT);
    // ledc_fade_start(LEDC_MODE,GREEN_COLOR_CHANNEL,LEDC_FADE_NO_WAIT);
    // ledc_fade_start(LEDC_MODE,BLUE_COLOR_CHANNEL,LEDC_FADE_NO_WAIT);
    
}