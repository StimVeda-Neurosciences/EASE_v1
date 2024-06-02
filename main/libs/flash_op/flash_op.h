#pragma once 

// functions for manipulating the OTA metadata 

// functions for getting the hardware version, serial number , etc 


#include <stdio.h>
#include <stdint.h>

#include "esp_err.h"

#include "sys_attr.h"


void flash_op_driver_init(void);


void flash_op_driver_deinit(void);


esp_err_t flash_op_read_app_desc(uint8_t * buff, uint16_t size);

esp_err_t flash_op_switch_to_dfu(void );

esp_err_t flash_op_read_boot_dump_size(uint16_t *size);

esp_err_t flash_op_read_boot_dump_msgs(uint16_t * no_of_msg);

esp_err_t flash_op_read_boot_fail_type(uint32_t * failure);