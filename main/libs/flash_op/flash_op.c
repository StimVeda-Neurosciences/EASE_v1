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

/// @brief get the dfu partition to boot and also ammend the changes in the partition_pos
/// @param part 
/// @return succ/failure
static esp_err_t esp_get_dfu_boot_partition(ota_metd_struct_t * part);


// =================================================================================
// -------------------------------------- function definations here ----------------


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
    EXIT_IF_ERR(err);
    err = esp_ota_get_used_size(&partition_pos, OTA_META_DATA_SECTOR1,&sec1_size);
    EXIT_IF_ERR(err);
    
    // ============== check if we have to do write data freshly ====================================== 
    // if size mismatch or overflows  then erase both the meta and start a new (do in sequential order)
    if(sec0_size != sec1_size ||  (MAX_OF(sec0_size,sec1_size) +  2*OTA_METAD_SIZE) > FLASH_SECTOR_SIZE)
    {
        // read the last active meta data from sector that have higher size 
        uint8_t sec_no =(sec0_size <sec1_size)?OTA_META_DATA_SECTOR1:OTA_META_DATA_SECTOR0;
        err =esp_read_flash_ota_data_fromsector(&partition_pos,sec_no, &temp_data); 
        EXIT_IF_ERR(err);

        // get the dfu boot partition index 
        err = esp_get_dfu_boot_partition(&temp_data);
        EXIT_IF_ERR(err);

        //now write new fresh data 
        esp_write_ota_data_fresh(&partition_pos,&temp_data);
        EXIT_IF_ERR(err);
    }
    else 
    {
        // read the ota data, exit if can't able to read  
        err = esp_read_flash_ota_data(&partition_pos,&temp_data);
        EXIT_IF_ERR(err);

        // get the dfu boot partition index 
        err = esp_get_dfu_boot_partition(&temp_data);
        EXIT_IF_ERR(err);

        uint16_t offset = (temp_data.boot_index.dfu_image == BOOT_INDEX_READY)?OFFSET_OF(ota_metd_struct_t,boot_index.dfu_image):OFFSET_OF(ota_metd_struct_t,boot_index.dfu_backup_image);
        // can we modify the content 
        err = esp_modify_ota_data(&partition_pos,offset,BOOT_INDEX_READY);
        
        // no , then write the new data 
        if(err == ERR_SYS_INVALID_DATA)
        {
            // write new data 
            err = esp_flash_write_ota_data(&partition_pos, &temp_data);
            EXIT_IF_ERR(err);
        }
    }
    
err: 
    ESP_LOGE(TAG,"error occured ");
    return err;
}


esp_err_t flash_op_read_boot_dump_size(uint16_t *size)
{
    esp_err_t err =0;

    return err;
}

esp_err_t flash_op_read_boot_fail_type(uint32_t * failure)
{
    esp_err_t err =0;

    return err;
}


esp_err_t flash_op_read_app_desc(uint8_t * buff, uint16_t size)
{
    esp_err_t err =0;

    return err;
}


// esp_err_t flash_op_get_bootloader_err(void)
// {
    
// }



// ===========================================================================================
// ----------------------------------------- static functions here ---------------------------

/// @brief get the dfu partition to boot and also ammend the changes in the partition_pos
/// @param part 
/// @return succ/failure
static esp_err_t esp_get_dfu_boot_partition(ota_metd_struct_t * part)
{

      // figure out which index to boot 
        if((part->image_index.dfu_image == OTA_IMAGE_INDEX_FIRST ||
         part->image_index.dfu_image == OTA_IMAGE_INDEX_SECOND) && part->state.dfu_image_state == OTA_IMAGE_VERIFIED)
        {
            // try to boot the dfu image first 
            part->boot_index.dfu_image = BOOT_INDEX_READY;
        }
        else if(part->image_index.dfu_backup_image == OTA_IMAGE_INDEX_FIRST || 
        part->image_index.dfu_backup_image == OTA_IMAGE_INDEX_SECOND)
        {
            // if not find a valid dfu then try to boot dfu_backup_image
            part->boot_index.dfu_backup_image = BOOT_INDEX_READY;
        }
        else
        {
            // no dfu image ========================
            // send the error 
            return OTA_ERR_BOOT_NO_DFU_IMAGES;
        }

    return ESP_OK;
}