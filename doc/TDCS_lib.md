# TDCS Module :

## Introduction

This page describes the TDCS implementation of the EASE device, its basic structure. For more information please refer to the [TDCS.c](../Ease_firmware/main/sources/tdcs/tdcs.c)

---

### Table of Contents

- [TDCS init](#tdcs-init)
- [TDCS deinit](#tdcs-deinit)
- [TDCS Waveforms](#tdcs-waveforms)
- [Saftey Features](#saftey-features)
- [Current monitoring](#current-monitoring)
- [BLE command structure](#ble-command-structure)
- [TDCS tasks](#TDCS-tasks)

---

## TDCS init
---
There are two types of Initialisation in TDCS 

- The TDCS has to be inited at the system initialisation, the system level init will install the SPI driver, init gpio and interrupt services. This function `void tdcs_bus_init(void)` has to be called at the System init scope.

- Before running any TDCS related function, the TDCS Hardware should be inited and in proper state.
``void tdcs_init(uint8_t waveform_type, uint16_t amplitude, uint32_t frequency,uint32_t time_till_run)``.  this init have to be called before running any tdcs waveform. 



### TDCS deinit
---
Similar to Inits there are two deinits 

- TDCS deinit have to be done at the end of the TDCS tasks. this would set the TDCS module free and turn off the module like `set the current to 0, turn off the 20v supply, turn off the interrupt`
cutt the circuit from the electrodes. 
- TDCS system deinit would uninstall the SPI driver and free the SPI bus. this is called at system shutdown process. 

---

### TDCS Waveforms
---
There are ony few waveforms that can be generated by the TDCS module These are listed below 

- **TAC Protocol**
- **TAC Ramp function unidirectional**
- **TAC Ramp function bidirectional**
- **TAC Sinewave bidirectional**
- **TAC squarewave unidirectional**
- **TAC square bidirectional**

***note- the Current feedback feature(send current sense through ble) is only supported in the TAC protocol***

---

### Saftey Features
--- 
There are various saftey features available in the TDCS from both software and herdware perspective.
- Hardware Overcurrent detection 
- Software Overcurrent detection 
- Impedance measurement 
- Headband wear off detection 
- Charging and low battery detection 
- send current measurements to the APP


### Current monitoring
---
The Current Monitoring in the TDCS protocol is done by using the ADC module of the esp32. the ADC measure the voltage of the shunt resistance and determine the Current passing through it. 
The current monitoring is then used in multiple places like

- Software Overcurrent detection 
- Impedance measurement 
- Headband wear off detection  
- send the current data to the APP


### BLE command structure
---
To actually run the TDCS, first you have to make sure that the device is in IDLE state, by reading the device state from the device state char.

After that you can run the TDCS by sending the TDCS command in the TDCS setting char. the command structure would look like this 



```c
typedef struct __packed _TDCS_TASK_
{
  /* data */
  uint8_t opcode;
  uint16_t amplitude;
  uint32_t frequency;
  uint32_t time_till_run;
  
  /// @brief check the stop type of the TDCS 
  uint8_t stop_type;

}funct_tdcs_data;
```

Basic structure 
- Opcode -> Waveform type 
- Amplitude -> Current Amplitude 
- frequency -> Frequency of the waveform, in TAC the raming time 
- time_till_run -> Running time of the waveform 
- stop type -> the STOP type of the TDCS when a STOP command is being sent 

***note-> you may choose to not send the STOP type, it is by default in Abort mode***


### TDCS tasks
---
The TDCS waveofrms and all its implementation are aggregated inside a task. This tasks is managed by the Genral tasks, there is a structure that the genral task would write to in order to command the TDCs tasks. this task also check for any error , faults while the session. it manages the High impedance detection aka Wear off detection and also check for device charging states. 

---
