#include "esp_time.h"

#include "driver/gptimer.h"

#include "esp_attr.h"

#include "rtc_wdt.h"

#include "esp_log.h"

#define TAG "ESP_time"

// general purpose timer handle 
static gptimer_handle_t timer_handle = NULL;


/// @brief initialise a timer driver 
/// @param  void 
void esp_timer_driver_init(void)
{
    ESP_LOGW(TAG,"esp timer init");
static const gptimer_config_t timer_config = {
    .clk_src = GPTIMER_CLK_SRC_APB,
    .direction = GPTIMER_COUNT_UP,
    .resolution_hz = 1 * 1000 *1000, // 1 MHz, 1 tick = 1usec
};

    gptimer_new_timer(&timer_config, &timer_handle);
    gptimer_enable(timer_handle);
    gptimer_set_raw_count(timer_handle,0);

}

/// @brief deinit a timer driver 
/// @param  
void esp_timer_driver_deinit(void)
{
    ESP_LOGW(TAG,"Timer deinit");   
    gptimer_stop(timer_handle);
    gptimer_disable(timer_handle);
    gptimer_del_timer(timer_handle);
}

/// @brief start the timer tick 
/// @param  void 
void esp_start_timer(void)
{
    gptimer_start(timer_handle);
}

/// @brief stop the timer tick 
/// @param  void 
void esp_stop_timer(void)
{
    gptimer_stop(timer_handle);
}

/// @brief get the tick count of the timer 
/// @param  void 
/// @return tickcount
uint64_t IRAM_ATTR esp_timer_get_tick(void)
{
    uint64_t tick =0;
    if(gptimer_get_raw_count(timer_handle,&tick) != ESP_OK)
    {
        return 0;
    }
    return tick;
}




void IRAM_ATTR delay_microsec(uint64_t dely)
{
    uint64_t prev_cnt=0, cnt =0;
    gptimer_get_raw_count(timer_handle,&prev_cnt);
    while(cnt < (prev_cnt + dely))
    {
        gptimer_get_raw_count(timer_handle,&prev_cnt);
    
    }

}


uint64_t IRAM_ATTR micros(void)
{
    uint64_t cnt =0;
    gptimer_get_raw_count(timer_handle,&cnt);
    return cnt;   
}

uint64_t IRAM_ATTR millis(void)
{
    uint64_t cnt =0;
    gptimer_get_raw_count(timer_handle,&cnt);
    cnt /= 1000;
    return cnt;
}


/***
 * @brief this functin need to be called after the main app init and start correctly 
 * bootloa. use RTC watchdog to confirm that app doesn't crash or hang 
*/
void esp_stop_bootloader_watchdog(void)
{
    // wdt_hal_context_t rtc_wdt_ctx = {.inst = WDT_RWDT, .rwdt_dev = &RTCCNTL};
    // wdt_hal_write_protect_disable(&rtc_wdt_ctx);
    // wdt_hal_disable(&rtc_wdt_ctx);

    // check if there is a protection ON on the Watchdog
    if(rtc_wdt_get_protect_status())
    {
        rtc_wdt_protect_off();
    }
    // disable the rtc watchdog 
    rtc_wdt_disable();
}