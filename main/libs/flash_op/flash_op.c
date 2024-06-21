#include <string.h>

#include "flash_op.h"
// include the pirvate OTA defs
#include "flash_op_private.h"
#include "flash_otameta_data.h"
// this app descriptor let's us to store value attributes related to app
#include "app_desc.h"

#include "esp_log.h"
#include "esp_flash.h"

#include "esp_partition.h"

static const char *TAG = "FLASH_OP";

static char key_value[MAX_NO_OF_KEY_ATTR][2][MAX_LEN_OF_KEY_ATTR];

static esp_flash_partition_struct_t partition_pos;

/// @brief get the dfu partition to boot and also ammend the changes in the partition_pos
/// @param part
/// @return succ/failure
static esp_err_t esp_get_dfu_boot_partition(ota_metd_struct_t *part);

/// @brief extract the system attributes related to hardware number,serial num,device number
/// @param
static void extract_system_attributes(void);

/// @brief this is a very simple json extractor
/// @param str
/// @param size
static void extract_json(const char str[], uint16_t size);

// =================================================================================
// -------------------------------------- function definations here ----------------

/// @brief Initialise the flash driver for operations like DFU switching, etc
/// @param  void
void flash_op_driver_init(void)
{
    ESP_LOGW(TAG, "flash op drv init");
    // read the partitions and init it
    esp_read_partition_table(&partition_pos);

    // extract the system attributes deliverd with the app
    extract_system_attributes();
}

/// @brief deinitalise the Flash op driver
/// @param  void
void flash_op_driver_deinit(void)
{
    // clear the partition position data
    memset(&partition_pos, 0, sizeof(partition_pos));
}

/// @brief this will write new metadata so that bootloader can switch to DFU partition
/// @param  void
/// @return succ/failure
esp_err_t flash_op_switch_to_dfu(void)
{
    uint32_t sec0_size = 0;
    uint32_t sec1_size = 0;

    ota_metd_struct_t temp_data = {0};
    // read the input size of both the meta data partition
    esp_err_t err = esp_ota_get_used_size(&partition_pos, OTA_META_DATA_SECTOR0, &sec0_size);
    EXIT_IF_ERR(err);
    err = esp_ota_get_used_size(&partition_pos, OTA_META_DATA_SECTOR1, &sec1_size);
    EXIT_IF_ERR(err);

    // ============== check if we have to do write data freshly ======================================
    // if size mismatch or overflows  then erase both the meta and start a new (do in sequential order)
    if (sec0_size != sec1_size || (MAX_OF(sec0_size, sec1_size) + 2 * OTA_METAD_SIZE) > FLASH_SECTOR_SIZE)
    {
        // read the last active meta data from sector that have higher size
        uint8_t sec_no = (sec0_size < sec1_size) ? OTA_META_DATA_SECTOR1 : OTA_META_DATA_SECTOR0;
        err = esp_read_flash_ota_data_fromsector(&partition_pos, sec_no, &temp_data);
        EXIT_IF_ERR(err);

        // get the dfu boot partition index
        err = esp_get_dfu_boot_partition(&temp_data);
        EXIT_IF_ERR(err);

        // now write new fresh data
        err = esp_write_ota_data_fresh(&partition_pos, &temp_data);
        EXIT_IF_ERR(err);
    }
    else
    {
        // read the ota data, exit if can't able to read
        err = esp_read_flash_ota_data(&partition_pos, &temp_data);
        EXIT_IF_ERR(err);

        esp_flash_print_otadata(&temp_data, sizeof(temp_data));

        // get the dfu boot partition index
        err = esp_get_dfu_boot_partition(&temp_data);
        EXIT_IF_ERR(err);
        // write new data
        err = esp_flash_write_ota_data(&partition_pos, &temp_data);
        EXIT_IF_ERR(err);
    }

    return ESP_OK;
err:
    ESP_LOGE(TAG, "error occured ");
    return err;
}

esp_err_t flash_op_read_boot_dump_size(uint16_t *size)
{
    esp_err_t err = 0;

    return err;
}

esp_err_t flash_op_read_boot_fail_type(uint32_t *failure)
{
    esp_err_t err = 0;

    return err;
}

/// @brief this would get the bootloader errors if any
/// @param  void
/// @return errors
uint32_t flash_op_get_boot_errs(void)
{
    uint32_t err = 0;

    return err;
}

/***
 * data is stored in the flash like
 * "START"="hardware ver":"xyz","serial number":"xyz","device number":"xyz"="END"
 */

