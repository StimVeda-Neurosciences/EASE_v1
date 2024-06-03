#include<string.h>

#include "flash_op.h"
// include the pirvate OTA defs 
#include "flash_op_private.h"
#include "flash_otameta_data.h"


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



esp_err_t flash_op_switch_to_dfu(void )
{
    uint32_t sec0_size =0;
    uint32_t sec1_size =0;

    ota_metd_struct_t temp_data = {0};
    // read the input size of both the meta data partition 
    esp_err_t err = esp_ota_get_used_size(&partition_pos, OTA_META_DATA_SECTOR0,&sec0_size);
    ESP_ERR_CHECK(err);
    err = esp_ota_get_used_size(&partition_pos, OTA_META_DATA_SECTOR1,&sec1_size);
    ESP_ERR_CHECK(err);
    
    // if size mismatch then erase both the meta and start a new (do in sequential order)
    if(sec0_size != sec1_size)
    {
        // read the last active meta data from sector that have higher size 
        esp_read_flash_ota_data(&partition_pos,temp); (sec0_size <sec1_size)?OTA_META_DATA_SECTOR1:OTA_META_DATA_SECTOR0;
    }
    
    
    // else 
    // can we modify the content 
    // no , then write the new data 
    // if size overflodded when writing new data , have to leave space atleast 250 bytes  


}

esp_err_t flash_op_read_boot_dump_size(uint16_t *size);

esp_err_t flash_op_read_boot_dump_msgs(uint16_t * no_of_msg);

esp_err_t flash_op_read_boot_fail_type(uint32_t * failure);


esp_err_t flash_op_read_app_desc(uint8_t * buff, uint16_t size)
{

}


uint8_t flash_op_get_bootloader_err(void);