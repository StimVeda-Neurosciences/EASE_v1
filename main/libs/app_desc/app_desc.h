#pragma once 

#include <stdio.h>
#include <stdint.h>

#include "flash_otameta_data.h"

// =================================================================================
// =================================================================================
#define APP_DESCRIPTOR_VERSION "1.0.0"

#define APP_VERSION "1.1.0"

#define APP_NAME "EASE_APP"

/// @brief extract the version from the app_descriptor version
#define VERSION_MAJOR(x) (x[0])
#define VERSION_MINOR(x) (x[2])
#define VERSION_PATCH(x) (x[4])
#define VERSION_FIX(x)   (0)

#define APP_NAME_MAX_LEN 32

/// define the magic number for the app descriptor
#define APP_DESC_MAGIC_NO 0xA5A5A0UL
#define FLASH_EMPTY       UINT32_MAX
#define FLASH_EMPTY_U8    UINT8_MAX

/// define the app type
#define APP_TYPE_DFU_APP  "DFU_APP"
#define APP_TYPE_EASE_APP "EASE_APP"

#define APP_EXTRA_SIZE (1024) // in bytes 


/// @brief app descriptor structure
typedef struct __ESP_APP_CUSTOM_DESCP__
{
    uint32_t magic_number;
    uint16_t size; // size would only be until app_extra_size

    /// @brief  the app descriptor header version
    ota_header_version_t hdr_ver;
    /// @brief app version
    ota_header_version_t app_ver;
    /// @brief app name is null terminated
    const char app_name[APP_NAME_MAX_LEN];
    // describe the app type, DFU, APP
    const char app_type[APP_NAME_MAX_LEN];
    uint32_t app_size;

    /// @brief app verification state same as app state in ota meta data
    uint32_t app_verif_state;
    /// @brief booting state of the app same as boot index
    uint32_t app_boot_state;

    // this contains the sizeof(app_rfu) + sizeof(app_crash) == (sizeof(anyone)/2)
    uint32_t app_extra_size;

    uint8_t extra_mem[0]; // flexible array member
} PACKED esp_app_custom_desc_t;

#define ESP_APP_DESC_STRUCT_SIZE (sizeof(esp_app_custom_desc_t))

typedef struct __ESP_APP_CUSTOM_MEMORY__
{
    esp_app_custom_desc_t app_desc;
    uint8_t app_extra_mem[APP_EXTRA_SIZE];
    // esp_app_custom_desc_t mydesc;
}PACKED esp_app_custom_mem_desc_t;

#define APP_DESCRIPTOR_SIZE (sizeof(esp_app_custom_mem_desc_t))



// this is the decleartion of the variable used by other files 
extern const esp_app_custom_mem_desc_t app_custom_desc;