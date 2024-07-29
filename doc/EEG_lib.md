# EEG module :

## Introduction

This page describes the EEG implementation of the EASE device, basic structure. For more information please refer to the [EEG.c](../Ease_firmware/main/sources/ads/ads.c)

---

### Table of Contents

- [EEG init](#EEG-init)
- [EEG deinit](#EEG-deinit)
- [EEG Data transfers](#Eeg-data-transfers)
- [EEG tasks](#EEG-tasks)
- [EEG saftey features](#eeg-saftey-detures)

---

### EEG init 
---
There are two types of initialisation in EEG 
- `System Level intisation` -> The system level init would install the SPI driver , init the GPIO pins and create Continers for Data transfer.
- `Init in EEG task` -> before using the EEG protocol, the EEG module has to be in ready state, to do so the ```uint8_t ads_init(uint8_t rate, uint8_t mode)``` function will intiailse the EEG module according to the parameter and init the EEG module with proper configuration. also start the interrupt handling 

---

### EEG deinit
---
Similarly like inits the deinitialisation are of two types 
- `systel level deinits` -> the system level deinit would uninstall the SPI driver, deinit the GPIo and delete & free the memory conatiner for data transfer.
1 `Deinit in EEG task` -> the EEG deinit should be called at the end of EEG task, this will stop the interrupt, reset the FIFO and turn off the EEG module  

---

### EEG Data transfers
---
The EEG module will interrupt the MCU when there is data available in the IC and the Mcu has to read the data from the IC. the MCU would transfer the data to the EEG task through the FIFO. 
<br>
The EEG task would wait for the particular amount of data and if it would available then get all the data in a buffer and transmit it to the APP.


---



### EEG tasks
---
The EEG task is controlled and handelled by the General task, the data arrives from interrupt to this task using FIFO. this task handles all the data transferiing to the APP. this task also check for any error and fault in the EEG module and handle it properly. 
<br>
The error could be BIOS error or hardware faults & device charging during sessions. 



### EEG saftey features
--- 
You cannot run any protocol while the device is charging, if the device see any charging moment then it immediately stops the protocols, send the errors to the APP. the App can only run the protocol when device is in discharging state. 

---

