
#include <stdlib.h>
#include "string.h"
#include "math.h"

#include "system_attr.h"

#include "driver/gpio.h"

#include "ble.h"
#include "tdcs.h"
#include "batt.h"
#include "eeg.h"
#include "led.h"
#include "esp_time.h"

///////// freertos libs
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/message_buffer.h"

/////// esp32 bare metal supportive libs
#include "esp_system.h"
#include "esp_log.h"

///////////// esp sleep library
#include "esp_sleep.h"

#include "sdkconfig.h" /// this include all the esp32 settings and configurations that are configure through menuconfig


///////////// wait tick period for queue and messages
#define wait_for_q_msg 10

#define wait_for_msg_buffer 10 /////// 10 ticks period

#define idle_state_wait_time 300000

/////// length of the queue
#define error_queue_length  1
#define error_q_item_Size   4 ///// sizeof(uint32_t)
#define status_queue_length 1
#define status_q_item_size  1 /////sizeof(uint8_t)

/////////////////////////////////////////////////////////////////
////////////////////// message buffer handlers ////////////////

#define msg_bfr_size 22

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
/***********
 * @name send_err_code
 * @param  errorcodes
 * @brief send the error code  to the queue
 * @return void
 */
void sys_send_err_code(uint32_t code)
{
    xQueueOverwrite(err_q_handle, &code);
}

/// @brief send the status code to the status queue
/// @param  status code
void sys_send_stats_code(uint8_t code)
{
    xQueueOverwrite(status_q_handle, &code);
}

/// @brief get the current no of items in the error q
/// @param  void
/// @return no of items
uint8_t sys_get_no_item_err_q(void)
{
    return (uint8_t) uxQueueMessagesWaiting(err_q_handle);
}

/// @brief get the number of items in the status queue
/// @param  void
/// @return no of items
uint8_t sys_get_no_item_stat_q(void)
{
    return (uint8_t) uxQueueMessagesWaiting(status_q_handle);
}

/// @brief get the first item from the error queue
/// @param  void
/// @return first item from the err q
uint32_t sys_pop_err_q(void)
{
    uint32_t recv = 0;
    if (xQueueReceive(err_q_handle, &recv, wait_for_q_msg) == pdPASS)
        return recv;
    return 0;
}

/// @brief get the first item from the status queue
/// @param  void
/// @return item from the stat queue
uint8_t sys_pop_stat_q(void)
{
    uint8_t recv = 0;
    if (xQueueReceive(status_q_handle, &recv, wait_for_q_msg) == pdPASS)
        return recv;
    return 0;
}

/// @brief reset the error queue
/// @param  void
void sys_reset_err_q(void)
{
    xQueueReset(err_q_handle);
}

/// @brief reset the status queue
/// @param  void
void sys_reset_status_q(void)
{
    xQueueReset(status_q_handle);
}

/// @brief reset the message buffer
/// @param  void
void sys_reset_msg_buffer(void)
{
    ////////// clear the message buffer
    xMessageBufferReset(gtsk_bfr_handle);
}

/// @brief pop the first message from the message buffer
/// @param  buff
/// @param  size
void sys_pop_msg_buff(uint8_t* buff, uint16_t size)
{
    xMessageBufferReceive(gtsk_bfr_handle, buff, size, wait_for_msg_buffer);
}

/// @brief push the message to the message buffer
/// @param buff
/// @param size
void sys_push_msg_buff(uint8_t* buff, uint16_t size)
{
    xMessageBufferSend(gtsk_bfr_handle, buff, size, wait_for_msg_buffer);
}

/// @brief check for emptiness of the message buffer
/// @param  void
/// @return true/false
bool sys_is_msgbuff_empty(void)
{
    return xMessageBufferIsEmpty(gtsk_bfr_handle);
}

void taskdelete(TaskHandle_t* handle)
{
    if (*handle != NULL)
    {
        vTaskDelete(*handle);
        *handle = NULL;
    }
}

/// @brief restart the system
/// @param  void
void system_restart(void)
{
    // check for any pending operations

    //////// deinit all the drivers
    tdcs_driver_deinit();
    eeg_driver_deinit();
    batt_driver_deinit();
    led_driver_deinit();
    esp_timer_driver_deinit();
    ble_driver_deinit();

    // resetart esp
    esp_restart();
}

/// @brief doesn't call any deinit api, just call restart 
/// @param  void 
void system_raw_restart(void)
{
    esp_restart();
}

////////////////////////// shutdown the system
void system_shutdown(void)
{
    ////////// other functions to sleep the ICS
    //////// deinit all the drivers
    tdcs_driver_deinit();
    eeg_driver_deinit();
    batt_driver_deinit();
    led_driver_deinit();
    esp_timer_driver_deinit();
    ble_driver_deinit();

    // // turn off all the wakeup domain source
    esp_sleep_pd_config(ESP_PD_DOMAIN_MAX, ESP_PD_OPTION_OFF);
    ////////// put the esp in sleep modes
    esp_deep_sleep_start();
}

void system_init(void)
{
    // install gpio isr service
    gpio_install_isr_service(ESP_INTR_FLAG_IRAM);

    //////// init all the system parameters
    create_err_queue();
    create_sts_queue();
    msg_buff_create();
}