#include <string.h>

#include "flash_op.h"
#include "flash_op_private.h"
#include "flash_otameta_data.h"

#include "esp_flash.h"
#include "esp_log.h"

static const char *TAG = "boot_flash";

#define ERASE_SECTOR_TRIES 4

/// @brief return the value based on the ota partition position
/// @param ota_pos
/// @param last_ele_addr
/// @param last_ele_size
/// @param size_used
/// @return succ/err
static esp_err_t esp_read_ota_partition(const esp_partition_pos_t *ota_pos, uint32_t *last_ele_addr, uint32_t *last_ele_size, uint32_t *size_used);

// =======================================================================================================
// ========================================================================================================

/// @brief write the OTA meta data into the ota meta sector
/// @param part_struct
/// @param data
/// @return err/succ
esp_err_t esp_flash_write_ota_data(const esp_flash_partition_struct_t *part_struct, const ota_metd_struct_t *data)
{
    // check for valid params
    if (!(part_struct && data))
    {
        return ERR_SYS_INVALID_PARAM;
    }
    if (data->garbage_struct != OTA_STRUCT_NEW)
    {
        return ERR_SYS_INVALID_DATA;
    }

    esp_err_t err = 0;

    uint32_t last_ele_addr = 0, last_ele_size = 0, used_size = 0;
    err = esp_read_ota_partition(&part_struct->ota_meta_partition[OTA_META_DATA_SECTOR0], &last_ele_addr, &last_ele_size, &used_size);

    // write to sector 0

    // ---------------------- erase the sector if req and reset the last pointer ------------------------------------------------------
    // check if have to erase the sector
    if ((used_size + data->size) >= FLASH_SECTOR_SIZE)
    {
        // erase the sector
        uint16_t sector = part_struct->ota_meta_partition[OTA_META_DATA_SECTOR0].offset / FLASH_SECTOR_SIZE;
        err = 0;
        BOOT_EXECUTE_PROCESS(esp_erase_flash_sector(sector), ERASE_SECTOR_TRIES, err);
        ESP_ERR_CHECK(err);
        // reset the pointer to start of the sector
        last_ele_addr = part_struct->ota_meta_partition[OTA_META_DATA_SECTOR0].offset;
        last_ele_size = 0;
    }

    // ------------------------------ start writing to the sector -----------------------------------------------------------------
    // mark last sector as garbage if not sector is erased
    // check if there is no sector full and no first element
    if (!((used_size + data->size) >= FLASH_SECTOR_SIZE) && !(last_ele_size == 0))
    {
        // get the address of last stack element garabge data address
        uint32_t garbage_no = OTA_GRABAGE_DUMP;
        uint32_t garbage_addr = last_ele_addr + OFFSET_OF(ota_metd_struct_t, garbage_struct);
        err = esp_write_flash(garbage_addr, &garbage_no, sizeof(garbage_no), false);
        ESP_ERR_CHECK(err);
    }

    // write to the next empty addr
    err = esp_write_flash((last_ele_addr + last_ele_size), (void *)data, data->size, false);
    ESP_ERR_CHECK(err);

    // ================================================================================================================================
    // ====================================================== similar process follows for writing next sector =========================
    // ================================================================================================================================

    err = esp_read_ota_partition(&part_struct->ota_meta_partition[OTA_META_DATA_SECTOR1], &last_ele_addr, &last_ele_size, &used_size);

    // ---------------------- erase the sector if req and reset the last pointer ------------------------------------------------------
    // check if have to erase the sector
    if ((used_size + data->size) >= FLASH_SECTOR_SIZE)
    {
        // erase the sector
        uint16_t sector = part_struct->ota_meta_partition[OTA_META_DATA_SECTOR1].offset / FLASH_SECTOR_SIZE;
        err = 0;
        BOOT_EXECUTE_PROCESS(esp_erase_flash_sector(sector), ERASE_SECTOR_TRIES, err);
        ESP_ERR_CHECK(err);
        // reset the pointer to start of the sector
        last_ele_addr = part_struct->ota_meta_partition[OTA_META_DATA_SECTOR1].offset;
        last_ele_size = 0;
    }

    // ------------------------------ start writing to the sector -----------------------------------------------------------------
    // mark last sector as garbage if not sector is erased
    // check if there is no sector full and no first element
    if (!((used_size + data->size) >= FLASH_SECTOR_SIZE) && !(last_ele_size == 0))
    {
        // get the address of last stack element garabge data address
        uint32_t garbage_no = OTA_GRABAGE_DUMP;
        uint32_t garbage_addr = last_ele_addr + OFFSET_OF(ota_metd_struct_t, garbage_struct);
        err = esp_write_flash(garbage_addr, &garbage_no, sizeof(garbage_no), false);
        ESP_ERR_CHECK(err);
    }

    err = esp_write_flash((last_ele_addr + last_ele_size), (void *)data, data->size, false);
    ESP_ERR_CHECK(err);

    // return success
    return ESP_OK;
}

