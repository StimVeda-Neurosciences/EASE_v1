#pragma once 


#include <stdio.h>
#include <stdint.h>

#include "sys_attr.h"

/// @brief this is the value of the flash when empty
#define FLASH_EMPTY_VAL UINT32_MAX

// ======================= define the error types here
typedef enum __BOOTLOADER_ERRORS__
{
    BOOT_ERR_BASE = 0xF0UL,

    // bootloader fial to load a image
    BOOT_ERR_BOOT_FAIL,
    BOOT_ERR_NO_MEMORY,
    BOOT_ERR_NO_DATA,
    BOOT_ERR_EMPTY_MEMORY,

    // resource type errors
    BOOT_ERR_NO_RESOURCES,
    BOOT_ERR_RESOURCE_BUSY,
    BOOT_ERR_API_ERR,
    // data is corrupted in flash/ram
    BOOT_ERR_DATA_CORRUPTED,

    // invalid type errors
    BOOT_ERR_INVALID_DATA,
    BOOT_ERR_INVALID_PARAM,
    BOOT_ERR_INVALID_OPERATION,
    BOOT_ERR_INVALID_STATE,

    // operation type error
    BOOT_ERR_OP_NOT_FINISHED,
    BOOT_ERR_OP_NOT_SUPPORTED,
    BOOT_ERR_OP_TIMEOUT,
    BOOT_ERR_OP_FAILED,

} bootloader_errors_types;

/// @brief   app image state
typedef enum __BOOTLOADER_OTA_IMAGE_STATE__
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

} bootloader_app_image_state;

/// @brief  DFU process state
typedef enum __BOOTLOADER_OTA_DFU_PROCESS_STATE__
{
    OTA_DFU_PROCESS_NOT_INITED = UINT32_MAX,
    OTA_DFU_PROCESS_START = 0xffffUL,
    OTA_DFU_PROCESS_PENDING = 0xffUL,
    OTA_DFU_PROCESS_ONGOING = 0x7fUL,

    // indicate that the DFU process is failed and
    OTA_DFU_PROCESS_FAILED = 0x7AUL,      // uniq no
    OTA_DFU_PROCESS_INTERRUPTED = 0x75UL, // uniq no
    OTA_DFU_PROCESS_DONE = 0x73UL,        // some number

} bootloader_ota_dfu_process_state;

/// @brief bootloader ota states
typedef struct __BOOTLOADER_OTA_STATES__
{
    // app_image_state
    uint32_t app_image_state;
    // dfu_image_state
    uint32_t dfu_image_state;
    // dfu_process_state
    uint32_t dfu_process_state;
} PACKED bootloader_image_state_t;

/// @brief bootloader boot indexes for the images
typedef enum __BOOTLOADER_BOOT_INDEXES__
{
    BOOTLOADER_BOOT_INDEX_NONE = UINT32_MAX,
    BOOTLOADER_BOOT_INDEX_INVALID = 0xFEUL,
    // image is ready to boot
    BOOTLOADER_BOOT_INDEX_READY = 0xFCUL,

    /// @brief no need to boot it again,like dfu
    BOOTLOADER_BOOT_INDEX_DONE = 0xF8UL,
    /// @brief image is corrupted, detected by bootloader, boot backup
    BOOTLOADER_BOOT_INDEX_CORRUPTED = 0xF4UL,
    /// @brief image is creashed, detected by bootloader 
    BOOTLOADER_BOOT_INDEX_CRASHED = 0xF2UL,
    /// @brief image is not found here
    BOOTLOADER_BOOT_INDEX_NOT_PRESENT = 0xF1UL,
} bootloader_boot_indexes;

/// @brief bootloader boot index state
typedef struct __BOOTLOADER_BOOT_INDEX__
{
    // boot_dfu_image
    uint32_t dfu_image;
    // instead of booting dfu main , boot backup main
    uint32_t dfu_backup_image;

    // indicate whether to boot main image
    uint32_t main_image;
    // boot backup image
    uint32_t backup_image;

} PACKED bootloader_boot_index_t;

