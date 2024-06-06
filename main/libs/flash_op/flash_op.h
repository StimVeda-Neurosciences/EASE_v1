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



typedef enum __APP_INFO_INDEX__
{
    flash_app_info_none,
    flash_app_info_Hardware_num,
    flash_app_info_serial_num,
    flash_app_info_firmware_num,
    flash_app_info_device_num,
    flash_app_info_app_name,
    flash_app_info_app_manuf_name,

}flash_app_info_index_t;



/// @brief get the input string from the flash/DROM 
/// @param  void 
/// @return hardware revision string
const char * flash_app_get_info(flash_app_info_index_t index);

/// @brief get the len of the string 
/// @param  void 
/// @return len of the string 
uint16_t flash_app_get_info_len(flash_app_info_index_t index);
