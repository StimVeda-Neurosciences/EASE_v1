#include "time.h"

/**
 * @brief Initialize selected timer of timer group
 *
 * @param group Timer Group number, index from 0
 * @param timer timer ID, index from 0
 * @param auto_reload whether auto-reload on alarm event
 * @param timer_interval_sec interval of alarm
 */
void func_timer_init()
{
    /* Select and initialize basic parameters of the timer */
    timer_config_t config = {
        .divider = TIMER_DIVIDER,
        .counter_dir = TIMER_COUNT_UP,
        .counter_en = TIMER_PAUSE,
        .alarm_en = TIMER_ALARM_DIS,
        .auto_reload = TIMER_AUTORELOAD_DIS,
    }; // default clock source is APB
    timer_init(TIMER_GROUP_1,TIMER_0, &config);

    /* Timer's counter will initially start from value below.
       Also, if auto_reload is set, this value will be automatically reload on alarm */
    timer_set_counter_value(TIMER_GROUP_1, TIMER_0, 0);

      timer_start(TIMER_GROUP_1, TIMER_0);
}


void IRAM_ATTR delay_microsec(uint64_t dely)
{
    uint64_t prev_cnt=0, cnt =0;
    timer_get_counter_value(TIMER_GROUP_1, TIMER_0, &prev_cnt);

    while(cnt < (prev_cnt + dely))
    {
        timer_get_counter_value(TIMER_GROUP_1, TIMER_0, &cnt);
    }

}


uint64_t IRAM_ATTR micros(void)
{
    uint64_t cnt =0;
    timer_get_counter_value(TIMER_GROUP_1, TIMER_0, &cnt);
    return cnt;   
}

uint64_t IRAM_ATTR millis(void)
{
    uint64_t cnt =0;
    timer_get_counter_value(TIMER_GROUP_1, TIMER_0, &cnt);
    cnt /= 1000;
    return cnt;
}





