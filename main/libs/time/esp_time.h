#pragma once 

/////////standard lib
#include <stdio.h>
#include <stdint.h>

#define TIMER_DIVIDER         (80)  //  Hardware timer clock divider
#define TIMER_SCALE           (TIMER_BASE_CLK / TIMER_DIVIDER)  // convert counter value to seconds



/// @brief initialise a timer driver 
/// @param  void 
void esp_timer_driver_init(void);

/// @brief deinit a timer driver 
/// @param  
void esp_timer_driver_deinit(void);

/// @brief start the timer tick 
/// @param  void 
void esp_start_timer(void);

/// @brief stop the timer tick 
/// @param  void 
void esp_stop_timer(void);

/// @brief get the tick count of the timer 
/// @param  void 
/// @return tickcount
uint64_t esp_timer_get_tick(void);

/// @brief delay the time with given microsec 
/// @param  microsec
void delay_microsec(uint64_t );

/// @brief millis will return the millisecond from the timer 
/// @param  void 
/// @return millisec 
uint64_t millis(void);

/// @brief return the microsec
/// @param  void 
/// @return micros 
uint64_t micros(void);

