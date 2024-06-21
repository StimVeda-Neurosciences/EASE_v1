#include "led.h"

#include "ledc_private.h"

#include "driver/ledc.h"
#include "driver/gptimer.h"

#include "esp_attr.h"
#include "esp_err.h"
#include "esp_log.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"

#define TAG "LED_DRV"

// ==============================================================================================

// ------------------------------------ registered callback -----------------------------------------------

static void *callback_argument;
static ledc_cmpt_callback task_cmpt_callb;

// ---------------------------------  led timer configuration ---------------------------------------------
// led gp timer
static gptimer_handle_t led_timer_handle = NULL;

// -------------------------------- led command q related stuff -----------------------------------------
// CMD Q handle
static QueueHandle_t ledc_cmd_q_handle;

static StaticQueue_t ledc_cmd_q_control_block;

static uint8_t ledc_cmd_q_memory[LEDC_CMD_Q_LEN * LEDC_CMD_Q_ITEM_SIZE];

// ---------------------------------- led semphore related stuff --------------------------------------
// semaphore handle
static SemaphoreHandle_t ledc_semphr_handle;

static StaticSemaphore_t static_semaphore_control_block;

// this led fader number makes such that it only send semphr when all the led finish operations
static uint8_t led_fader_nums;

// store the driver current state
static ledc_driver_state_struct_t curr_state;

// ---------------------------------------------------------------------------------------------------------
// ------------------------ creating a task for the ledc for manging led configurations -------------
// ------------------------ ledc task for controlling led --------------------------------------------------

#define LED_TASK_PARAM NULL
#define LED_TASK_CPU PRO_CPU
#define LED_TASK_PRIORITY PRIORITY_5
#define LED_TASK_STACK_DEPTH 2048
#define LED_TASK_NAME "LEDC_TASK"

static TaskHandle_t ledc_taskhandle;

static StackType_t led_task_stack_mem[LED_TASK_STACK_DEPTH];
static StaticTask_t led_task_tcb;

static void ledc_command_handler_task(void *args);

// --------------------------- led task will run on core 0---------------------------------------------

static TaskHandle_t caller_task_handle;
// --------------------------- led task will run on core 0---------------------------------------------

/**
 * @brief Timer alarm callback prototype
 *
 * @param[in] timer Timer handle created by `gptimer_new_timer()`
 * @param[in] edata Alarm event data, fed by driver
 * @param[in] user_ctx User data, passed from `gptimer_register_event_callbacks()`
 * @return Whether a high priority task has been waken up by this function
 */
static IRAM_ATTR bool Led_timer_evt_callback(gptimer_handle_t timer_handle, const gptimer_alarm_event_data_t *alarm_evt, void *user_data)
{
    portBASE_TYPE high_task_woken = pdFALSE;
    // only send notif to the task
    xTaskNotifyFromISR(ledc_taskhandle, LED_NOTIF_TIMER_EXPIRES, eSetBits, &high_task_woken);

    ///// stop the timer
    gptimer_stop(led_timer_handle);
    // reset the counter
    gptimer_set_raw_count(led_timer_handle, 0);

    return high_task_woken;
}

/// @brief ledc isr callback
/// @param param
static IRAM_ATTR bool ledc_fade_callback(const ledc_cb_param_t *param, void *uesr_args)
{
    portBASE_TYPE high_task_awoken = pdFALSE;
    if (param->event == LEDC_FADE_END_EVT)
    {
        led_fader_nums--;
        if (led_fader_nums == 0)
        {
            led_fader_nums = MAX_NO_LEDS;

            if (curr_state.request_to_idle)
            {
                // send the lock from the callback
                xSemaphoreGiveFromISR(ledc_semphr_handle, &high_task_awoken);
                // request to idle satisfied
                curr_state.request_to_idle = clear;
            }
            else
            {
                // send the notif to main task
                xTaskNotifyFromISR(ledc_taskhandle, LED_NOTIF_FADE_EXPIRES, eSetBits, &high_task_awoken);
            }
        }
    }
    return high_task_awoken;
}

