#include "sys_attr.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include "freertos/queue.h"

#include "esp_log.h"
#include "esp_heap_caps.h"

#include "led.h"
#include "batt.h"
#include "ble.h"
#include "eeg.h"
#include "tdcs.h"
#include "esp_time.h"
#include "flash_op.h"
// ///////////////////////////////////////////////////////////////////////////////////////////////
// ////////////////////////////////////////////////////////// function prototype

static const char * TAG = "main";

#define ESP_BATT_CRITICAL_SOC 2

#define IDLE_STATE_WAIT_TIME (1000 * 60 * 2) // 2 minutes 
  //////// structure to store the tdcs message content format

typedef uint32_t (*errors)(void);

static errors bios_functions[4] = {eeg_verify_component,
    tdcs_verify_component,batt_verify_component,flash_op_get_boot_errs};


///+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++/////
///++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++/////
/// general task
void generaltask(void*);

#define FX_GEN                   "gen_task"
#define general_task_stack_depth 2048

//// these are the memory and the tcb (task control block ) for the general task
static StackType_t genral_task_stack_mem[general_task_stack_depth];
static StaticTask_t genral_task_tcb;

static TaskHandle_t general_tsk_handle = NULL;

///+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++/////
///++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++/////
/////////// function that handle the tdcs task
void function_tdcs_task(void*);
////// task name strings
#define FX_TDCS               "func_tdcs"
#define tdcs_task_stack_depth 3072

static TaskHandle_t tdcs_task_handle = NULL;

/////////// function that handle the eeg task
void function_eeg_task(void*);
#define FX_EEG               "func_eeg"
#define eeg_task_stack_depth 3072

static TaskHandle_t eeg_task_handle = NULL;

///+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++/////
///++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++/////
/////////// function to handle idle wait time
void function_waiting_task(void*);

#define idle_wait_task_stack_depth 3072 // 3kb memory
#define FX_WAITING                 "idle_wait"

static StackType_t waiting_task_stack_mem[idle_wait_task_stack_depth];

static StaticTask_t waiting_task_tcb;

static TaskHandle_t waiting_task_handle = NULL;

///+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++////
/// create a timer buffer
static StaticTimer_t timer_mem_buffer;

static TimerHandle_t tdcs_timer_handle = NULL;

#define TDCS_TIMER_NAME "TDCS_TIMER"

#define TDCS_TIMER_WAIT_TIME pdMS_TO_TICKS(40)

#define TDCS_TIMER_AUTORELOAD pdTRUE

/// @brief run the tdcs protcols in the sandbox
/// @param param
void tdcs_timer_task_Callback(TimerHandle_t);

//// time for scanning that the drdry int arrives or not
#define DRDY_INT_SCAN_TIME 3000
//////////////////////only use this if you want to test something

////////////////////////////////////////////////////////////////////////


static volatile uint32_t err_code = 0;
static volatile bool send_idle_state = false;

static volatile uint8_t device_state = DEV_STATE_BLE_DISCONNECTED;