/// @brief get the input string from the flash/DROM
/// @param  void
/// @return hardware revision string
const char *flash_app_get_info(flash_app_info_index_t index)
{
    const char *str_to_srch = NULL;
    switch (index)
    {
    case flash_app_info_Hardware_num:
        str_to_srch = "Hardware ver";
        break;
    case flash_app_info_serial_num:
        str_to_srch = "Serial num";
        break;

    case flash_app_info_firmware_num:
        str_to_srch = "Firmware ver";
        break;

    case flash_app_info_device_num:
        str_to_srch = "Device num";
        break;

    case flash_app_info_app_manuf_name:
        str_to_srch = "Manuf. name";
        break;

    case flash_app_info_app_name:
        return app_custom_desc.app_desc.app_name;
        // str_to_srch = "App name";
        break;
    default:
        return " "; // this is intended as the strlen won;t give an error
        break;
    }

    int index_to_srch = -1;
    // run a for loop and compare the str in the key value
    for (int i = 0; i < MAX_NO_OF_KEY_ATTR; i++)
    {
        if (strncmp(key_value[i][0], str_to_srch, strlen(str_to_srch)) == 0)
        {
            index_to_srch = i;
            break;
        }
    }
    if (index_to_srch == -1)
    {
        return " ";
    }

    // /return the string
    return key_value[index_to_srch][1];
}

// ===========================================================================================
// ----------------------------------------- static functions here ---------------------------

/// @brief get the dfu partition to boot and also ammend the changes in the partition_pos
/// @param part
/// @return succ/failure
static esp_err_t esp_get_dfu_boot_partition(ota_metd_struct_t *part)
{

    // figure out which index to boot
    if ((part->image_index.dfu_image == OTA_IMAGE_INDEX_FIRST ||
         part->image_index.dfu_image == OTA_IMAGE_INDEX_SECOND) &&
        part->state.dfu_image_state == OTA_IMAGE_VERIFIED)
    {
        // try to boot the dfu image first
        part->boot_index.dfu_image = BOOT_INDEX_READY;
    }
    else if (part->image_index.dfu_backup_image == OTA_IMAGE_INDEX_FIRST ||
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

/// @brief extract the system attributes related to hardware number,serial num,device number
/// @param
static void extract_system_attributes(void)
{
    const char *str = (const char *)app_custom_desc.app_extra_mem;
    extract_json(str, strlen(str));

    ESP_LOGW(TAG, "the APP attributes are ");
    // show this json to user
    for (size_t i = 0; i < MAX_NO_OF_KEY_ATTR; i++)
    {
        printf("%s::%s ||", key_value[i][0], key_value[i][1]);
    }
}

/// @brief this is a very simple json extractor, make sure the string in json doesn't exceed 20 byte
/// @param str
/// @param size
static void extract_json(const char str[], uint16_t size)
{
    if (!str && size)
    {
        return;
    }
    static bool startjson, startstring;
    static bool next_pair, srch_value;

    static int next_index;
    static int iter;

    // reset all the values
    startjson = startstring = next_pair = srch_value = false;
    next_index = iter = 0;

    for (size_t i = 0; i < size; i++)
    {
        // search for the { --> start of json  "--> start of string  in the string
        if (str[i] == '{')
        {
            startjson = true;
            continue;
        }
        else if (str[i] == '}')
        {
            startjson = false;
            break;
        }

        if (startjson)
        {
            // find the start string char
            if (str[i] == '\"' || str[i] == '\'')
            {
                startstring = startstring ? 0 : 1;
                iter = 0;
                continue;
            }

            if (startstring)
            {
                // only store upto maxlen-2 cause at maxlen-1 == "\0"==0==static arr init value
                if (iter >= (MAX_LEN_OF_KEY_ATTR - 2))
                {
                    continue;
                }
                // record the value
                if (srch_value)
                {
                    key_value[next_index][1][iter++] = str[i];
                }
                // recored the key
                else
                {
                    key_value[next_index][0][iter++] = str[i];
                }
            }

            if (!startstring)
            { // search for colon, i.e we can search for the value of the key we just found
                if (str[i] == ':')
                {
                    srch_value = true;
                    continue;
                }

                // search for next pair, if exist
                if (srch_value)
                {
                    if (str[i] == ',')
                    {
                        // update to next index
                        next_index++;
                        srch_value = false;
                    }
                }
            }
        }
    }
}
