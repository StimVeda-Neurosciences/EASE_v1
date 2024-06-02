#include <string.h>

#include "flash_op.h"
#include "flash_op_private.h"

// ////// include the rom functionality here 
#include "esp_rom_spiflash.h"
#include "esp_rom_md5.h"


#include "esp_log.h"

#include "spi_flash_mmap.h"
#include "sdkconfig.h"

static const char * TAG = "Flash_partitions";


// this is the global handle , so as to use this api only onces in pair 
static spi_flash_mmap_handle_t map;

/// @brief map the free pages 
/// @param  void 
/// @return free pages in the flash 
uint32_t esp_mmap_get_free_pages(void)
{
    return spi_flash_mmap_get_free_pages(SPI_FLASH_MMAP_DATA);
}


/// @brief map the flash addr to cache for cpu to read 
/// @param src_addr 
/// @param size 
/// @return succ/err
const void * esp_flash_mmap(uint32_t src_addr, uint16_t size)
{
   if (map) {
        ESP_LOGE(TAG, "tried to bootloader_mmap twice");
        return NULL; /* existing mapping in use... */
    }
    const void *result = NULL;

    // map the flash page wise 
    uint32_t src_page = src_addr & ~(SPI_FLASH_MMU_PAGE_SIZE - 1);
    size += (src_addr - src_page);
    esp_err_t err = spi_flash_mmap(src_page, size, SPI_FLASH_MMAP_DATA, &result, &map);
    
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "spi_flash_mmap failed: 0x%x", err);
        return NULL;
    }
    return (void *)((intptr_t)result + (src_addr - src_page));
}

/// @brief unmap the mapped flash address 
/// @param map_addr 
void esp_flash_unmap(const void * map_addr)
{
    if (map_addr && map) {
        spi_flash_munmap(map);
    }
    map = 0;
}


/// @brief verify the partition table based on the input data
/// @param partition_table
/// @param num_partitions
/// @return succ/err
static esp_err_t esp_flash_verify_partition_table(const esp_partition_info_t* partition_table, int* num_partitions)
{
    int md5_found = 0;
    size_t num_parts; 
    uint32_t chip_size = g_rom_flashchip.chip_size;
    *num_partitions = 0;


    for (num_parts = 0; num_parts < ESP_PARTITION_TABLE_MAX_ENTRIES; num_parts++)
    {
        const esp_partition_info_t* part = &partition_table[num_parts];

        if (part->magic == ESP_PARTITION_MAGIC)
        {
            if (part->pos.offset > chip_size || part->pos.offset + part->pos.size > chip_size)
            {
                ESP_LOGE(TAG,
                         "partition %d invalid - offset 0x%x size 0x%x exceeds flash chip size 0x%x",
                         num_parts,
                         part->pos.offset,
                         part->pos.size,
                         chip_size);
                return ERR_SYS_INVALID_STATE;
            }
        } else if (part->magic == ESP_PARTITION_MAGIC_MD5)
        {
            if (md5_found)
            {
                ESP_LOGE(TAG, "Only one MD5 checksum is allowed");
                return ERR_SYS_INVALID_STATE;
            }

            md5_context_t context;
            unsigned char digest[16];
            esp_rom_md5_init(&context);
            esp_rom_md5_update(&context, (unsigned char*) partition_table, num_parts * sizeof(esp_partition_info_t));
            esp_rom_md5_final(digest, &context);

            unsigned char* md5sum = ((unsigned char*) part) + ESP_PARTITION_MD5_OFFSET;

            if (memcmp(md5sum, digest, sizeof(digest)) != 0)
            {

                ESP_LOGE(TAG, "Incorrect MD5 checksum");
                return ERR_SYS_INVALID_STATE;
            }
            // MD5 checksum matches and we continue with the next interation in
            // order to detect the end of the partition table
            md5_found = 1;
        }

        // end of partition table in flash detected
        else if (part->magic == 0xFFFF && part->type == PART_TYPE_END && part->subtype == PART_SUBTYPE_END)
        {
            ESP_LOGD(TAG, "partition table verified, %d entries", num_parts);
            *num_partitions = num_parts - md5_found; // do not count the partition where the MD5 checksum is held
            if (num_parts == 0)
            {
                return ERR_SYS_NO_RESOURCES;
            } else
            {
                return ESP_OK;
            }
        } else
        {
            ESP_LOGE(TAG, "partition %d invalid magic number 0x%x", num_parts, part->magic);
            return ERR_SYS_INVALID_STATE;
        }
    }

    ESP_LOGE(TAG, "partition table has no terminating entry, not valid");

    return ERR_SYS_INVALID_STATE;
}

