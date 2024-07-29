#ifndef _TIMER_
#define _TIMER_

/////////standard lib
#include <stdio.h>
#include <stdint.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "driver/timer.h"




#define delay(x) (vTaskDelay(x/portTICK_PERIOD_MS))



#define TIMER_DIVIDER         (80)  //  Hardware timer clock divider
#define TIMER_SCALE           (TIMER_BASE_CLK / TIMER_DIVIDER)  // convert counter value to seconds




void func_timer_init(void);

void delay_microsec(uint64_t );

uint64_t millis(void);

uint64_t micros(void);



#endif