void app_main(void)
{

    //// initializing all the harware and init functions
    //////// init the system here
    system_init();
    // esp timer driver init 
    esp_timer_driver_init();
    ////////// init the ads spi bus
    eeg_driver_init();
    //// init the tdcs spi bus
    tdcs_driver_init();
    //// init the fuel gauge bus
    batt_driver_init();
    // led driver init
    led_driver_init();
    // flash_op_driver_init
    flash_op_driver_init();
    /////////init the ble module
    ble_driver_init();

    // start the esp timer 
    esp_start_timer();


    // create the task that handle the state and action 
    /////// create the general task that will handle the communication and creaate new task
    general_tsk_handle = xTaskCreateStaticPinnedToCore(generaltask,
                                                       FX_GEN,
                                                       general_task_stack_depth,
                                                       NULL,
                                                       PRIORITY_4,
                                                       genral_task_stack_mem,
                                                       &genral_task_tcb,
                                                       APP_CPU);
    assert((general_tsk_handle != NULL));

    waiting_task_handle = xTaskCreateStaticPinnedToCore(function_waiting_task,
                                                        FX_WAITING,
                                                        idle_wait_task_stack_depth,
                                                        (void *) &device_state,
                                                        PRIORITY_1,
                                                        waiting_task_stack_mem,
                                                        &waiting_task_tcb,
                                                        APP_CPU);
    assert((waiting_task_handle != NULL));

    tdcs_timer_handle =  xTimerCreateStatic(TDCS_TIMER_NAME, 1, TDCS_TIMER_AUTORELOAD, (void*) 0, tdcs_timer_task_Callback, &timer_mem_buffer);
    assert((tdcs_timer_handle != NULL));
    // / creating the timer

    ESP_LOGI(TAG,"the free heap memory is %ld", heap_caps_get_free_size(MALLOC_CAP_DEFAULT));

    // the general task start immediately, and its handle doesn't init at that moment 
    ble_start_communication(general_tsk_handle);

    // stop the watchdog timer 
    esp_stop_bootloader_watchdog();
    // suspend itself
    vTaskSuspend(NULL);
}

// ///////////// this function will return the bios test result to the app when connected

esp_err_t device_run_bios_test(void)
{
    esp_err_t 
    err = eeg_verify_component() + tdcs_verify_component()
    +  batt_verify_component() + flash_op_get_boot_errs(); 
    if (err != 0)
    {
        ESP_LOGE(TAG, "Bios err %x", err);
    }
    return err;
}


esp_err_t device_get_bios_err(uint32_t arr[],uint8_t num)
{
 if(num>SYS_ERRORS_MAX_NUMS || arr ==NULL)
 {
    return ERR_SYS_INVALID_PARAM;
 }   
    for(int i=0; i<num; i++)
    {
        arr[i] = bios_functions[i]();
    }

return ESP_OK;
}

void generaltask(void* param)
{
    uint32_t notf_val = DEV_STATE_BLE_DISCONNECTED;

    /// structure for the tdcs
    tdcs_cmd_struct_t tdcs_cmd = {0};
    eeg_cmd_struct_t eeg_cmd = {0};
    ///// while loop
    for (;;)
    {
        //// prase the notification value
        device_state = notf_val;

        if (notf_val == DEV_STATE_RUN_TDCS)
        {
            /// suspend the task
            vTaskSuspend(waiting_task_handle);
            ESP_LOGI(TAG,"tdcs_command");
            ////////// if notification recieve then check the message buffer to get the data .
            ///// data have a particular format so the opcode, amplitude, freq, etc get extracted from the recv msg
            if ((tdcs_task_handle == NULL) && (eeg_task_handle == NULL))
            {
                if (!sys_is_msgbuff_empty())
                {
                    /// this local var will not destroy as it is in stack and refer to the same variable
                    sys_pop_msg_buff(u8_ptr(tdcs_cmd), sizeof(tdcs_cmd));

                    tdcs_cmd.stop_type = tac_abort;
                    xTaskCreatePinnedToCore(function_tdcs_task,
                                            FX_TDCS,
                                            tdcs_task_stack_depth,
                                            &tdcs_cmd,
                                            PRIORITY_3,
                                            &tdcs_task_handle,
                                            APP_CPU);
                }
            }
        } else if (notf_val == DEV_STATE_RUN_EEG)
        {
            ESP_LOGI(TAG,"eeg command");
            /// suspend the task
            vTaskSuspend(waiting_task_handle);
            if ((tdcs_task_handle == NULL) && (eeg_task_handle == NULL))
            {
                if (!sys_is_msgbuff_empty())
                {
                    sys_pop_msg_buff(u8_ptr(eeg_cmd), sizeof(eeg_cmd));
                    xTaskCreatePinnedToCore(function_eeg_task,
                                            FX_EEG,
                                            eeg_task_stack_depth,
                                            &eeg_cmd,
                                            PRIORITY_3,
                                            &eeg_task_handle,
                                            APP_CPU);
                }
            }
        } else if (notf_val == DEV_STATE_STOP)
        {

            if (tdcs_task_handle != NULL)
            {
                //// check that what the status buffer have
                tdcs_cmd_struct_t temp = {0};
                if (!sys_is_msgbuff_empty())
                {
                    sys_pop_msg_buff(u8_ptr(temp), sizeof(temp));
                }
                tdcs_cmd.stop_type = temp.opcode;
            }

            ////////// clear the message buffer

            ////// send the status code that 0 is idle
            // send_stats_code(status_idle);
            // esp_ble_send_status_indication(status_idle);
            ESP_LOGW(TAG,"stop command");

            // //// resume the waiting task
            // vTaskResume(waiting_task_handle);
        } else if (notf_val == DEV_STATE_BLE_DISCONNECTED)
        {
            ESP_LOGW(TAG,"DEvice disconnected ");

            //// function automatically destroy themselves
            tdcs_cmd.stop_type = tac_abort;

        } else if (notf_val == DEV_STATE_BLE_CONNECTED)
        {
            ESP_LOGW(TAG,"DEvice connected ");
            // ///// check if the backgroud process  are done or not
            led_driver_put_color(GREEN_COLOR);
            sys_send_stats_code(STATUS_IDLE);
            sys_send_err_code(device_run_bios_test());
            // set the ble error code 
            err_code = 0;
        }


        sys_reset_msg_buffer();
        // notf_val = IDLE;
        ////// wait for the notification to recieve
        xTaskNotifyWait(0x00, 0xff, &notf_val, portMAX_DELAY);
    }


    //////////// this function never reach here ///////////////////////////////
    vTaskDelete(NULL);
}

