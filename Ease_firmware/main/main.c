
#include "fuel_guage.h"
#include "system.h"
#include "nvs_lib.h"
#include "ble.h"
#include "ads.h"
#include "tdcs.h"

///////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////// function prototype

#define set   1
#define reset 0

#define ESP_BATT_CRITICAL_SOC 2
//////// structure to store the tdcs message content format

void error_hand(void);

///+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++/////
///++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++/////
/// general task
void generaltask(void*);

#define FX_GEN                   "gen_task"
#define general_task_stack_depth 2048

//// these are the memory and the tcb (task control block ) for the general task
static StackType_t genral_task_stack_mem[general_task_stack_depth];
static StaticTask_t genral_task_tcb;

TaskHandle_t general_tsk_handle = NULL;

///+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++/////
///++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++/////
/////////// function that handle the tdcs task
void function_tdcs_task(void*);
////// task name strings
#define FX_TDCS               "func_tdcs"
#define tdcs_task_stack_depth 2048

static TaskHandle_t tdcs_task_handle = NULL;

/////////// function that handle the eeg task
void function_eeg_task(void*);
#define FX_EEG               "func_eeg"
#define eeg_task_stack_depth 2048

static TaskHandle_t eeg_task_handle = NULL;

///+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++/////
///++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++/////
/////////// function to handle idle wait time
void function_waiting_task(void*);

#define idle_wait_task_stack_depth 2048
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

///////// check if we are testing firmware or not
#if !defined TEST

static volatile uint32_t err_code = 0;
static volatile bool send_idle_state = false;

static volatile uint8_t device_state = BLE_DISCONNECTED;

static volatile uint8_t device_color = NO_COLOR;

// static volatile uint8_t dev_err_code  = error_none ;

void app_main(void)
{

    //// initializing all the harware and init functions
    //////// init the system here
    system_init();
    ////////// init the ads spi bus
    ads_spi_bus_init();

    //// init the tdcs spi bus
    tdcs_bus_init();
    //// init the fuel gauge bus
    fuel_gauge_init();
    // fuel init done

    printf("err val %lx\r\n", err_code);
    /////////init the ble module
    ble_init();

    printf("starting app\r\n");
    ///////// create the general task that will handle the communication and creaate new task
    general_tsk_handle = xTaskCreateStaticPinnedToCore(generaltask,
                                                       FX_GEN,
                                                       general_task_stack_depth,
                                                       NULL,
                                                       PRIORTY_4,
                                                       genral_task_stack_mem,
                                                       &genral_task_tcb,
                                                       app_cpu);
    assert((general_tsk_handle != NULL));

    waiting_task_handle = xTaskCreateStaticPinnedToCore(function_waiting_task,
                                                        FX_WAITING,
                                                        idle_wait_task_stack_depth,
                                                        &device_state,
                                                        PRIORTY_1,
                                                        waiting_task_stack_mem,
                                                        &waiting_task_tcb,
                                                        app_cpu);
    assert((waiting_task_handle != NULL));

    tdcs_timer_handle =
      xTimerCreateStatic(TDCS_TIMER_NAME, 1, TDCS_TIMER_AUTORELOAD, (void*) 0, tdcs_timer_task_Callback, &timer_mem_buffer);
    assert((tdcs_timer_handle != NULL));
    /// creating the timer
    // vTaskSuspend(waiting_task_handle);
    // vTaskSuspend(handle1);
    // printf("the free heap memory is %d\r\n", esp_get_free_heap_size());

    vTaskSuspend(NULL);
    while (1)
    {
        delay(1000);
    }

    //////////// test the fuel gauge ic in ble mode

    return;
}

///////////// this function will return the bios test result to the app when connected

uint32_t device_run_bios_test(void)
{
    uint32_t err = 0;
    /// start the bios testing
    uint8_t ret = ESP_OK;
    ret = ads_verify_process();
    err |= (ret << EEG_ERR_POSITION);

    ret = tdcs_verify_process();
    err |= (ret << TDCS_ERR_POSITION);

    ret = fuel_gauge_verify_process();
    err |= (ret << FUEL_GAUGE_ERR_POSITION);

    printf("bios teset err %lx\r\n", err);
    return err;
}

