#include<string.h>

#include "flash_op.h"
// include the pirvate OTA defs 
#include "flash_op_private.h"

#include "esp_log.h"
#include "esp_flash.h"

#include "esp_partition.h"


static const char *  TAG = "FLASH_OP";


static esp_flash_partition_struct_t partition_pos;



void flash_op_driver_init(void)
{
    // read the partitions and init it 
    esp_read_partition_table(&partition_pos);
        
}


void flash_op_driver_deinit(void)
{
    // clear the partition position data 
    memset(&partition_pos,0,sizeof(partition_pos));

}



esp_err_t flash_op_switch_to_dfu(void );

esp_err_t flash_op_read_boot_dump_size(uint16_t *size);

esp_err_t flash_op_read_boot_dump_msgs(uint16_t * no_of_msg);

esp_err_t flash_op_read_boot_fail_type(uint32_t * failure);


esp_err_t flash_op_read_app_desc(uint8_t * buff, uint16_t size)
{

}