///////////////// waiting task for ble to be connected

void function_waiting_task(void* param)
{

    uint16_t ESP_NO_OF_BLINKS = 0;

    uint64_t prev_milli = millis();

    uint8_t* dev_connection = param;

    ///// @brief device is connected and in idle state
device_ideal_state:
    prev_milli = millis();
    for (;;)
    {
        if (*dev_connection == DEV_STATE_BLE_DISCONNECTED)
        {
            goto device_adv_state;
        }
        // device connected 
        else

        {
            ///// delay here for some time
            delay(400);

            if (batt_get_soc() <= ESP_BATT_CRITICAL_SOC)
            {
                uint32_t err = 0;
                err = batt_verify_component();
                if (err == ESP_OK)
                {
                    err = ERR_BATT_CRITICAL_LOW;
                }

                delay(50);
                esp_ble_send_err_indication(err);
                ESP_NO_OF_BLINKS = 150;
                goto dev_terminate;
            }

            //// send the idle state only once
            if (send_idle_state)
            {
                delay(100);
                ESP_LOGE(TAG,"sending state %d", esp_ble_send_status_indication(STATUS_IDLE));
                send_idle_state = false;
            }
            //// send the err code if not none
            if (err_code != 0)
            {
                delay(200);
                ESP_LOGE(TAG,"sending err %x", esp_ble_send_err_indication(err_code));
                err_code = 0;
            }
        }
    }

device_adv_state:

    sys_reset_msg_buffer();
    sys_reset_status_q();
    sys_reset_err_q();

    /// start the advertisement
    //// it is assumed that the device must be in the ideal state before entering here
    ble_start_advertise();

    ESP_LOGW(TAG,"advertising device");
    prev_milli = millis();
    for (;;)
    {

        if (*dev_connection == DEV_STATE_BLE_DISCONNECTED)
        {

            ///  //////// this is not reliable here , the delay API .make the function to exit (blocked , so 1 sec tolrence is here)
            led_driver_put_color(YELLOW_COLOR);
            delay(500);
            led_driver_put_color(NO_COLOR);
            delay(500);

            if ((millis() - prev_milli) > IDLE_STATE_WAIT_TIME)
            {
                ESP_NO_OF_BLINKS = 10;
                goto dev_terminate;
            }

            /// shutdown the device as battery is critically low
            if (batt_get_soc() <= ESP_BATT_CRITICAL_SOC)
            {
                ESP_NO_OF_BLINKS = 150;
                goto dev_terminate;
            }

        } else
        {
            led_driver_put_color(GREEN_COLOR);
            goto device_ideal_state;
        }
    }

dev_terminate:

    delay(400);
    ble_stop_advertise();
    ble_disconnect_device();
    ESP_LOGW(TAG,"going to shut down");
    ////////// this is the
    for (uint8_t i = 0; i < ESP_NO_OF_BLINKS; i++)
    {
        led_driver_put_color(RED_COLOR);
        delay(200);
        led_driver_put_color(NO_COLOR);
        delay(200);
    }

    system_shutdown();
}


