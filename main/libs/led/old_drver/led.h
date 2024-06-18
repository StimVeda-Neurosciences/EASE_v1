#pragma once

#include <stdio.h>
#include <stdint.h>

#include "system_attr.h"

////////////////////////////////////////////////////////////////
/////////////////////// enum to store the color of the system led

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

/// @brief initialise the led driver
/// @param  void
void led_driver_init(void);

/// @brief deinit the led driver
/// @param  void
void led_driver_deinit(void);

/// @brief put the color on the led
/// @param color
void led_driver_put_color(led_color_struct_t);

/// @brief no color on the led
/// @param
void led_driver_no_color(void);

/// @brief put the color with fade on the led
/// @param color
/// @param fade_time
/// @param fade_mode
void led_driver_put_color_fade(led_color_struct_t,  uint32_t fade_time, uint8_t fade_mode);

/// @brief put the custom fade on the led
/// @param color
/// @param in_fade_Time
/// @param out_dafe_time
void led_driver_put_custom_fade(led_color_struct_t, uint32_t in_fade_Time, uint32_t out_dafe_time);
