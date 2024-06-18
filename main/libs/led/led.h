#pragma once

#include <stdio.h>
#include <stdint.h>

#include "system_attr.h"
#include "esp_err.h"

////////////////////////////////////////////////////////////////
/////////////////////// enum to store the color of the system led
// =========== define the timer configuration here =======================

#define LED_TIMER_FREQ (10 * 1000) // 10Khz

#define GET_ALARM_FROM_MSEC(x) (x * 10) /// get the alarm count value in  milliseconds

#define LED_COLOR_MIN_FADE_VALUE (4)

/// deifne the min blink time that should be present in order to start the driver 
#define LED_TIMER_MIN_BLINK_TIME  (8) /// 8 millisecond is the least time

#define LED_TIMER_MIN_OFF_TIME (8) /// 8 milli second 

#define LED_TIMER_MIN_FADE_TIME (8) /// 8millisecond 


#define MAX_COLOR_INTENSITY 8192

#define MAX_FADE_TIME 0xffUL

typedef struct __LED_COLORS__
{
    uint16_t red;
    uint16_t green;
    uint16_t blue;
}PACKED led_color_struct_t;

/// @brief red blue green
#define COLOR(red, green, blue)  (led_color_struct_t){(red),(green),(blue)}

#define RED_COLOR COLOR(MAX_COLOR_INTENSITY, 0, 0)
#define GREEN_COLOR COLOR(0,MAX_COLOR_INTENSITY,0)
#define BLUE_COLOR COLOR(0, 0,MAX_COLOR_INTENSITY)
#define ORANGE_COLOR COLOR(MAX_COLOR_INTENSITY,0, MAX_COLOR_INTENSITY/2 ) // orange
#define YELLOW_COLOR COLOR(MAX_COLOR_INTENSITY/2, MAX_COLOR_INTENSITY/2, 0) // Yellow
#define PURPLE_COLOR COLOR(MAX_COLOR_INTENSITY/2, 0, MAX_COLOR_INTENSITY/2) // Purple
#define NO_COLOR COLOR(0, 0, 0)
#define WHITE_COLOR COLOR(MAX_COLOR_INTENSITY, MAX_COLOR_INTENSITY, MAX_COLOR_INTENSITY)

/// @brief color fade modes of the LED
typedef enum __COLOR_FADE_MODE__
{
    MODE_FADE_NONE,      // only solid colors -- no fading at all
    MODE_FADE_IN,        // fade in the color i.e. only led fade from zero to max brightness
    MODE_FADE_OUT,       // fade out i.e. only led fade from max brightness to zero
    MODE_FADE_BOTH,      // both fade in and fade out
    MODE_FADE_UNDEFINED, // fade is undefined ,
} led_color_fade_mode_enum_t;

typedef struct __LED_TIEM_CONFIG__
{
    uint32_t fade_in_time; // fade in time in milliseconds
    uint32_t fade_out_time; /// fade out time in milliseconds 
    uint32_t blink_time;    //// blink time in milliseconds 
    uint32_t off_time;      //// turn off time in seconds, after this driver will automatically off 
}PACKED led_time_config_struct_t;


#define LED_OFF_TIME_MAX (0xFFFFFFFAUL)

// ----------------------------------------------------------------------------------------------------------------

#define COLOR_OFF_TIME(OFF_TIME)    \
    (led_time_config_struct_t){0,0,0,MAX_OF(OFF_TIME,LED_TIMER_MIN_OFF_TIME)}  \
 
#define COLOR_TIME_MAX \
    COLOR_OFF_TIME(LED_OFF_TIME_MAX)
// ----------------------------------------------------------------------------------------------------------------

#define BLINK_TIME_MAX(BLINK) \
    (led_time_config_struct_t){MAX_OF(LED_TIMER_MIN_BLINK_TIME,BLINK),MAX_OF(LED_TIMER_MIN_BLINK_TIME,BLINK),0,LED_OFF_TIME_MAX}  \