/// @brief erase a sector based on the sector number
/// @param part_struct
/// @param sect_no
/// @return succ/failure
esp_err_t esp_erase_ota_sectors(const esp_flash_partition_struct_t *part_struct, uint8_t sect_no)
{
    if (!(part_struct))
    {
        return ERR_SYS_INVALID_PARAM;
    }
    esp_err_t err = 0;
    // erase the sector
    if (sect_no >= OTA_META_DATA_MAX_SECTORS)
        return ERR_SYS_INVALID_PARAM;

    uint16_t sector = part_struct->ota_meta_partition[sect_no].offset / FLASH_SECTOR_SIZE;
    BOOT_EXECUTE_PROCESS(esp_erase_flash_sector(sector), ERASE_SECTOR_TRIES, err);
    ESP_ERR_CHECK(err);

    return ESP_OK;
}

/// @brief dump the ota data memory into serial terminal
/// @param part_struct
/// @param bytes
/// @return succ/failure
esp_err_t esp_flash_dump_ota_data(esp_flash_partition_struct_t *part_struct, uint8_t sect_no, uint16_t bytes)
{
    if (!(part_struct))
    {
        return ERR_SYS_INVALID_PARAM;
    }
    esp_err_t err = 0;
    // erase the sector
    if ((bytes >= FLASH_SECTOR_SIZE) || (sect_no >= OTA_META_DATA_MAX_SECTORS))
        return ERR_SYS_INVALID_PARAM;

    uint32_t mem[(bytes / 4) + 4];

    // treat the memory as 1 byte
    err = esp_read_flash(part_struct->ota_meta_partition[sect_no].offset, (uint8_t *)mem, bytes, false);
    ESP_ERR_CHECK(err);

    uint32_t word_size = (bytes / 4) + 1;
    for (uint16_t i = 0; i < word_size; i++)
    {
        esp_rom_printf("%x ", mem[i]);

        if ((i > 0) && !((i + 1) % (OTA_METAD_SIZE / 4)))
        {
            esp_rom_printf("\r\n");
        }
    }
    esp_rom_printf("\r\nend of data \r\n");
    return ESP_OK;
}

/// @brief get the size used by OTA meta data
/// @param part_struct
/// @param sec_no
/// @param size_ptr
/// @return
esp_err_t esp_ota_get_used_size(const esp_flash_partition_struct_t *part_struct, uint8_t sec_no, uint32_t *size)
{

    if (!(part_struct))
    {
        return ERR_SYS_INVALID_PARAM;
    }
    if ((sec_no >= OTA_META_DATA_MAX_SECTORS) || (size == NULL))
    {
        return ERR_SYS_INVALID_PARAM;
    }
    return esp_read_ota_partition(&part_struct->ota_meta_partition[sec_no], NULL, NULL, size);
}

