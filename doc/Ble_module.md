# Bluetooth Low-Energy :

## Introduction

This page describes the BLE implementation of the EASE device, basic structure. For more information please refer to the [ble.c](../Ease_firmware/main/sources/ble/ble.c)

---

### Table of Contents

- [BLE Connection](#ble-connection)
- [BLE UUIDs](#ble-uuids)
- [List of services](#list-of-services)
  - [Device Information](#device-information)
  - [Battery Level](#battery-level)
  - [Firmware Upgrades](#firmware-upgrades)
  - [EASE Custom Service](#Ease-custom_service)
- [BLE tasks](#ble-tasks)
- [Intertask communication](#intertask-communication)

---

## BLE Connection

When starting, the firmware starts BLE advertising. It sends small messages that can be received by any *central* device in range. This allows the device to announce its presence to other devices.

A companion application (running on a PC,) which receives this advertising packet can request a connection to the device. This connection procedure allows the 2 devices to negotiate communication parameters, security keys, etc.

When the connection is established, the companion app will discover the services of the EASE device and subscribe for notification 

---

## BLE UUIDs

When the service does not exist in the BLE specification, EASE implements custom services. Custom services are identified by a UUID, as are all BLE services. Here is how to define the UUID of custom services in EASE:

```c
 Ease custom service   :          0xff01
 TDCS data characteristics :      0xff02
 TDCS setting characteristics :   0xff03
 EEG data characteristics :       0xff04
 EEG setting characteristcs :     0xff05
 EASE error characteristics :     0xff06
 EASE status characteristics :    0xff07
```
---

## List of services

There are some standard (16 bit) BLE services that are listed by the Bluetooth SIG group. but also custom (128) bit services  
[List of standard BLE services](https://www.bluetooth.com/specifications/gatt/services/)

---

### Device information 
--- 
this Service has the following 16 bit UUID (0x180A).
The device information service holds the Device info like Serial number, manifacturing name,etc.  <br>
The Charcterics of this service are listed below 

- Serial number : ``` 0x2A25```
- Manufacturing name : `0x2A29`
- Hardware Version : `0x2A27`
- Firmware Version : `0x2A26`


### Battery Level
---
The battery Level and charging status of the device will be reading from the Battery value char - `0x2A19`. The App has to subscribe for the notification in order to get the Battery value asynchronusly or can read it direclty .<br>
The battery custom data value can be read from the Battery custom char `0xff00`. the custom value structure is shown below. only reading on this characteristics is supported.

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



### Firmware Upgrades 
---
The Firmware upgrades is supported on a 128 bit UUID custom nrf Device Firmware updates (DFU) service .

```c
 static const uint8_t dfu_service [] = {0x50, 0xea, 0xda, 0x30, 0x88, 0x83, 0xb8, 0x9f, 0x60, 0x4f, 0x15, 0xf3, 0x01, 0x00, 0x40, 0x8e};

static const uint8_t dfu_char[] = {0x50, 0xea, 0xda, 0x30, 0x88, 0x83, 0xb8, 0x9f, 0x60, 0x4f, 0x15, 0xf3, 0x01, 0x00, 0x40, 0x8e};

```
it has one Command  and one data characteristics, it does not have supported in EASE version 1.


### EASE Custom Service 
---
Ease has 1 custom service for handling of Device related operation like running of TDCS and EEG sessions. The custom service is a custom 16 bit UUID  `0xff01`. 

There are bunch of Characteristics for controling the session and Device error and status char  

The device has to subscribe for the Device error and status notification in order to recieve the Data from these char asyncially.

The device has to write commands to the TDCS command char in order to start/stop the TDCS session , similarly for EEG .

***TDCS charcteristics***
`TDCS data char  = 0xff02`
`TDCS Command char = 0xff03`

***EEG Characteristics***
`EEG data char  = 0xff04`
`EEG command char = 0xff05`

***Device Control characteristics***
`Device Error char = 0xff06`
`Device status char = 0xff07`


### BLE tasks
---
All of the BLE operations is done on core 0 and it is managed by the BLE stack and its tasks. you can assign the callbacks to the tasks for the GAP and GATT realted operations. The callbacks can then manage the data transactions betweeen the BLE device.


### Intertask communication
--- 
the BLE task manages the Communication between the device and smartphone/Laptops. but thare are other tasks too like the EEG,TDCS,General task,etc. 
the General tasks running on the core 1 and manages all the device operations and state. but this Tasks got the commands from the BLE tasks (callbacks) through the tasks notifications and message buffers.  

---

