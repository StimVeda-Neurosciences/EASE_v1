#pragma once 


#include <stdio.h>
#include <stdint.h>

#include "sys_attr.h"
#include "esp_err.h"

#include "flash_op_private.h"


/// @brief this is the value of the flash when empty
#define FLASH_EMPTY_VAL UINT32_MAX


/// @brief   app image state
typedef enum __OTA_APP_IMAGE_STATE__
{
    // not inited state
    OTA_IMAGE_STATE_EMPTY = UINT32_MAX,
    // verification state is pending , mark by DFU code process
    OTA_IMAGE_PENDING_VERIFY = 0xFEUL,
    OTA_IMAGE_START_VERIFICATION = 0xFCUL,   // this is bootloader verification
    OTA_IMAGE_PROCESS_VERIFICATION = 0xF8UL, // bootloader verification

    // errors 
    OTA_IMAGE_INVALID_HDR = 0xE8UL,
    OTA_IMAGE_CRASHED    =  0xE4UL,
    OTA_IMAGE_CORRUPTED   = 0xE2UL,
    OTA_IMAGE_NOT_PRESENT = 0xE1UL,
    // image verified 
    OTA_IMAGE_VERIFIED =  0x00UL

} ota_app_image_state_enum_t;

/// @brief  DFU process state
typedef enum __OTA_DFU_PROCESS_STATE__
{
    OTA_DFU_PROCESS_NOT_INITED = UINT32_MAX,
    OTA_DFU_PROCESS_START = 0xffffUL,
    OTA_DFU_PROCESS_PENDING = 0xffUL,
    OTA_DFU_PROCESS_ONGOING = 0x7fUL,

    // indicate that the DFU process is failed and
    OTA_DFU_PROCESS_FAILED = 0x7AUL,      // uniq no
    OTA_DFU_PROCESS_INTERRUPTED = 0x75UL, // uniq no
    OTA_DFU_PROCESS_DONE = 0x73UL,        // some number

} ota_dfu_process_state_enum_t;

/// @brief bootloader ota states
typedef struct __OTA_IMAGE_STATES__
{
    // app_image_state
    uint32_t app_image_state;
    // dfu_image_state
    uint32_t dfu_image_state;
    // dfu_process_state
    uint32_t dfu_process_state;
} PACKED ota_image_state_t;

/// @brief bootloader boot indexes for the images
typedef enum __OTA_BOOT_INDEXES__
{
    BOOT_INDEX_NONE = UINT32_MAX,
    BOOT_INDEX_INVALID = 0xFEUL,
    // image is ready to boot
    BOOT_INDEX_READY = 0xFCUL,

    /// @brief no need to boot it again,like dfu
    BOOT_INDEX_DONE = 0xF8UL,
    /// @brief image is corrupted, detected by bootloader, boot backup
    BOOT_INDEX_CORRUPTED = 0xF4UL,
    /// @brief image is creashed, detected by bootloader 
    BOOT_INDEX_CRASHED = 0xF2UL,
    /// @brief image is not found here
    BOOT_INDEX_NOT_PRESENT = 0xF1UL,
} ota_boot_indexes_enum_t;

/// @brief bootloader boot index state
typedef struct __OTA_BOOT_INDEX__
{
    // boot_dfu_image
    uint32_t dfu_image;
    // instead of booting dfu main , boot backup main
    uint32_t dfu_backup_image;

    // indicate whether to boot main image
    uint32_t main_image;
    // boot backup image
    uint32_t backup_image;

} PACKED ota_boot_index_t;

/// @brief the image index that the valid index should be less than either 0 or 1
#define OTA_IMAGE_INDEX_MAX 2

/// @brief store the image indexes format
typedef enum __OTA_IMAGE_INDEXS__
{
    OTA_IMAGE_INDEX_FIRST = 0x01UL,
    OTA_IMAGE_INDEX_SECOND = 0x10UL,
    OTA_IMAGE_INDEX_INVALID = 0xFFUL,
    OTA_IMAGE_INDEX_NOT_PRESENT = 0x80UL,
} bootloader_image_indexes_enum_t;

/// @brief bootloader image indexes
typedef struct __OTA_IMAGE_INDEXES__
{
    // main application indexes
    uint8_t main_app;
    uint8_t backup_app;

    // backup application indexes
    uint8_t dfu_image;
    uint8_t dfu_backup_image;
}PACKED ota_image_index_t;

/// @brief  @brief meta data header version
typedef struct __OTA_META_HEADER_VER__
{
    uint8_t ver_major;
    uint8_t ver_minor;
    uint8_t ver_patch;
    uint8_t ver_fix;
} PACKED ota_header_version_t;

/// @brief define the custom data header,this should be present at the end of the struct
typedef struct __OTA_CUSTOM_DATA_HDR__
{
    uint32_t magic_no;
    uint16_t size;
    // reserve 4 bytes 
    uint32_t reserverd[4];
    /// @brief dynamic size memory , not defined it as 0 as that would be FAM
    uint8_t data[0];
} PACKED ota_custom_data_t;


#define BOOTLOADER_BOOT_STRUCT_SIZE (sizeof(bootloader_boot_struct_t))
#define BOOT_OTA_CUSTOM_DATA_SIZE (sizeof(ota_custom_data_t))


/// @brief this is a function pointer that can recieve variable argument function
typedef int (*var_arg_func_ptr)(void* param, ...);

