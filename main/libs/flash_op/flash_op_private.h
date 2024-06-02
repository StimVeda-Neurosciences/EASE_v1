#pragma once 


// include the flash partitions 
#include "esp_flash_partitions.h"


/// define the meta data sectors
#define OTA_META_DATA_SECTOR0     0
#define OTA_META_DATA_SECTOR1     1
#define OTA_META_DATA_MAX_SECTORS 2

// define the max partitions for applications
#define MAX_OTA_PARTITIONS 2
#define MAX_DFU_PARTITIONS 2

// define the extra partition subtype
#define PART_SUBTYPE_DATA_BOOT_DUMP     0x25UL
#define PART_SUBTYPE_DATA_OTA_META_DATA 0x35UL

#define PART_SUBTYPE_APP_DFU_0 0x40UL
#define PART_SUBTYPE_APP_DFU_1 0x45UL
#define PART_SUBTYPE_APP_APP_0 0x50UL
#define PART_SUBTYPE_APP_APP_1 0x55UL


///=============== flash sector size 
#define FLASH_SECTOR_SIZE 0x1000
#define FLASH_BLOCK_SIZE 	0x10000
#define MMAP_ALIGNED_MASK 	(SPI_FLASH_MMU_PAGE_SIZE - 1)
#define MMU_FLASH_MASK    (~(SPI_FLASH_MMU_PAGE_SIZE - 1))

#define MAX_OTA_PARTITIONS 2 
#define MAX_APP_PARTITIONS 2

typedef struct __ESP_FLASH_PARTITIONS__
{
    esp_partition_pos_t  ota_meta_partition[MAX_OTA_PARTITIONS];
    esp_partition_pos_t  boot_dump_partition;
    esp_partition_pos_t  app_partition[MAX_APP_PARTITIONS];

}esp_flash_partition_struct_t;



/// @brief map the free pages 
/// @param  void 
/// @return free pages in the flash 
uint32_t esp_mmap_get_free_pages(void);


/// @brief map the flash addr to cache for cpu to read 
/// @param src_addr 
/// @param size 
/// @return succ/err
const void * esp_flash_mmap(uint32_t src_addr, uint16_t size);


/// @brief unmap the mapped flash address 
/// @param map_addr 
void esp_flash_unmap(const void * map_addr);


/// @brief this func read the partition table and init the 
/// @param  partition position structure
void esp_read_partition_table(esp_flash_partition_struct_t *part_struct);




// define the magic number for the flash as 1 & 0 alternates
#define BOOTLOADER_OTA_STRUCT_MAGIC_NO    0xA5A5UL
#define BOOTLOADER_OTA_STRUCT_MAGIC_EMPTY UINT32_MAX

// this struct/meta data is dump to garbage
#define BOOTLOADER_OTA_GRABAGE_DUMP 0x10UL
#define BOOTLAODER_OTA_STRUCT_NEW   UINT32_MAX


/// @brief errors described as OTA errors 
typedef enum __BOOTLOADER_OTA_ERRORS__
{
    BOOT_OTA_ERR_BASE = 0x00,

    // invalid error cases 
    BOOT_OTA_ERR_IMAGE_INDEX_INVALID,
    BOOT_OTA_ERR_BOOT_INDEX_INVALID,
    BOOT_OTA_ERR_IMAGE_INAVLID,

    BOOT_OTA_ERR_BOOT_INDEX_MISMATCH,

}bootloader_ota_errors_t;


/// @brief bootloader ota structure
typedef struct __BOOTLOADER_OTA_META_STRUCT__
{
    // magic number start has a specific value 
    uint32_t magic_number_start;
    /// @brief size in bytes 
    uint32_t size;
    // grabage
    uint32_t garbage_struct;

    // ota_header_version
    bootloader_header_version_t ota_header_ver;
    
    // boot index for the OTA
    bootloader_boot_index_t boot_index;
    
    // ota states
    bootloader_image_state_t state;

    // image indexes 
    bootloader_image_index_t image_index;

    // this magic number end has a specific value 
    uint32_t magic_number_end;

    // this data can be present here in flash , but not gaurente that it is valid 
    // there are some extra data that are appended  
    bootloader_custom_data_t extra_data;

} PACKED bootloader_ota_metd_struct_t;


#define BOOTLOADER_OTA_METAD_SIZE (sizeof(bootloader_ota_metd_struct_t))

/// start and end offset of the meta data struct 
#define BOOTLOADER_OTA_MODF_OFFS_START (OFFSET_OF(bootloader_ota_metd_struct_t,boot_index))
#define BOOTLOADER_OTA_MODF_OFFS_END   (OFFSET_OF(bootloader_ota_metd_struct_t,state))