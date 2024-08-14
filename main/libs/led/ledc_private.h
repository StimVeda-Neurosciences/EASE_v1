/****
 * 
 * 
 * ledc private driver for implementing the ledc driver 
 * 
 */

#pragma once 

#include<stdio.h>
#include<stdint.h>

#include "system_attr.h"
#include "led.h"


#define LED_DRIVER_FREQ      (1 * 1000 * 5) // 5 KHZ
#define LED_TIMER_RESOLUTION LEDC_TIMER_13_BIT
#define LEDC_MODE            LEDC_HIGH_SPEED_MODE
#define LEDC_TIMER           LEDC_TIMER_0

#define RED_COLOR_CHANNEL LEDC_CHANNEL_0
#define RED_COLOR_PIN     PIN_RED_LED

#define BLUE_COLOR_CHANNEL LEDC_CHANNEL_1
#define BLUE_COLOR_PIN     PIN_BLUE_LED

#define GREEN_COLOR_CHANNEL LEDC_CHANNEL_2
#define GREEN_COLOR_PIN     PIN_GREEN_LED

#define MAX_NO_LEDS 3

/// @brief Led driver supported commands 
typedef enum __LEDC_DRV_COMMANDS__
{
    LEDC_CMD_NONE = 0x00,
    LEDC_CMD_IDLE = 0x01,
    LEDC_CMD_OFF,
    LEDC_CMD_FADER,
    LEDC_CMD_BLINK,
    LEDC_CMD_COLOR,
    LEDC_CMD_UNDEF,

} ledc_driver_cmds_t;

// ledc fade mode state required by the callback to handle the procedures 
typedef enum __LEDC_DRIVER_STATE__
{
    LEDC_DRV_STATE_NONE =0x00,
    LEDC_DRV_STATE_IDLE = LEDC_CMD_IDLE,
    LEDC_DRV_STATE_OFF,
    LEDC_DRV_STATE_UP = 0x02,
    LEDC_DRV_STATE_DOWN= 0x03,
    LEDC_DRV_STATE_SOLID = 0x04,
    
}ledc_fade_mode_state_t;

static_assert(sizeof(ledc_fade_mode_state_t)!=1);

typedef enum __LED_DRIVER_NOTIF_STATE__
{
    LED_NOTIF_NONE = 0x00,
    LED_NOTIF_TIMER_EXPIRES,
    LED_NOTIF_FADE_EXPIRES,
    LED_NOTIF_API_ENFORCES,
    LED_NOTIF_UNDEF,
}ledc_driver_notif_enum_t;

/// @brief this is the cmd queu structure of the ledc
typedef struct __LEDC_CMD_Q_STRUCTURE__
{
    uint8_t cmd;
    led_color_struct_t color;
    led_time_config_struct_t time;
} PACKED ledc_cmd_q_struct_t;

#define LEDC_CMD_Q_STRUCT_SIZE (sizeof(ledc_cmd_q_struct_t))

#define LEDC_CMD_Q_LEN 1
#define LEDC_CMD_Q_ITEM_SIZE LEDC_CMD_Q_STRUCT_SIZE


/// @brief to store the driver current state across different int handlers 
typedef struct __LEDC_CURRENT_DRIVER_STATE__
{
    bool request_to_idle;
    ledc_fade_mode_state_t current_state;
    uint32_t current_time;
}PACKED ledc_driver_state_struct_t;