/// @brief this func read the partition table and init the 
/// @param  partition_pos_struct
void esp_read_partition_table(esp_flash_partition_struct_t *part_struct)
{
    // map the flash parition to data cache 
    esp_err_t err = 0;

    const esp_partition_info_t* partitions;
    int num_partitions;

    // map the partition address flash to cpu map
    partitions = esp_flash_mmap(ESP_PARTITION_TABLE_OFFSET, ESP_PARTITION_TABLE_MAX_LEN);
    if (!partitions)
    {
        ESP_LOGE(TAG, "bootloader_mmap(0x%x, 0x%x) failed", ESP_PARTITION_TABLE_OFFSET, ESP_PARTITION_TABLE_MAX_LEN);
        err = ERR_SYS_OP_FAILED;
        goto exit;
    }

    ///verify the partition table and get number of partitions
    err = esp_flash_verify_partition_table(partitions, &num_partitions);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to verify partition table");
        goto exit;
    }

    for (uint16_t i = 0; i < num_partitions; i++)
    {
        if (partitions[i].type == PART_TYPE_DATA)
        {
            switch (partitions[i].subtype)
            {
                case PART_SUBTYPE_DATA_OTA_META_DATA:
                    /// partitions position for finding ota meta data sector 0
                    part_struct->ota_meta_partition[OTA_META_DATA_SECTOR0].offset = partitions[i].pos.offset;
                    part_struct->ota_meta_partition[OTA_META_DATA_SECTOR0].size = FLASH_SECTOR_SIZE;

                    part_struct->ota_meta_partition[OTA_META_DATA_SECTOR1].offset = partitions[i].pos.offset + FLASH_SECTOR_SIZE;
                    part_struct->ota_meta_partition[OTA_META_DATA_SECTOR1].size = FLASH_SECTOR_SIZE;

                    break;
                    
                case PART_SUBTYPE_DATA_BOOT_DUMP:
                    /// partition for dumping bootloader messages to the main app 
                    part_struct->boot_dump_partition.offset = partitions[i].pos.offset;
                    part_struct->boot_dump_partition.size = partitions[i].pos.size;
                    break;

                default:
                    break;
            }
        }

        else if (partitions[i].type == PART_TYPE_APP)
        {
            switch (partitions[i].subtype)
            {
                case PART_SUBTYPE_APP_APP_0:
                    memcpy(&part_struct->app_partition[0], &partitions[i].pos, sizeof(esp_partition_pos_t));
                    break;

                case PART_SUBTYPE_APP_APP_1:
                    memcpy(&part_struct->app_partition[1], &partitions[i].pos, sizeof(esp_partition_pos_t));
                    break;
                // case PART_SUBTYPE_APP_DFU_0:
                //     memcpy(&boot_struct->dfu_parts[0], &partitions[i].pos, sizeof(esp_partition_pos_t));
                //     break;
                // case PART_SUBTYPE_APP_DFU_1:
                //     memcpy(&boot_struct->dfu_parts[1], &partitions[i].pos, sizeof(esp_partition_pos_t));
                //     break;
                default:
                    break;
            }
        }
    }

exit:
    // unmap the partition table
    esp_flash_unmap(partitions);
    // fialed the asset here if there is an error 
    assert(!err);


}


/// @brief return the value based on the ota partition position
/// @param ota_pos
/// @param last_ele_addr
/// @param last_ele_size
/// @param size_used
/// @return succ/err
static esp_err_t esp_read_ota_partition(const esp_partition_pos_t* ota_pos, uint32_t* last_ele_addr, uint32_t* last_ele_size, uint32_t* size_used)
{
    esp_err_t err = ESP_OK;

    if (!ota_pos)
    {
        err = ERR_SYS_INVALID_PARAM;
        goto err;
    }

    // have to map the sector 0 first
    const void* esp_ota_partition_addr = esp_flash_mmap(ota_pos->offset, ota_pos->size);

    // this should not happen ideally, if so then fail and reset
    if (!esp_ota_partition_addr)
    {
        err = ERR_SYS_OP_FAILED;
        goto err;
    }

    // -------------------------------- find the last stacked element and also check if sector is full -----------------------------
    // size in bytes
    uint32_t data_size_sec = 0;
    // store the address, where to write
    uint32_t last_elem_size = 0;

    // search the first sector about last element
    while (data_size_sec < FLASH_SECTOR_SIZE)
    {
        // void * is incremented by 1 byte
        const bootloader_ota_metd_struct_t* ota_data = esp_ota_partition_addr + data_size_sec;

        // check for valid magic number
        if ((ota_data->magic_number_start == BOOTLOADER_OTA_STRUCT_MAGIC_NO) &&
            (ota_data->magic_number_end == BOOTLOADER_OTA_STRUCT_MAGIC_NO))
        {
            data_size_sec += ota_data->size;
            last_elem_size = ota_data->size;
            // skip to next block, if garbage collect
            if (ota_data->garbage_struct == BOOTLAODER_OTA_STRUCT_NEW)
            {
                break;
            }

        }
        // data is corrupted
        else if ((ota_data->magic_number_start == BOOTLOADER_OTA_STRUCT_MAGIC_NO) &&
                 (ota_data->magic_number_end != BOOTLOADER_OTA_STRUCT_MAGIC_NO))
        {
            // data is corrupted, skip
            // check for valid size not excedding 25 % of total space
            if (ota_data->size < (FLASH_SECTOR_SIZE / 4))
            {
                data_size_sec += ota_data->size;
                last_elem_size = ota_data->size;
            } else
            {
                // assume that size is less then or equal to struct size
                data_size_sec += BOOTLOADER_OTA_METAD_SIZE;
                last_elem_size = BOOTLOADER_OTA_METAD_SIZE;
            }

        } else
        {
            // rest we are treating as end of stack
            // reach at the end of stack
            break;
        }
    }

    if (data_size_sec == 0)
    {
        err = ;
        goto err;
    }

    // unmap the sector
    esp_flash_unmap(esp_ota_partition_addr);

    // return the params
    if (size_used != NULL)
    {
        *size_used = data_size_sec;
    }
    if (last_ele_size != NULL)
    {
        *last_ele_size = last_elem_size;
    }
    if (last_ele_addr != NULL)
    {
        *last_ele_addr = ota_pos->offset + data_size_sec - last_elem_size;
    }

    return ESP_OK;

/// reach here when ther is an error
err:
    // return the params
    if (size_used != NULL)
    {
        *size_used = 0;
    }
    if (last_ele_size != NULL)
    {
        *last_ele_size = 0;
    }
    if (last_ele_addr != NULL)
    {
        *last_ele_addr = 0;
    }
    return err;
}