/// @brief run the tdcs protcols in the sandbox
/// @param param
void tdcs_timer_task_Callback(TimerHandle_t time_handle)
{

    /// use the paramter for the tdcs protocol
    tdcs_cmd_struct_t* tdcs_cmd = (tdcs_cmd_struct_t*) pvTimerGetTimerID(time_handle);

    switch (tdcs_cmd->opcode)
    {
        case tac_sine_wave:

            sine_wave();
            break;

        case tac_ramp_fun_uni:
            ramp_fun_uni();
            break;

        case tac_ramp_fun_bi:
            ramp_fun_bi();
            break;

        case tac_square_bi:
            square_bi();
            break;

        case tac_square_uni:
            square_uni();
            break;

        case tac_set_current:
            // set_Current(tdcs_data->amplitude);
            break;

        case tac_tdcs_prot:
            run_tdcs();
            break;

        default:
            break;
    }
}

/// @brief ////////// function to handle the tdcs process
/// @param param funct_tdcs_data type that is given  by system task
void function_tdcs_task(void* param)
{
    tdcs_cmd_struct_t* tdcs_cmd = param;
    /// convert the time into milliseconds
    tdcs_cmd->time_till_run *= 1000;

    ESP_LOGI(TAG,"opc %d, amp %d, fre %d,time %d", tdcs_cmd->opcode, tdcs_cmd->amplitude, tdcs_cmd->frequency, tdcs_cmd->time_till_run);

    tdcs_start_prot(tdcs_cmd->opcode, tdcs_cmd->amplitude, tdcs_cmd->frequency, tdcs_cmd->time_till_run);
    // read_tdc_reg();

    //////////// send the status that tdcs runs
    sys_send_stats_code(STATUS_TDCS);
    esp_ble_send_status_indication(STATUS_TDCS);

    uint64_t prev_mili_run = millis();

    uint32_t err = 0;

    bool do_once = true;
    led_color_struct_t color ={0};

    /// append the timer handle
    vTimerSetTimerID(tdcs_timer_handle, tdcs_cmd);

    xTimerChangePeriod(tdcs_timer_handle, pdMS_TO_TICKS(tdcs_get_delay_Time()), TDCS_TIMER_WAIT_TIME);

    ///////////////// put the tdcs color
    led_driver_put_color(BLUE_COLOR);

    ESP_LOGI(TAG,"tdcs hardware inited");

    /// start the timer
    xTimerStart(tdcs_timer_handle, TDCS_TIMER_WAIT_TIME);

    for (;;)
    {

        err = check_tdcs_protection();
        if (err)
        {
            ESP_LOGE(TAG,"tdcs err %d \r\n", err);
            sys_send_err_code(err);
            err_code = err;
            color = RED_COLOR;
            break;
        }

        if ((device_state == DEV_STATE_BLE_DISCONNECTED) || (device_state == DEV_STATE_STOP))
        {
            color = GREEN_COLOR;
            if (tdcs_cmd->stop_type == tac_abort)
            {
                if (do_once)
                {
                    abort_tdcs();
                    do_once = false;
                }
            } else
            {
                goto return_mech;
            }
        }

        //// test the battery charging here
        if (batt_get_chg_status() == BATT_CHARGING)
        {
            color = RED_COLOR;
            ESP_LOGE(TAG,"protocol cant be run while charigin");
            sys_send_err_code(ERR_BATT_CHG_IN_PROTOCOL);
            err_code = ERR_BATT_CHG_IN_PROTOCOL;
            goto return_mech;
        }

        if (batt_get_soc() <= ESP_BATT_CRITICAL_SOC)
        {
            color= RED_COLOR;
            ESP_LOGE(TAG,"battery critically low");
            sys_send_err_code(ERR_BATT_CRITICAL_LOW);
            err_code = ERR_BATT_CRITICAL_LOW;
            goto return_mech;
        }

        /// check that is tdcs time is over
        if (((millis() - prev_mili_run) > ((tdcs_cmd->opcode == tac_tdcs_prot) ? (tdcs_cmd->time_till_run + (2 * tdcs_cmd->frequency))
                                                                                : (tdcs_cmd->time_till_run))) ||
            is_tdcs_complete())
        {
            color = GREEN_COLOR;
            break;
        }
    
        // give some delay 
        delay(100);
    }

return_mech:

    ////////first stop the timer
    xTimerStop(tdcs_timer_handle, TDCS_TIMER_WAIT_TIME);
    //// deinit the tdcs task
    tdcs_stop_prot();

    led_driver_put_color(color);

    sys_send_stats_code(STATUS_IDLE);
    send_idle_state = true;

    ESP_LOGW(TAG,"tdcs_func_destroy");

    // reset the buffer just for saftey precuations
    sys_reset_msg_buffer();

    ////////// reseume the waiting task
    vTaskResume(waiting_task_handle);

    /////// delete this task
    tdcs_task_handle = NULL;
    vTaskDelete(NULL);
}