/// @brief initialise the led driver
/// @param  void
void led_driver_init(void)
{
    ESP_LOGW(TAG, "led drv init");
    // configure the timer
    static const ledc_timer_config_t led_timer_config = {
        .timer_num = LEDC_TIMER,
        .duty_resolution = LED_TIMER_RESOLUTION,
        .clk_cfg = LEDC_AUTO_CLK,
        .freq_hz = LED_DRIVER_FREQ,
        .speed_mode = LEDC_MODE,
    };
    esp_err_t err = ledc_timer_config(&led_timer_config);
    assert((err == 0));

    // configure the channel
    static const ledc_channel_config_t led0_channel_config = {
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

    static const ledc_channel_config_t led1_channel_config = {
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

    static const ledc_channel_config_t led2_channel_config = {
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

    // create a biary semaphore
    ledc_semphr_handle = xSemaphoreCreateBinaryStatic(&static_semaphore_control_block);
    assert(ledc_semphr_handle != NULL);
    xSemaphoreGive(ledc_semphr_handle);

    // create the static queue of size ledc_cmd_Q size
    ledc_cmd_q_handle = xQueueCreateStatic(LEDC_CMD_Q_LEN, LEDC_CMD_Q_ITEM_SIZE, ledc_cmd_q_memory, &ledc_cmd_q_control_block);
    assert(ledc_cmd_q_handle != NULL);
    xQueueReset(ledc_cmd_q_handle);

    // ------------------------------------ timer for blinking operations -----------------------------------------------------
    // create a gptimer
    static const gptimer_config_t ledc_timer_config = {.clk_src = GPTIMER_CLK_SRC_APB,
                                                       .direction = GPTIMER_COUNT_UP,
                                                       .resolution_hz = LED_TIMER_FREQ};

    err = gptimer_new_timer(&ledc_timer_config, &led_timer_handle);
    assert((err == 0));
    gptimer_set_raw_count(led_timer_handle, 0);

    // config the timer callback
    static const gptimer_event_callbacks_t gp_timer_evt_cb = {.on_alarm = Led_timer_evt_callback};
    gptimer_register_event_callbacks(led_timer_handle, &gp_timer_evt_cb, NULL);

    gptimer_enable(led_timer_handle);

    // create the ledc task
    ledc_taskhandle = xTaskCreateStaticPinnedToCore(ledc_command_handler_task,
                                                    LED_TASK_NAME,
                                                    LED_TASK_STACK_DEPTH,
                                                    LED_TASK_PARAM,
                                                    LED_TASK_PRIORITY,
                                                    led_task_stack_mem,
                                                    &led_task_tcb,
                                                    LED_TASK_CPU);
    // failed assert if null handle
    assert(ledc_taskhandle != NULL);

    // configure the fade interrupts
    ledc_fade_func_install(ESP_INTR_FLAG_IRAM);

    ledc_cbs_t callback = {.fade_cb = ledc_fade_callback};
    ledc_cb_register(LEDC_MODE, RED_COLOR_CHANNEL, &callback, NULL);
    ledc_cb_register(LEDC_MODE, GREEN_COLOR_CHANNEL, &callback, NULL);
    ledc_cb_register(LEDC_MODE, BLUE_COLOR_CHANNEL, &callback, NULL);
}

/// @brief deinit the led driver
/// @param  void
void led_driver_deinit(void)
{
    ESP_LOGW(TAG, "deinit led driver ");
    // first suspend and delete the task
    vTaskSuspend(ledc_taskhandle);
    vTaskDelete(ledc_taskhandle);

    // delete the q handle
    vQueueDelete(ledc_cmd_q_handle);

    // delete the semaphore handle
    vSemaphoreDelete(ledc_semphr_handle);

    // disable and delete the gptimer
    gptimer_disable(led_timer_handle);
    gptimer_del_timer(led_timer_handle);

    ledc_fade_func_uninstall();
    // disable and delete the LEDC driver
    ledc_timer_pause(LEDC_MODE, LEDC_TIMER);
    ledc_timer_rst(LEDC_MODE, LEDC_TIMER);

    // make gpio as input
    gpio_set_direction(RED_COLOR_PIN, GPIO_MODE_INPUT);
    gpio_set_direction(GREEN_COLOR_PIN, GPIO_MODE_INPUT);
    gpio_set_direction(BLUE_COLOR_PIN, GPIO_MODE_INPUT);
}

/// @brief this registered callback called when the operation is complete (off time expires)
/// @param  callback
/// @param  user_argument
void led_driver_register_cmpt_callback(ledc_cmpt_callback callback, void *args)
{
    task_cmpt_callb = callback;
    callback_argument = args;
}

/// @brief remove the complete callback from the led driver
/// @param  void
void led_driver_remove_cmpt_callback(void)
{
    task_cmpt_callb = NULL;
}

/// @brief this api would stop all the outgoing operations on led
/// @param  void
void led_driver_stop_all_operations(void)
{
    // check the driver state
    if (curr_state.current_state != LEDC_DRV_STATE_IDLE)
    {
        // send notif to stop all things
        xTaskNotify(ledc_taskhandle, LED_NOTIF_API_ENFORCES, eSetValueWithOverwrite);
    }
}

/// @brief this will wait for the completion of the operations
/// @param  time
/// @return succ/err
esp_err_t led_driver_wait_for_completion(uint32_t time)
{
    caller_task_handle = xTaskGetCurrentTaskHandle();
    return xTaskNotifyWait(0, 0, NULL, pdMS_TO_TICKS(time));
}

/// @brief put the color on the led
/// @param color
esp_err_t led_driver_put_color(led_color_struct_t color, led_time_config_struct_t time)
{

    if (time.off_time < LED_TIMER_MIN_BLINK_TIME || color.red > MAX_COLOR_INTENSITY || color.blue > MAX_COLOR_INTENSITY ||
        color.green > MAX_COLOR_INTENSITY)
    {
        ESP_LOGE(TAG, "invalid color");
        return ERR_SYS_INVALID_PARAM;
    }
    // copy the color
    ledc_cmd_q_struct_t drv_cmd = {.cmd = LEDC_CMD_COLOR, .color = color, .time = time};
    // send the command to the q
    xQueueOverwrite(ledc_cmd_q_handle, &drv_cmd);

    // check the driver state
    if (curr_state.current_state != LEDC_DRV_STATE_IDLE)
    {
        // send notif to stop all things
        xTaskNotify(ledc_taskhandle, LED_NOTIF_API_ENFORCES, eSetValueWithOverwrite);
    }
    return ESP_OK;
}

/// @brief no color on the led
/// @param
esp_err_t led_driver_no_color(void)
{
    // copy the color
    ledc_cmd_q_struct_t drv_cmd = {.cmd = LEDC_CMD_IDLE, .color = {0}, .time = {0}};
    // send the command to the q
    xQueueOverwrite(ledc_cmd_q_handle, &drv_cmd);

    // check the driver state
    if (curr_state.current_state != LEDC_DRV_STATE_IDLE)
    {
        // send notif to stop all things
        xTaskNotify(ledc_taskhandle, LED_NOTIF_API_ENFORCES, eSetValueWithOverwrite);
    }

    return ESP_OK;
}

/// @brief blink the led with the blink time and off time
/// @param color
/// @param time
/// @return succ/err
esp_err_t led_driver_blink_color(led_color_struct_t color, led_time_config_struct_t time)
{
    // ------------- led is blinking using fade in and fade out as blink
    if (time.fade_in_time < LED_TIMER_MIN_BLINK_TIME || time.fade_out_time < LED_TIMER_MIN_BLINK_TIME || color.red > MAX_COLOR_INTENSITY || color.blue > MAX_COLOR_INTENSITY ||
        color.green > MAX_COLOR_INTENSITY)
    {
        ESP_LOGE(TAG, "invalid color, blink time > %d msec", LED_TIMER_MIN_BLINK_TIME);
        return ERR_SYS_INVALID_PARAM;
    }
    // copy the color
    ledc_cmd_q_struct_t drv_cmd = {.cmd = LEDC_CMD_BLINK, .color = color, .time = time};

    // send the command to the q
    xQueueOverwrite(ledc_cmd_q_handle, &drv_cmd);

    // check the driver state
    if (curr_state.current_state != LEDC_DRV_STATE_IDLE)
    {
        // send notif to stop all things
        xTaskNotify(ledc_taskhandle, LED_NOTIF_API_ENFORCES, eSetValueWithOverwrite);
    }
    return ESP_OK;
}

/// @brief this is to fade the color on the led with this configuration
/// @param color
/// @param time
/// @return succ/err code
esp_err_t led_driver_fade_color(led_color_struct_t color, led_time_config_struct_t time)
{
    if (time.blink_time < LED_TIMER_MIN_BLINK_TIME || color.red > MAX_COLOR_INTENSITY || color.blue > MAX_COLOR_INTENSITY ||
        color.green > MAX_COLOR_INTENSITY)
    {
        ESP_LOGE(TAG, "invalid color, blink time > %d msec", LED_TIMER_MIN_BLINK_TIME);
        return ERR_SYS_INVALID_PARAM;
    }
    // copy the color
    ledc_cmd_q_struct_t drv_cmd = {.cmd = LEDC_CMD_FADER, .color = color, .time = time};
    // send the command to the q
    xQueueOverwrite(ledc_cmd_q_handle, &drv_cmd);

    // check the driver state
    if (curr_state.current_state != LEDC_DRV_STATE_IDLE)
    {
        // send notif to stop all things
        xTaskNotify(ledc_taskhandle, LED_NOTIF_API_ENFORCES, eSetValueWithOverwrite);
    }

    return ESP_OK;
}

/****
 * This is the led command handler task that is used to handle the request from apis
 * this will make sure that the hardware will not be in a invalid state or inproper state
 * it will make sure that the faders will be reseted when asked by the driver
 */

static void ledc_command_handler_task(void *args)
{
    led_fader_nums = 0;

    // some initing task

    ledc_stop(LEDC_MODE, RED_COLOR_CHANNEL, 0);
    ledc_stop(LEDC_MODE, BLUE_COLOR_CHANNEL, 0);
    ledc_stop(LEDC_MODE, GREEN_COLOR_CHANNEL, 0);
    curr_state.current_state = LEDC_DRV_STATE_IDLE;

    ledc_cmd_q_struct_t ledc_cmd = {0};
    for (;;)
    {
    // ------------ run to idle goto for goiing to idle state
    run_to_idle:
        // waiting for the Queue to get the data
        xQueueReceive(ledc_cmd_q_handle, &ledc_cmd, portMAX_DELAY);

        // after reciving the cmd , make sure that the state is not fader, if yes then disable the fader
        if (curr_state.current_state == LEDC_DRV_STATE_UP || curr_state.current_state == LEDC_DRV_STATE_DOWN)
        {
            curr_state.request_to_idle = set;
            // wait for the lock release by the fader from the callback
            xSemaphoreTake(ledc_semphr_handle, portMAX_DELAY);
        }

        ESP_LOGI(TAG, "cmd %d, color %d,%d,%d, time %lu,%lu,%lu,%lu", ledc_cmd.cmd, ledc_cmd.color.red, ledc_cmd.color.green, ledc_cmd.color.blue,
                 ledc_cmd.time.fade_in_time, ledc_cmd.time.fade_out_time, ledc_cmd.time.blink_time, ledc_cmd.time.off_time);

        switch (ledc_cmd.cmd)
        {
        case LEDC_CMD_FADER:
        {
            led_fader_nums = MAX_NO_LEDS;
            // config alarm time for blinking
            gptimer_alarm_config_t alarm_config = {.alarm_count = GET_ALARM_FROM_MSEC(ledc_cmd.time.blink_time)};
            gptimer_set_alarm_action(led_timer_handle, &alarm_config);
            // restart the timer
            gptimer_set_raw_count(led_timer_handle, 0);
            gptimer_start(led_timer_handle);

            curr_state.current_state = LEDC_DRV_STATE_OFF;
        }
        break;
        case LEDC_CMD_BLINK:
        {
            // config alarm time for blinking
            gptimer_alarm_config_t alarm_config = {.alarm_count = GET_ALARM_FROM_MSEC(ledc_cmd.time.fade_in_time)};
            gptimer_set_alarm_action(led_timer_handle, &alarm_config);
            // restart the timer
            gptimer_set_raw_count(led_timer_handle, 0);
            gptimer_start(led_timer_handle);

            curr_state.current_state = LEDC_DRV_STATE_OFF;
        }

        break;
        case LEDC_CMD_COLOR:
        {
            ledc_set_duty(LEDC_MODE, RED_COLOR_CHANNEL, ledc_cmd.color.red);
            ledc_set_duty(LEDC_MODE, GREEN_COLOR_CHANNEL, ledc_cmd.color.green);
            ledc_set_duty(LEDC_MODE, BLUE_COLOR_CHANNEL, ledc_cmd.color.blue);

            ledc_update_duty(LEDC_MODE, RED_COLOR_CHANNEL);
            ledc_update_duty(LEDC_MODE, GREEN_COLOR_CHANNEL);
            ledc_update_duty(LEDC_MODE, BLUE_COLOR_CHANNEL);

            // config alarm time forturn off
            gptimer_alarm_config_t alarm_config = {.alarm_count = GET_ALARM_FROM_MSEC(ledc_cmd.time.off_time)};
            gptimer_set_alarm_action(led_timer_handle, &alarm_config);
            // restart the timer
            gptimer_set_raw_count(led_timer_handle, 0);
            gptimer_start(led_timer_handle);

            curr_state.current_state = LEDC_DRV_STATE_SOLID;
        }
        break;

        case LEDC_CMD_OFF:
        case LEDC_CMD_IDLE:
        {
            goto reset_state;
        }
        break;

        case LEDC_CMD_UNDEF:
        default:
            ESP_LOGE(TAG, "doesnt't support cmd %d", ledc_cmd.cmd);
            goto run_to_idle;
            break;
        }

        // reset the notification
        xTaskNotifyWait(0xff, 0xff, NULL, 0);
        // reset any configuration before moving to wait state
        curr_state.current_time = 0;
        curr_state.request_to_idle = clear;

        /****** here the task will be wait for notification from the hardware source or the api  */
    wait_for_notif:
        uint32_t notif_val = 0;
        xTaskNotifyWait(0, UINT32_MAX, &notif_val, portMAX_DELAY);

        // now switch to different notif states
        switch (notif_val)
        {
        // timer is resetted
        case LED_NOTIF_TIMER_EXPIRES:
        {
            //// check if current time reaches the off time
            if (curr_state.current_time >= ledc_cmd.time.off_time)
            {
                goto reset_state;
            }
            else if (ledc_cmd.cmd == LEDC_CMD_COLOR)
            {
                // we arrive here because timer timeout occurs for off time of solid color
                goto reset_state;
            }
            // if there is a blink cmd then
            else if (ledc_cmd.cmd == LEDC_CMD_BLINK)
            {
                // if blink then check current state
                if (curr_state.current_state == LEDC_DRV_STATE_OFF)
                {
                    // turn on led
                    ledc_set_duty(LEDC_MODE, RED_COLOR_CHANNEL, ledc_cmd.color.red);
                    ledc_set_duty(LEDC_MODE, GREEN_COLOR_CHANNEL, ledc_cmd.color.green);
                    ledc_set_duty(LEDC_MODE, BLUE_COLOR_CHANNEL, ledc_cmd.color.blue);

                    ledc_update_duty(LEDC_MODE, RED_COLOR_CHANNEL);
                    ledc_update_duty(LEDC_MODE, GREEN_COLOR_CHANNEL);
                    ledc_update_duty(LEDC_MODE, BLUE_COLOR_CHANNEL);

                    curr_state.current_state = LEDC_DRV_STATE_SOLID;
                    // increment the tick
                    curr_state.current_time += ledc_cmd.time.fade_in_time;
                    gptimer_alarm_config_t alarm_config = {.alarm_count = GET_ALARM_FROM_MSEC(ledc_cmd.time.fade_out_time)};
                    gptimer_set_alarm_action(led_timer_handle, &alarm_config);
                }
                else // it is in solid state
                {
                    // turn off led
                    ledc_stop(LEDC_MODE, RED_COLOR_CHANNEL, 0);
                    ledc_stop(LEDC_MODE, BLUE_COLOR_CHANNEL, 0);
                    ledc_stop(LEDC_MODE, GREEN_COLOR_CHANNEL, 0);

                    curr_state.current_state = LEDC_DRV_STATE_OFF;
                    // increment the tick
                    curr_state.current_time += ledc_cmd.time.fade_out_time;

                    gptimer_alarm_config_t alarm_config = {.alarm_count = GET_ALARM_FROM_MSEC(ledc_cmd.time.fade_in_time)};
                    gptimer_set_alarm_action(led_timer_handle, &alarm_config);
                }
                /// in fader cmd is rcvd
            }
            else if (ledc_cmd.cmd == LEDC_CMD_FADER)
            {
                ESP_LOGI(TAG, "fade expires");
                // again start the fading procedure
                ledc_set_fade_with_time(LEDC_MODE, RED_COLOR_CHANNEL, ledc_cmd.color.red, ledc_cmd.time.fade_in_time);
                ledc_set_fade_with_time(LEDC_MODE, GREEN_COLOR_CHANNEL, ledc_cmd.color.green, ledc_cmd.time.fade_in_time);
                ledc_set_fade_with_time(LEDC_MODE, BLUE_COLOR_CHANNEL, ledc_cmd.color.blue, ledc_cmd.time.fade_in_time);

                ledc_fade_start(LEDC_MODE, RED_COLOR_CHANNEL, LEDC_FADE_NO_WAIT);
                ledc_fade_start(LEDC_MODE, GREEN_COLOR_CHANNEL, LEDC_FADE_NO_WAIT);
                ledc_fade_start(LEDC_MODE, BLUE_COLOR_CHANNEL, LEDC_FADE_NO_WAIT);

                // now agaon the driver will go into up state
                curr_state.current_state = LEDC_DRV_STATE_UP;
                // increment the tick
                curr_state.current_time += ledc_cmd.time.blink_time;

                // we never start timer again
                goto wait_for_notif;
            }

            /// start the timer
            gptimer_start(led_timer_handle);

            // wait for notif
            goto wait_for_notif;
        }
        break;

        case LED_NOTIF_FADE_EXPIRES:
        {
            // check which fade the driver is in
            if (curr_state.current_state == LEDC_DRV_STATE_UP)
            {
                ESP_LOGI(TAG, "fade up expires");
                // switch to down state
                ledc_set_fade_with_time(LEDC_MODE, RED_COLOR_CHANNEL, 0, ledc_cmd.time.fade_out_time);
                ledc_set_fade_with_time(LEDC_MODE, GREEN_COLOR_CHANNEL, 0, ledc_cmd.time.fade_out_time);
                ledc_set_fade_with_time(LEDC_MODE, BLUE_COLOR_CHANNEL, 0, ledc_cmd.time.fade_out_time);

                ledc_fade_start(LEDC_MODE, RED_COLOR_CHANNEL, LEDC_FADE_NO_WAIT);
                ledc_fade_start(LEDC_MODE, GREEN_COLOR_CHANNEL, LEDC_FADE_NO_WAIT);
                ledc_fade_start(LEDC_MODE, BLUE_COLOR_CHANNEL, LEDC_FADE_NO_WAIT);

                curr_state.current_state = LEDC_DRV_STATE_DOWN;
                curr_state.current_time += ledc_cmd.time.fade_in_time;
            }
            else /// driver is in fade down state
            {
                ESP_LOGI(TAG, "fade down expires");
                // switch to up state and config blink timer
                gptimer_alarm_config_t alarm_config = {.alarm_count = GET_ALARM_FROM_MSEC(ledc_cmd.time.blink_time)};
                gptimer_set_alarm_action(led_timer_handle, &alarm_config);
                // set the timer for blink
                gptimer_set_raw_count(led_timer_handle, 0);
                gptimer_start(led_timer_handle);

                curr_state.current_state = LEDC_DRV_STATE_OFF;
                curr_state.current_time += ledc_cmd.time.fade_out_time;
            }

            goto wait_for_notif;
        }
        break;

        case LED_NOTIF_API_ENFORCES:
        {

            // shut down timer and
            gptimer_stop(led_timer_handle);
            // here we can't stop the process immediately, go to idle state
            goto run_to_idle;
        }
        break;

        case LED_NOTIF_UNDEF:
        case LED_NOTIF_NONE:
        default:
        {
        }
        break;
        }

    // this is the reset state for the driver
    reset_state:
        ESP_LOGW(TAG, "reseting the led driver");

        // also call the callback
        if (task_cmpt_callb != NULL)
        {
            task_cmpt_callb(callback_argument);
        }
        // turn off the leds
        ledc_stop(LEDC_MODE, RED_COLOR_CHANNEL, 0);
        ledc_stop(LEDC_MODE, BLUE_COLOR_CHANNEL, 0);
        ledc_stop(LEDC_MODE, GREEN_COLOR_CHANNEL, 0);
        curr_state.current_state = LEDC_DRV_STATE_IDLE;

        // inform other task who's waiting for val
        if (caller_task_handle != NULL)
        {
            xTaskNotify(caller_task_handle, 0, eSetBits);
            // automatically cleat the handle
            caller_task_handle = NULL;
        }
    }
    // delet the task itself
    vTaskDelete(NULL);
}