/// @brief write the ota meta data to the ota partition as fresh, i.e. erase all the data first and then write
/// @param part_struct
/// @param data
/// @return succ/failure
esp_err_t esp_write_ota_data_fresh(const esp_flash_partition_struct_t *part_struct, const ota_metd_struct_t *data)
{
    esp_err_t err = 0;
    if (!(part_struct && data))
    {
        return ERR_SYS_INVALID_PARAM;
    }

    if (data->garbage_struct != OTA_STRUCT_NEW)
    {
        return ERR_SYS_INVALID_DATA;
    }

    // erase the sector
    uint16_t sector = part_struct->ota_meta_partition[OTA_META_DATA_SECTOR0].offset / FLASH_SECTOR_SIZE;
    BOOT_EXECUTE_PROCESS(esp_erase_flash_sector(sector), ERASE_SECTOR_TRIES, err);
    ESP_ERR_CHECK(err);

    err = esp_write_flash(part_struct->ota_meta_partition[OTA_META_DATA_SECTOR0].offset, (void *)data, data->size, false);
    ESP_ERR_CHECK(err);

    // write to sector 1

    // erase the sector
    sector = part_struct->ota_meta_partition[OTA_META_DATA_SECTOR1].offset / FLASH_SECTOR_SIZE;
    BOOT_EXECUTE_PROCESS(esp_erase_flash_sector(sector), ERASE_SECTOR_TRIES, err);
    ESP_ERR_CHECK(err);

    err = esp_write_flash(part_struct->ota_meta_partition[OTA_META_DATA_SECTOR1].offset, (void *)data, data->size, false);
    ESP_ERR_CHECK(err);

    return ESP_OK;
}

/// @brief check for valid 1-->0 translation
/// @param prev_no
/// @param new_no
/// @return true/false
static inline bool check_valid_1_to_0(uint32_t prev_no, uint32_t new_no)
{
    // check if new number is >0 not prev_no as we only intereset in 1-->0,
    // if rest of the left part of new_no is zero then no need to check further
    while (new_no > 0)
    {
        prev_no = prev_no >> 1;
        new_no = new_no >> 1;
        // check for falses
        bool res = ((prev_no & 0x01)) ? (1) : (((new_no & 0x01) ? (0) : (1)));
        if (!res)
            return res;
    }
    return 1;
}

// =======================================================================================================
// ========================================================================================================

/// @brief this function would overwrite/modify the fields of last stack element to the data provided.
/// @note this function also checks if the data provided can be writeable to flash as it can only do 1-->0 traslation
/// @param part_struct
/// @param data
/// @return succ/failure
esp_err_t esp_modify_ota_data(const esp_flash_partition_struct_t *part_struct, uint16_t offset, uint32_t data)
{

    if (!(part_struct))
    {
        return ERR_SYS_INVALID_PARAM;
    }

    // check for valid offset
    if ((offset > OTA_MODF_OFFS_END) || (offset < OTA_MODF_OFFS_START))
    {
        return ERR_SYS_INVALID_PARAM;
    }

    esp_err_t err = 0;

    // write to sector 0
    // =========================================================================================================
    // temp variable to store that variable
    uint32_t prev_data = 0;

    uint32_t last_ele_addr = 0, last_ele_size = 0, used_size = 0;
    err = esp_read_ota_partition(&part_struct->ota_meta_partition[OTA_META_DATA_SECTOR0], &last_ele_addr, &last_ele_size, &used_size);
    ESP_ERR_CHECK(err);

    if (!last_ele_size)
    {
        return ERR_SYS_EMPTY_MEM;
    }

    // get the data from that member
    err = esp_read_flash((last_ele_addr + offset), &prev_data, sizeof(prev_data), false);
    ESP_ERR_CHECK(err);

    // check for valid data transactions
    // changing 1--> 0 always lead to decrease in number, also check no of 1s decrease
    if (!check_valid_1_to_0(prev_data, data))
    {
        return ERR_SYS_INVALID_DATA;
    }

    err = esp_write_flash((last_ele_addr + offset), &data, sizeof(data), false);
    ESP_ERR_CHECK(err);

    // ================================================================================================================================
    // ====================================================== similar process follows for writing next sector =========================
    // ================================================================================================================================

    // write to sector 1
    err = esp_read_ota_partition(&part_struct->ota_meta_partition[OTA_META_DATA_SECTOR1], &last_ele_addr, &last_ele_size, &used_size);
    ESP_ERR_CHECK(err);

    if (!last_ele_size)
    {
        return ERR_SYS_EMPTY_MEM;
    }
    // get the data from the offset
    prev_data = 0;
    err = esp_read_flash(last_ele_addr + offset, &prev_data, sizeof(prev_data), false);
    ESP_ERR_CHECK(err);

    // check for valid data transactions
    // changing 1--> 0 always lead to decrease in number, also check no of 1s decrease
    if (!check_valid_1_to_0(prev_data, data))
    {
        return ERR_SYS_INVALID_DATA;
    }

    err = esp_write_flash(last_ele_addr + offset, &data, sizeof(data), false);
    ESP_ERR_CHECK(err);

    // return success
    return ESP_OK;
}