/// @brief function eeg task

/// @param param
void function_eeg_task(void* param)
{
    eeg_cmd_struct_t* eeg_cmd = param;

    ESP_LOGI(TAG,"rate %d,time till run%d\r\n", eeg_cmd->rate, eeg_cmd->timetill_run);
    uint8_t err = 0;
    led_color_struct_t color = PURPLE_COLOR;

    QueueHandle_t eeg_data_q_handle= NULL;


    // convert the time in milliseconds
    eeg_cmd->timetill_run *= 1000;

    err =eeg_verify_component();
     /////////// if some error is encountered then send it to phone
    if (err != ERR_NONE)
    {
        color = RED_COLOR;
        sys_send_err_code(err);
        err_code = err;
        goto return_mech;
    }


    eeg_data_q_handle = eeg_start_reading(((eeg_cmd->rate > 16)?(eeg_cmd->rate -16):(eeg_cmd->rate)),
        ((eeg_cmd->rate > 16)?READING_EEG_WITH_IMP:READING_EEG_ONLY),eeg_task_handle);

    eeg_cmd->rate = (eeg_cmd->rate > 16)?(eeg_cmd->rate-16):(eeg_cmd->rate);

    //// send the status that eeg runs
    sys_send_stats_code(STATUS_EEG_RUN);
    esp_ble_send_status_indication(STATUS_EEG_RUN);

    ///////// init the eeg hardware
    uint64_t prev_milli = millis();

    uint64_t prev_milli_for_drdy = millis();

    /// this variable keeps track of the is drdy int arrives or not
    uint8_t drdy_int_trg = 0;
    // uint64_t prev_milli_for_chg = millis();
    //////////// put the colo

    uint64_t no_of_samp = 0;
    uint64_t act_no_of_samp;
    ///// we are sending the data at every 40 msec
    act_no_of_samp = eeg_cmd->timetill_run / GET_NO_OF_SAMPLES(EEG_DATA_SENDING_TIME,eeg_cmd->rate);

    led_driver_put_color(PURPLE_COLOR);
    

    for (;;)
    {
        // only send the required number of samples
        if (act_no_of_samp > no_of_samp)
        {   
            // check whether we have enough samples to send 
           if(uxQueueMessagesWaiting(eeg_data_q_handle) >= GET_NO_OF_SAMPLES(EEG_DATA_SENDING_TIME,eeg_cmd->rate))
            {
                uint8_t data_buff[GET_NO_OF_SAMPLES(EEG_DATA_SENDING_TIME,eeg_cmd->rate)][EEG_DATA_SAMPLE_LEN];

                // get all data in burst and send it 
                for(int i=0; i<GET_NO_OF_SAMPLES(EEG_DATA_SENDING_TIME,eeg_cmd->rate); i++)
                {
                    xQueueReceive(eeg_data_q_handle,&data_buff[i][0],0);
                }
                esp_ble_send_notif_eeg(data_buff,sizeof(data_buff));
                no_of_samp += GET_NO_OF_SAMPLES(EEG_DATA_SENDING_TIME,eeg_cmd->rate);
                drdy_int_trg =1;
            }
           
        }

        // give a small delay so that to avoid task wdt timer to trigger 
        delay(10);
        //// check the battery charging here
        if (batt_get_chg_status() == BATT_CHARGING)
        {
            color = RED_COLOR;
            ESP_LOGE(TAG,"protocol cant be run while charigin");
            sys_send_err_code(ERR_BATT_CHG_IN_PROTOCOL);
            err_code = ERR_BATT_CHG_IN_PROTOCOL;
            goto return_mech;
        }

        if (batt_get_soc() <= ESP_BATT_CRITICAL_SOC)
        {
            color = RED_COLOR;
            ESP_LOGE(TAG,"battery critically low");
            sys_send_err_code(ERR_BATT_CRITICAL_LOW);
            err_code = ERR_BATT_CRITICAL_LOW;
            goto return_mech;
        }

        ///// scan the drdy interrupt for any drdy related errors
        if ((millis() - prev_milli_for_drdy) >= DRDY_INT_SCAN_TIME)
        {
            if (drdy_int_trg == 1)
            {
                //// reset the interrupt trigger
                drdy_int_trg = 0;
            } else
            {
                color = RED_COLOR;
                ESP_LOGE(TAG,"protocol running stopped due to drdy error");
                sys_send_err_code(ERR_EEG_HARDWARE_FAULT);
                err_code = ERR_EEG_HARDWARE_FAULT;
                goto return_mech;
            }
            //// track back the millis()
            prev_milli_for_drdy = millis();
        }

        // ////// only make it complete with both the time and no_of sample
        if (((millis() - prev_milli) >= eeg_cmd->timetill_run) && (act_no_of_samp <= no_of_samp))
        {
            color = GREEN_COLOR;
            goto return_mech;
        }
        /// handle device disconnection

        if ((device_state == DEV_STATE_BLE_DISCONNECTED) || (device_state == DEV_STATE_STOP))
        {
            color = GREEN_COLOR;
            goto return_mech;
        }
    }

return_mech:

    // //////// exiting the eeg funcction
    ESP_LOGW(TAG,"eeg_func_destroy");
    //////// reset the message buffer
    eeg_stop_reading();
    led_driver_put_color(color);

    sys_send_stats_code(STATUS_IDLE);
    send_idle_state = true;

    sys_reset_msg_buffer();
    ////////// reseume the waiting task
    vTaskResume(waiting_task_handle);

    //////////////// making the task handler to null and void
    eeg_task_handle = NULL;
    ///////////// delete the task when reach here
    vTaskDelete(NULL);
}

