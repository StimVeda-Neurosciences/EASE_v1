# Main file :

## Introduction

This page describes the Main file  implementation of the EASE device, basic structure. For more information please refer to the [main.c](../Ease_firmware/main/main.c)

---

### Table of Contents

- [Main system init](#main-system-init)
- [Bios testing](#bios-testing)
- [General task](#general-task)
- [Waiting task](#Waiting-task)
- [System shutdown](#system-shutdown)


---

## Main system init

The main level init would initialize all the base level modules like BLE,EEG,TDCS,BLE and create the General task,IDLE task & timer for TDCS task.

```c 
system_init();
    ////////// init the ads spi bus
    ads_spi_bus_init();

    //// init the tdcs spi bus
    tdcs_bus_init();
    //// init the fuel gauge bus
    fuel_gauge_init();
    // fuel init done

    printf("err val %x\r\n", err_code);
    /////////init the ble module
    ble_init();

    printf("starting `APP`\r\n");
    ///////// create the general task that will handle the communication and creaate new task
    general_tsk_handle = xTaskCreateStaticPinnedToCore(generaltask, FX_GEN, general_task_stack_depth, NULL, PRIORTY_4, genral_task_stack_mem, &genral_task_tcb, app_cpu);
    assert((general_tsk_handle != NULL));

    waiting_task_handle = xTaskCreateStaticPinnedToCore(function_waiting_task, FX_WAITING, idle_wait_task_stack_depth, &device_state, PRIORTY_1, waiting_task_stack_mem, &waiting_task_tcb, app_cpu);
    assert((waiting_task_handle != NULL));

    tdcs_timer_handle = xTimerCreateStatic(TDCS_TIMER_NAME, 1, TDCS_TIMER_AUTORELOAD, (void*)0, tdcs_timer_task_Callback, &timer_mem_buffer);
    assert((tdcs_timer_handle != NULL));
```

---

## Bios testing

When the Device connects with the `APP` the device performs the BIOS checking and send the report to the Err characteristcs. 
<br>
However the `APP` can force the device to reperform the BIOS testing by sending 1 to the Err characteristics and the device will store the new BIOS test result in the Err characteristics.

***before running the EEG task also the device will perform the EEG hardware check in order to be sure that EEG module is working***



### General task
--- 
The Genral task is the master task of the Whole EASE implementation. it is the actual Task that takes care of all the other task, it runs on core 1. 

the genral task has the following things listed below-
- Handles the BLE commands from the BLE tasks(core 0)
- Manages the device state and connection stage.
- Perform te BIOS testing when connected
- Manage and handles the EEG,TDCS,waiting task 
- Highest priority task running on the core 1



### Waiting task
---
The waiting task Handles the Device wait state like (device idle and advertisement state). In these two state the device doesn't have to do any major functionality. 

The Waiting task handles the Two major functionaity
- ***device advertisement*** - The device start,stop advertisement , limited advertisement time, battery tasks management 
- ***Device Idling*** - When the device is connected ,it is in IDLe state until the `APP` would run any protocol. Battery task management , low battery checkups are handled inside this tasks.

### System shutdown
--- 

The system will shutdown in many scenarios like 
- advertisement time expiry 
- Low battery situation 
- `APP` sends the command

Below is the code for shutdown process 
```c 
////////////////////////// shutdown the system
void system_shutdown(void)
{
    ////////// other functions to sleep the ICS
    //////// deinit the tdcs module
    tdcs_deinit();
    ////////// deinit the ads module
    ads_deint();
    // // turn off all the wakeup domain source 
    esp_sleep_pd_config(ESP_PD_DOMAIN_MAX,  ESP_PD_OPTION_OFF);

    // turn off the ble hardware 
    bt_controller_off();
    ////////// put the esp in sleep modes
    esp_deep_sleep_start();
}
```