// =======================================================================================================
// ========================================================================================================

/// @brief read the ota meta data from the ota meta sector
/// @param part_struct
/// @param ata
/// @return err/succ
esp_err_t esp_read_flash_ota_data(const esp_flash_partition_struct_t *part_struct,ota_metd_struct_t *data)
{
    if (!(part_struct && data))
    {
        return ERR_SYS_INVALID_PARAM;
    }

    ota_metd_struct_t meta_data_sector0 = {0};
    ota_metd_struct_t meta_data_sector1 = {0};

    // =============================== read from sector 0================================================
    uint32_t last_ele_addr0 = 0, last_ele_size0 = 0, used_size0 = 0;
    esp_err_t err = esp_read_ota_partition(&part_struct->ota_meta_partition[OTA_META_DATA_SECTOR0], &last_ele_addr0, &last_ele_size0, &used_size0);
    ESP_ERR_CHECK(err);

    err = esp_read_flash(last_ele_addr0, (void *)&meta_data_sector0, OTA_METAD_SIZE, false);
    ESP_ERR_CHECK(err);
    // ==========================================================================================================
    // ================================ read from sector 1 ============================================================
    uint32_t last_ele_addr1 = 0, last_ele_size1 = 0, used_size1 = 0;
    err = esp_read_ota_partition(&part_struct->ota_meta_partition[OTA_META_DATA_SECTOR1], &last_ele_addr1, &last_ele_size1, &used_size1);

    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "can't load backup ota meta data sector");
        // load sector 0 data if valid
        if ((meta_data_sector0.magic_number_start == OTA_STRUCT_MAGIC_NO) &&
            (meta_data_sector0.magic_number_end == OTA_STRUCT_MAGIC_NO))
        {
            // copy the data and return
            memcpy((void *)data, &meta_data_sector0, sizeof(meta_data_sector0));
        }
        else
        {
            return ERR_SYS_OP_FAILED;
        }
        return ERR_SYS_API_ERR;
    }

    // read the data from sector 1
    err = esp_read_flash(last_ele_addr1, (void *)&meta_data_sector1, OTA_METAD_SIZE, false);
    ESP_ERR_CHECK(err);
    // ------------------------------------------------------------------------------------------------------
    // now validate both data sector0 and sector1 data

    // if we don't have value in sector 0
    if (meta_data_sector0.magic_number_start != OTA_STRUCT_MAGIC_NO)
    {
        // is sector 1 is also empty then no clue about ota meta data
        if ((meta_data_sector1.magic_number_start != OTA_STRUCT_MAGIC_NO))
        {
            return ERR_SYS_INVALID_STATE;
        }
        // get the ota meta from sector 1
        memcpy((void *)data, &meta_data_sector1, sizeof(ota_metd_struct_t));
    }
    // sector 1 has more data than sector 0 i.e. the process is interrupted, use backup sector data
    else if (last_ele_addr0 < last_ele_addr1)
    {
        memcpy((void *)data, &meta_data_sector1, sizeof(ota_metd_struct_t));
    }
    // use always sector 0 data
    else
    {
        memcpy((void *)data, &meta_data_sector0, sizeof(ota_metd_struct_t));
    }

    return ESP_OK;
}