/// @brief the image index that the valid index should be less than either 0 or 1
#define OTA_IMAGE_INDEX_MAX 2

/// @brief store the image indexes format
typedef enum __BOOTLOADER_IMAGE_INDEXS__
{
    OTA_IMAGE_INDEX_FIRST = 0x01UL,
    OTA_IMAGE_INDEX_SECOND = 0x10UL,
    OTA_IMAGE_INDEX_INVALID = 0xFFUL,
    OTA_IMAGE_INDEX_NOT_PRESENT = 0x80UL,
} bootloader_image_indexes;

/// @brief bootloader image indexes
typedef struct __BOOTLOADER_IMAGE_INDEXES__
{
    // main application indexes
    uint8_t main_app;
    uint8_t backup_app;

    // backup application indexes
    uint8_t dfu_image;
    uint8_t dfu_backup_image;
} bootloader_image_index_t;

/// @brief  @brief meta data header version
typedef struct __BOOTLOADER_OTA_META_HEADER_VER__
{
    uint8_t ver_major;
    uint8_t ver_minor;
    uint8_t ver_patch;
    uint8_t ver_fix;
} PACKED bootloader_header_version_t;

/// @brief define the custom data header,this should be present at the end of the struct
typedef struct __APP_CUSTOM_DATA_HDR__
{
    uint32_t magic_no;
    uint16_t size;
    uint8_t reserverd;
    /// @brief dynamic size memory , not defined it as 0 as that would be FAM
    uint8_t data[1];
} PACKED bootloader_custom_data_t;

/// define the meta data sectors
#define OTA_META_DATA_SECTOR0     0
#define OTA_META_DATA_SECTOR1     1
#define OTA_META_DATA_MAX_SECTORS 2

// define the max partitions for applications
#define BOOTLOADER_MAX_OTA_PARTITIONS 2
#define BOOTLOADER_MAX_DFU_PARTITIONS 2

// define the extra partition subtype
#define PART_SUBTYPE_DATA_BOOT_DUMP     0x25UL
#define PART_SUBTYPE_DATA_OTA_META_DATA 0x35UL

#define PART_SUBTYPE_APP_DFU_0 0x40UL
#define PART_SUBTYPE_APP_DFU_1 0x45UL
#define PART_SUBTYPE_APP_APP_0 0x50UL
#define PART_SUBTYPE_APP_APP_1 0x55UL


/// @brief  @brief bootloader structure for bootloader functioning
typedef struct __BOOTLOADER_STRUCTURE__
{
    // header information
    bootloader_header_version_t bootloader_header;

    /// @brief the below 3 items are given by OTA meta data
    // store the state of images
    bootloader_image_state_t state;
    // store the boot indexes for the image
    bootloader_boot_index_t boot_index;
    /// @brief store the image indexes
    bootloader_image_index_t image_index;

    // store the applications partition position
    esp_partition_pos_t ota_parts[BOOTLOADER_MAX_OTA_PARTITIONS];

    // store the dfu partition position
    esp_partition_pos_t dfu_parts[BOOTLOADER_MAX_DFU_PARTITIONS];

    // store the meta data partition address
    esp_partition_pos_t ota_meta_part[OTA_META_DATA_MAX_SECTORS];

    esp_partition_pos_t boot_dump_part;

} bootloader_boot_struct_t;

#define BOOTLOADER_BOOT_STRUCT_SIZE (sizeof(bootloader_boot_struct_t))

/// @brief this is a function pointer that can recieve variable argument function
typedef esp_err_t (*var_arg_func_ptr)(void* param, ...);

/// @brief execute a process to max num times if getting error, if succ then return
/// @param func_ptr
/// @param param
/// @param num
/// @return succ/failure
esp_err_t boot_execute_process(var_arg_func_ptr, void* param, uint8_t num, ...);

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

