# Fuel_gauge module :

## Introduction

This page describes the Fuel Gauge implementation of the EASE device, basic structure. For more information please refer to [Fuel_gauge.c](../Ease_firmware/main/sources/fuel_gauge/fuel_gauge.c)

---

### Table of Contents

- [Fuel init](#Fuel-init)
- [Fuel deinit](#Fuel-deinit)
- [Features supported](#Features-suppoerted)
- [Fuel gauge params](#firmware-upgrades)
- [Fuel tasks](#Fuel-tasks)
- [Fuel gauge usage](#Fuel-gauge-usage)

---

### Fuel init
---
fuel Gauge plays an important role in the Device functioning as it would have to make sure that the battery should be in a state to power the device correctly and to send the Battery information to the application 

The battery perfomance can be monitored by the application by reading the Fuel gauge data from the BLE custom characteristics.
This would be updated by the Fuel gauge task on a regular period.

The fuel gauge should be inited for funtioning properly, first have to install the I2c driver and then init a task to run on the core 1 for getting the Battery data.

---

### Fuel deinit
---

when the Fuel gauge operation is no longer required then its module should be deinited to make the I2c bus free. the I2c deinit would free the I2c driver and its associated tasks 

---

### Features supported
---
The Fuel gauge tasks would read the Fuel guauge registers like 
Temperatuer, vcell, etc
The structure would be listed below 

```c
typedef struct __packed _FUEL_GAUGE_DATA_
{
    /// @brief this will send by notif from headband
    uint8_t soc;
    uint8_t charging_status;

    /// @brief  rest are the data that it will supply in diff char
    uint16_t V_cell;
    uint16_t rem_cap;
    uint32_t time_epty;
    int16_t curr_flow;
    int16_t temp;
    uint16_t ic_status;

    /// @brief the device id of the fuel gauge 
    uint16_t device_id;
    /// @brief the device state 
    uint8_t ease_state;

} batt_data_struct_t;
```


### Fuel gauge params
--- 
You can set some parametrs in the Fuel gauge for configuring the fuel gauge m5 algorithm . you have to refer to the datasheet for more details. you don't need to configure the m5 alogotihm every time the device boots up as the m5 saves the configuration if it is in standby state. But you must Read the datasheet properly and then configure the Ic. otherwise you get wrong results from the IC.




### Fuel tasks
---
 the Fuel gauge task is running on core 1 and it would only run once. it automatically suspend itself when reading the data from the Fuel gauge ic. this tasks has to be manually resume form other task in order to function again. the Fuel gauge tasks will records the Fuel gauge paramters and then update in a global structure in which all the task able to read the data. 
 

### Fuel gauge usage
--- 
 
 the Fuel gauge task has to be manually resume in order to get the fuel gauge data. the task which resume the Fuel gauge has to suspend itself inorder to run the Fuel Gauge task.
 The Task priority is 2 which is lowest but not from the IDLE task,


---

