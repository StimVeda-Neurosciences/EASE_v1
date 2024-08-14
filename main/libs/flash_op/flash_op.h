#pragma once 

// functions for manipulating the OTA metadata 

// functions for getting the hardware version, serial number , etc 


#include <stdio.h>
#include <stdint.h>

#include "esp_err.h"

#include "system_attr.h"

/// @brief how many key value pair can be stored 
#define MAX_NO_OF_KEY_ATTR 6
/// @brief len of the atttribute
#define MAX_LEN_OF_KEY_ATTR 20

/******
 * the flash stores the headband data in json
 * {"hardware_Ver":"45SQ","serial_number"}
 */

/// @brief Initialise the flash driver for operations like DFU switching, etc
/// @param  void
void flash_op_driver_init(void);


/// @brief deinitalise the Flash op driver 
/// @param  void
void flash_op_driver_deinit(void);


/// @brief this would get the bootloader errors if any 
/// @param  void
/// @return errors
uint32_t flash_op_get_boot_errs(void);


/// @brief this will write new metadata so that bootloader can switch to DFU partition 
/// @param  void
/// @return succ/failure
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