void generaltask(void* param)
{
    uint32_t notf_val = BLE_DISCONNECTED;

    /// structure for the tdcs
    static funct_tdcs_data tdcs_data = {0};

    static func_eeg_task eeg_data = {0};
    ///// while loop
    for (;;)
    {
        //// prase the notification value
        device_state = notf_val;

        if (notf_val == RUN_TDCS)
        {
            /// suspend the task
            vTaskSuspend(waiting_task_handle);
            printf("tdcs_command \r\n");
            ////////// if notification recieve then check the message buffer to get the data .
            ///// data have a particular format so the opcode, amplitude, freq, etc get extracted from the recv msg
            if ((tdcs_task_handle == NULL) && (eeg_task_handle == NULL))
            {
                if (!is_msgbuff_empty())
                {
                    /// this local var will not destroy as it is in stack and refer to the same variable
                    pop_msg_buff(u8(tdcs_data), s(tdcs_data));

                    tdcs_data.stop_type = tac_abort;
                    xTaskCreatePinnedToCore(function_tdcs_task,
                                            FX_TDCS,
                                            tdcs_task_stack_depth,
                                            &tdcs_data,
                                            PRIORTY_3,
                                            &tdcs_task_handle,
                                            app_cpu);
                }
            }
        } else if (notf_val == RUN_EEG)
        {
            printf("eeg command \r\n");
            /// suspend the task
            vTaskSuspend(waiting_task_handle);
            if ((tdcs_task_handle == NULL) && (eeg_task_handle == NULL))
            {
                if (!is_msgbuff_empty())
                {
                    pop_msg_buff(u8(eeg_data), s(eeg_data));
                    xTaskCreatePinnedToCore(function_eeg_task,
                                            FX_EEG,
                                            eeg_task_stack_depth,
                                            &eeg_data,
                                            PRIORTY_3,
                                            &eeg_task_handle,
                                            app_cpu);
                }
            }
        } else if (notf_val == STOP)
        {

            if (tdcs_task_handle != NULL)
            {
                //// check that what the status buffer have
                funct_tdcs_data temp = {0};
                if (!is_msgbuff_empty())
                {
                    pop_msg_buff(u8(temp), s(temp));
                }
                tdcs_data.stop_type = temp.opcode;
            }

            ////////// clear the message buffer

            ////// send the status code that 0 is idle
            // send_stats_code(status_idle);
            // esp_ble_send_status_indication(status_idle);
            printf("stop command\r\n");

            // //// resume the waiting task
            // vTaskResume(waiting_task_handle);
        } else if (notf_val == BLE_DISCONNECTED)
        {
            printf("disconnected\r\n");

            //// function automatically destroy themselves
            tdcs_data.stop_type = tac_abort;
        } else if (notf_val == BLE_CONNECTED)
        {
            printf("connected \r\n");
            // ///// check if the backgroud process  are done or not

            send_stats_code(status_idle);
            send_err_code(device_run_bios_test());
            err_code = 0;
        }
        reset_msg_buffer();
        // notf_val = IDLE;

        ////// wait for the notification to recieve
        xTaskNotifyWait(0x00, 0xff, &notf_val, portMAX_DELAY);
        // reset_msg_buffer();
        ////// put no color
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
        if (*dev_connection == BLE_DISCONNECTED)
        {
            goto device_adv_state;
        } else
        {

            fuel_gauge_start_calculation();
            ///// delay here for some time
            delay(400);

            if (batt_data.soc <= ESP_BATT_CRITICAL_SOC)
            {
                uint32_t err = 0;
                err = fuel_gauge_verify_process();
                if (err == ESP_OK)
                {
                    err = err_battery_critically_low;
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
                printf("sending state %d \r\n", esp_ble_send_status_indication(status_idle));
                send_idle_state = false;
            }
            //// send the err code if not none
            if (err_code != 0)
            {
                delay(200);
                printf("sending err %x\r\n", esp_ble_send_err_indication(err_code));
                err_code = 0;
            }
        }
    }

device_adv_state:

    reset_msg_buffer();
    reset_status_q();
    reset_err_q();

    /// start the advertisement
    //// it is assumed that the device must be in the ideal state before entering here
    ble_start_advertise();

    printf("adv esp\r\n");
    prev_milli = millis();
    for (;;)
    {

        if (*dev_connection == BLE_DISCONNECTED)
        {
            //// resume the task for feul gauge calculation
            fuel_gauge_start_calculation();
            ///  //////// this is not reliable here , the delay API .make the function to exit (blocked , so 1 sec tolrence is here)
            put_color(YELLOW_COLOR);
            delay(500);
            put_color(NO_COLOR);
            delay(500);

            if ((millis() - prev_milli) > idle_state_wait_time)
            {
                ESP_NO_OF_BLINKS = 10;
                goto dev_terminate;
            }

            /// shutdown the device as battery is critically low
            if (batt_data.soc <= ESP_BATT_CRITICAL_SOC)
            {
                ESP_NO_OF_BLINKS = 150;
                goto dev_terminate;
            }

        } else
        {
            put_color(GREEN_COLOR);
            goto device_ideal_state;
        }
    }

dev_terminate:

    delay(400);
    ble_stop_advertise();
    ble_disconnect_device();
    printf("going to shut down\r\n");
    ////////// this is the
    for (uint8_t i = 0; i < ESP_NO_OF_BLINKS; i++)
    {
        put_color(RED_COLOR);
        delay(200);
        put_color(NO_COLOR);
        delay(200);
    }

    // printf("shutting down system\r\n");
    system_shutdown();
}

/// @brief run the tdcs protcols in the sandbox
/// @param param
void tdcs_timer_task_Callback(TimerHandle_t time_handle)
{

    /// use the paramter for the tdcs protocol
    funct_tdcs_data* tdcs_data = (funct_tdcs_data*) pvTimerGetTimerID(time_handle);

    switch (tdcs_data->opcode)
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
            set_Current(tdcs_data->amplitude);
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
    funct_tdcs_data* tdcs_data = param;
    /// convert the time into milliseconds
    tdcs_data->time_till_run *= 1000;

    printf("opc %d, amp %d, fre %ld,time %ld\r\n", tdcs_data->opcode, tdcs_data->amplitude, tdcs_data->frequency, tdcs_data->time_till_run);

    tdcs_init(tdcs_data->opcode, tdcs_data->amplitude, tdcs_data->frequency, tdcs_data->time_till_run);
    // read_tdc_reg();

    //////////// send the status that tdcs runs
    send_stats_code(status_tdcs_runs);
    esp_ble_send_status_indication(status_tdcs_runs);

    uint64_t prev_mili_run = millis();

    uint32_t err = 0;

    bool do_once = true;

    /// append the timer handle
    vTimerSetTimerID(tdcs_timer_handle, tdcs_data);

    xTimerChangePeriod(tdcs_timer_handle, pdMS_TO_TICKS(tdcs_get_delay_Time()), TDCS_TIMER_WAIT_TIME);

    ///////////////// put the tdcs color
    put_color(BLUE_COLOR);

    printf("tdcs hardware inited\r\n");

    /// start the timer
    xTimerStart(tdcs_timer_handle, TDCS_TIMER_WAIT_TIME);

    for (;;)
    {
        fuel_gauge_start_calculation();
        delay(50);

        err = check_tdcs_protection();
        if (err)
        {
            printf("tdcs err %ld \r\n", err);
            send_err_code(err);
            err_code = err;
            device_color = RED_COLOR;
            break;
        }

        if ((device_state == BLE_DISCONNECTED) || (device_state == STOP))
        {
            device_color = GREEN_COLOR;
            if (tdcs_data->stop_type == tac_abort)
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
        if (batt_data.charging_status == BATTERY_IS_CHARGING)
        {
            device_color = RED_COLOR;
            printf("protocol cant be run while charigin\r\n");
            send_err_code(err_battery_charging_while_protocol_running);
            err_code = err_battery_charging_while_protocol_running;
            goto return_mech;
        }

        if (batt_data.soc <= ESP_BATT_CRITICAL_SOC)
        {
            device_color = RED_COLOR;
            printf("battery critically low \r\n");
            send_err_code(err_battery_critically_low);
            err_code = err_battery_critically_low;
            goto return_mech;
        }

        /// check that is tdcs time is over
        if (((millis() - prev_mili_run) > ((tdcs_data->opcode == tac_tdcs_prot) ? (tdcs_data->time_till_run + (2 * tdcs_data->frequency))
                                                                                : (tdcs_data->time_till_run))) ||
            is_tdcs_complete())
        {
            device_color = GREEN_COLOR;
            break;
        }

    }

return_mech:

    ////////first stop the timer
    xTimerStop(tdcs_timer_handle, TDCS_TIMER_WAIT_TIME);
    //// deinit the tdcs task
    tdcs_deinit();

    put_color(device_color);

    send_stats_code(status_idle);
    send_idle_state = true;

    printf("tdcs_func_destroy\r\n");

    // reset the buffer just for saftey precuations
    reset_msg_buffer();

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
    func_eeg_task* eeg_data = param;

    printf("rate %d,time till run%ld\r\n", eeg_data->rate, eeg_data->timetill_run);
    uint8_t err = 0;
    // convert the time in milliseconds
    eeg_data->timetill_run *= 1000;

    if (eeg_data->rate >= 16)
    {
        eeg_data->rate -= 16;
        err = ads_init(eeg_data->rate, eeg_with_imp);
    } else
    {
        err = ads_init(eeg_data->rate, eeg_only);
    }

    /////////// if some error is encountered then send it to phone
    if (err != error_none)
    {
        send_err_code(err);
        err_code = err;
        device_color = RED_COLOR;
        goto return_mech;
    }
    //// send the status that eeg runs
    send_stats_code(status_eeg_runs);
    esp_ble_send_status_indication(status_eeg_runs);

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
    act_no_of_samp = eeg_data->timetill_run / samples_to_send_time;
    put_color(PURPLE_COLOR);

    // put dummy data in the buffer and then send it to

    for (;;)
    {
        // only send the required number of samples
        if (act_no_of_samp > no_of_samp)
        {

            //// make the buffer zero
            uint8_t tx_buff[items_to_club(eeg_data->rate)][ads_item_size];
            memset(tx_buff, 0, sizeof(tx_buff));

            // get_data_r2(tx_buff);
            // send_notif_eeg(tx_buff, sizeof(tx_buff));
            if (uxQueueMessagesWaiting(ads_queue) >= items_to_club(eeg_data->rate))
            {
                for (int i = 0; i < items_to_club(eeg_data->rate); i++)
                {
                    xQueueReceive(ads_queue, tx_buff[i], time_to_wait_for_ads_data);
                }
                esp_ble_send_notif_eeg(tx_buff, sizeof(tx_buff));

                no_of_samp++;
                drdy_int_trg = 1;
            } else if (uxQueueMessagesWaiting(ads_queue) < 1)
            {
                fuel_gauge_start_calculation();
                delay(30);
            }
        }
        //// check the battery charging here
        if (batt_data.charging_status == BATTERY_IS_CHARGING)
        {
            device_color = RED_COLOR;
            printf("protocol cant be run while charigin\r\n");
            send_err_code(err_battery_charging_while_protocol_running);
            err_code = err_battery_charging_while_protocol_running;
            goto return_mech;
        }

        if (batt_data.soc <= ESP_BATT_CRITICAL_SOC)
        {
            device_color = RED_COLOR;
            printf("battery critically low \r\n");
            send_err_code(err_battery_critically_low);
            err_code = err_battery_critically_low;
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
                device_color = RED_COLOR;
                printf("protocol running stopped due to drdy error \r\n");
                send_err_code(err_eeg_hardware_system_fault);
                err_code = err_eeg_hardware_system_fault;
                goto return_mech;
            }
            //// track back the millis()
            prev_milli_for_drdy = millis();
        }

        // ////// only make it complete with both the time and no_of sample
        if (((millis() - prev_milli) >= eeg_data->timetill_run) && (act_no_of_samp <= no_of_samp))
        {
            device_color = GREEN_COLOR;
            goto return_mech;
        }
        /// handle device disconnection

        if ((device_state == BLE_DISCONNECTED) || (device_state == STOP))
        {
            device_color = GREEN_COLOR;
            goto return_mech;
        }
    }

return_mech:

    // //////// exiting the eeg funcction
    printf("eeg_func_destroy\r\n");
    //////// reset the message buffer
    ads_deint();

    put_color(device_color);

    send_stats_code(status_idle);
    send_idle_state = true;

    reset_msg_buffer();
    ////////// reseume the waiting task
    vTaskResume(waiting_task_handle);

    //////////////// making the task handler to null and void
    eeg_task_handle = NULL;
    ///////////// delete the task when reach here
    vTaskDelete(NULL);
}

#endif

void error_hand(void)
{
    while (1)
    {
        delay(2000);
        printf("error\r\n");
    }
}