/// @brief read the ota meta data from the ota meta sector
/// @param part_struct
/// @param ata
/// @return err/succ
esp_err_t esp_read_flash_ota_data_fromsector(const esp_flash_partition_struct_t *part_struct, uint8_t sec_no, ota_metd_struct_t *data)
{
    if (!(part_struct && data))
    {
        return ERR_SYS_INVALID_PARAM;
    }
    
    // erase the sector
    if (sec_no >= OTA_META_DATA_MAX_SECTORS)
        return ERR_SYS_INVALID_PARAM;
    // =============================== read from sector 0================================================
    uint32_t last_ele_addr0 = 0, last_ele_size0 = 0, used_size0 = 0;
    esp_err_t err = esp_read_ota_partition(&part_struct->ota_meta_partition[sec_no], &last_ele_addr0, &last_ele_size0, &used_size0);
    ESP_ERR_CHECK(err);

    err = esp_read_flash(last_ele_addr0, (void *)&data, OTA_METAD_SIZE, false);
    ESP_ERR_CHECK(err);

    return ESP_OK;
}


// ============--------------------------------------------------------------------------------------------------
// ---------------------------------------------------------------------------------------------------------------
// =========================== static functions here  =============================================================

/// @brief return the value based on the ota partition position
/// @param ota_pos
/// @param last_ele_addr
/// @param last_ele_size
/// @param size_used
/// @return succ/err
static esp_err_t esp_read_ota_partition(const esp_partition_pos_t *ota_pos, uint32_t *last_ele_addr, uint32_t *last_ele_size, uint32_t *size_used)
{
    esp_err_t err = ESP_OK;

    if (!ota_pos)
    {
        err = ERR_SYS_INVALID_PARAM;
        goto err;
    }

    // have to map the sector 0 first
    const void *esp_ota_partition_addr = esp_flash_mmap(ota_pos->offset, ota_pos->size);

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
        const ota_metd_struct_t *ota_data = esp_ota_partition_addr + data_size_sec;

        // check for valid magic number
        if ((ota_data->magic_number_start == OTA_STRUCT_MAGIC_NO) &&
            (ota_data->magic_number_end == OTA_STRUCT_MAGIC_NO))
        {
            data_size_sec += ota_data->size;
            last_elem_size = ota_data->size;
            // skip to next block, if garbage collect
            if (ota_data->garbage_struct == OTA_STRUCT_NEW)
            {
                break;
            }
        }
        // data is corrupted
        else if ((ota_data->magic_number_start == OTA_STRUCT_MAGIC_NO) &&
                 (ota_data->magic_number_end != OTA_STRUCT_MAGIC_NO))
        {
            // data is corrupted, skip
            // check for valid size not excedding 25 % of total space
            if (ota_data->size < (FLASH_SECTOR_SIZE / 4))
            {
                data_size_sec += ota_data->size;
                last_elem_size = ota_data->size;
            }
            else
            {
                // assume that size is less then or equal to struct size
                data_size_sec += OTA_METAD_SIZE;
                last_elem_size = OTA_METAD_SIZE;
            }
        }
        else
        {
            // rest we are treating as end of stack
            // reach at the end of stack
            break;
        }
    }

    // unmap the sector
    esp_flash_unmap(esp_ota_partition_addr);

    // check for zero data 
    if (data_size_sec == 0)
    {
        err = ERR_SYS_EMPTY_MEM;
        goto err;
    }

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