#define BLINK_TIME(BLINK,OFF) \
    (led_time_config_struct_t){MAX_OF(LED_TIMER_MIN_BLINK_TIME,BLINK),MAX_OF(LED_TIMER_MIN_BLINK_TIME,BLINK),0,MAX_OF(OFF,LED_TIMER_MIN_OFF_TIME)}  \

// ----------------------------------------------------------------------------------------------------------------

#define BLINK_TIME_BOTH_MAX(BLINK_ON,BLINK_OFF)     \
    (led_time_config_struct_t){MAX_OF(BLINK_ON,LED_TIMER_MIN_BLINK_TIME),MAX_OF(BLINK_OFF,LED_TIMER_MIN_BLINK_TIME),0,LED_OFF_TIME_MAX}  \

#define BLINK_TIME_BOTH(BLINK_ON,BLINK_OFF,OFF)     \
    (led_time_config_struct_t){MAX_OF(BLINK_ON,LED_TIMER_MIN_BLINK_TIME),MAX_OF(BLINK_OFF,LED_TIMER_MIN_BLINK_TIME),0,(OFF)}  \
// ----------------------------------------------------------------------------------------------------------------

#define FADE_TIME_BLINK(BLINK_ON,BLINK_OFF,BLINK_TIME,OFF) \
    (led_time_config_struct_t){MAX_OF(BLINK_ON,LED_TIMER_MIN_BLINK_TIME),MAX_OF(BLINK_OFF,LED_TIMER_MIN_BLINK_TIME),MAX_OF(LED_TIMER_MIN_BLINK_TIME,BLINK_TIME),(OFF)}  \

// ----------------------------------------------------------------------------------------------------------------

#define FADE_TIME_MAX(FADE_IN,FADE_OUT) \
    (led_time_config_struct_t){MAX_OF(FADE_IN,LED_TIMER_MIN_FADE_TIME),MAX_OF(FADE_OUT,LED_TIMER_MIN_FADE_TIME),LED_TIMER_MIN_BLINK_TIME,(LED_OFF_TIME_MAX)}    \

#define FADE_TIME(FADE_IN,FADE_OUT,OFF) \
    (led_time_config_struct_t){MAX_OF(FADE_IN,LED_TIMER_MIN_FADE_TIME),MAX_OF(FADE_OUT,LED_TIMER_MIN_FADE_TIME),LED_TIMER_MIN_BLINK_TIME,(OFF)}    \

// ----------------------------------------------------------------------------------------------------------------

/// @brief initialise the led driver
/// @param  void
void led_driver_init(void);

/// @brief deinit the led driver
/// @param  void
void led_driver_deinit(void);

typedef void (*ledc_cmpt_callback)(void *args);

/// @brief this registered callback called when the operation is complete (off time expires)
/// @param  callback
/// @param  user_argument
void led_driver_register_cmpt_callback(ledc_cmpt_callback, void * user_arg);

/// @brief remove the complete callback from the led driver 
/// @param  void
void led_driver_remove_cmpt_callback(void);

/// @brief this api would stop all the outgoing operations on led 
/// @param  void 
void led_driver_stop_all_operations(void);

/// @brief this will wait for the completion of the operations  
/// @param  time
/// @return succ/err
esp_err_t led_driver_wait_for_completion(uint32_t time );


/// @brief put the color on the led
/// @param color
esp_err_t led_driver_put_color(led_color_struct_t, led_time_config_struct_t);

/// @brief no color on the led
/// @param
esp_err_t led_driver_no_color(void);

/// @brief blink the led with the blink time and off time 
/// @param color 
/// @param time 
/// @return succ/err 
esp_err_t led_driver_blink_color(led_color_struct_t color, led_time_config_struct_t time);

/// @brief this is to fade the color on the led with this configuration 
/// @param color 
/// @param time 
/// @return succ/err code 
esp_err_t led_driver_fade_color(led_color_struct_t color,led_time_config_struct_t time );