/// @brief execute a process to max num times if getting error, if succ then return
/// @param func_ptr
/// @param param
/// @param num
/// @return succ/failure
int boot_execute_process(var_arg_func_ptr, void* param, uint8_t num, ...);

/// @brief func-->the function to run , num--> num of tries, err--> variable to store error
#define BOOT_EXECUTE_PROCESS(func, num, err)                                                                                               \
    {                                                                                                                                      \
        uint8_t tries = num;                                                                                                           \
        while ((err != 0) && (tries-- > 0))                                                                                            \
        {                                                                                                                                  \
            err = func;                                                                                                                    \
        }                                                                                                                                  \
    }

/// @brief get the boot index from the meta data
#define BOOT_GET_INDEX(x) ((x == OTA_IMAGE_INDEX_FIRST) ? (0) : ((x == OTA_IMAGE_INDEX_SECOND) ? (1) : (OTA_IMAGE_INDEX_NOT_PRESENT)))



// define the magic number for the flash as 1 & 0 alternates
#define OTA_STRUCT_MAGIC_NO    0xA5A5UL
#define OTA_STRUCT_MAGIC_EMPTY UINT32_MAX

// this struct/meta data is dump to garbage
#define OTA_GRABAGE_DUMP 0x10UL
#define OTA_STRUCT_NEW   UINT32_MAX


/// @brief errors described as OTA errors 
typedef enum __OTA_ERRORS__
{
    OTA_ERR_BASE = ERR_OTA_ERR_BASE,

    // invalid error cases 
    OTA_ERR_IMAGE_INDEX_INVALID,
    OTA_ERR_BOOT_INDEX_INVALID,
    OTA_ERR_IMAGE_INAVLID,

    OTA_ERR_IMAGE_ABSENT,
    OTA_ERR_IMAGE_PENDING_VERIFY,
    
    OTA_ERR_BOOT_INDEX_MISMATCH,
    OTA_ERR_BOOT_NO_DFU_IMAGES,
}ota_errors_enum_t;


/// @brief bootloader ota structure
typedef struct __OTA_META_STRUCT__
{
    // magic number start has a specific value 
    uint32_t magic_number_start;
    /// @brief size in bytes 
    uint32_t size;
    // grabage
    uint32_t garbage_struct;

    // ota_header_version
    ota_header_version_t ota_header_ver;
    
    // boot index for the OTA
    ota_boot_index_t boot_index;
    
    // ota states
    ota_image_state_t state;

    // image indexes 
    ota_image_index_t image_index;

    // this magic number end has a specific value 
    uint32_t magic_number_end;

    // this data can be present here in flash , but not gaurente that it is valid 
    // there are some extra data that are appended  
    ota_custom_data_t extra_data;

} PACKED ota_metd_struct_t;


#define OTA_METAD_SIZE (sizeof(ota_metd_struct_t))

/// start and end offset of the meta data struct 
#define OTA_MODF_OFFS_START (OFFSET_OF(ota_metd_struct_t,boot_index))
#define OTA_MODF_OFFS_END   (OFFSET_OF(ota_metd_struct_t,state))


/// @brief write the OTA meta data into the ota meta sector
/// @param part_struct
/// @param data
/// @return err/succ
esp_err_t esp_flash_write_ota_data(const esp_flash_partition_struct_t* part_struct, const ota_metd_struct_t* data);

/// @brief erase a sector based on the sector number
/// @param part_struct
/// @param sect_no
/// @return succ/failure
esp_err_t esp_erase_ota_sectors(const esp_flash_partition_struct_t *part_struct, uint8_t sect_no);


/// @brief dump the ota data memory into serial terminal
/// @param part_struct
/// @param bytes
/// @return succ/failure
esp_err_t esp_flash_dump_ota_data(esp_flash_partition_struct_t *part_struct, uint8_t sect_no, uint16_t bytes);

/// @brief get the size used by OTA meta data
/// @param part_struct
/// @param sec_no
/// @param size_ptr
/// @return
esp_err_t esp_ota_get_used_size(const esp_flash_partition_struct_t *part_struct, uint8_t sec_no, uint32_t *size);

/// @brief write the ota meta data to the ota partition as fresh, i.e. erase all the data first and then write
/// @param part_struct
/// @param data
/// @return succ/failure
esp_err_t esp_write_ota_data_fresh(const esp_flash_partition_struct_t *part_struct, const ota_metd_struct_t *data);

/// @brief this function would overwrite/modify the fields of last stack element to the data provided.
/// @note this function also checks if the data provided can be writeable to flash as it can only do 1-->0 traslation
/// @param part_struct
/// @param data
/// @return succ/failure
esp_err_t esp_modify_ota_data(const esp_flash_partition_struct_t *part_struct, uint16_t offset, uint32_t data);

/// @brief read the ota meta data from the ota meta sector
/// @param part_struct
/// @param ata
/// @return err/succ
esp_err_t esp_read_flash_ota_data(const esp_flash_partition_struct_t *part_struct, ota_metd_struct_t *data);

/// @brief read the ota meta data from the ota meta sector
/// @param part_struct
/// @param ata
/// @return err/succ
esp_err_t esp_read_flash_ota_data_fromsector(const esp_flash_partition_struct_t *part_struct, uint8_t sec_no, ota_metd_struct_t *data);