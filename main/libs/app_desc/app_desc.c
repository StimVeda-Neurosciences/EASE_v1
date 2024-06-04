/****
 *
 *
 * may 06,2024
 *
 *
 */

#include <stdio.h>
#include <stdint.h>

#include "app_desc.h"

// the custom descriptor has to be inited at runtime
const __attribute__((section(".rodata_custom_desc"))) esp_app_custom_mem_desc_t app_custom_desc = {

  .app_desc =
    {
      .magic_number = APP_DESC_MAGIC_NO,
      .size = OFFSET_OF(esp_app_custom_desc_t, app_extra_size),

      /// header version
      .hdr_ver.ver_major = VERSION_MAJOR(APP_DESCRIPTOR_VERSION),
      .hdr_ver.ver_minor = VERSION_MINOR(APP_DESCRIPTOR_VERSION),
      .hdr_ver.ver_patch = VERSION_PATCH(APP_DESCRIPTOR_VERSION),
      .hdr_ver.ver_fix = VERSION_FIX(APP_DESCRIPTOR_VERSION),

      /// app version
      .app_ver.ver_major = VERSION_MAJOR(APP_VERSION),
      .app_ver.ver_minor = VERSION_MINOR(APP_VERSION),
      .app_ver.ver_patch = VERSION_PATCH(APP_VERSION),
      .app_ver.ver_fix = VERSION_FIX(APP_VERSION),

      .app_name = APP_NAME,

      .app_type = APP_TYPE_DFU_APP,
      .app_size = FLASH_EMPTY, // need a way to get the size of the app

      .app_verif_state = FLASH_EMPTY,
      .app_boot_state = FLASH_EMPTY,

      // as we are using 2 custom sizes
      .app_extra_size = 2 * APP_EXTRA_SIZE,
    },

  .app_extra_mem = {[0 ...(2 * APP_EXTRA_SIZE - 1)] = FLASH_EMPTY_U8},
};





// // the custom descriptor has to be inited at runtime
// const __attribute__((section(".rodata_custom_desc"))) esp_app_custom_mem_desc_t app_custom_desc =
// {
//     .mydesc = {
//         .magic_number = APP_DESC_MAGIC_NO,
//         .size = offsetof(esp_app_custom_desc_t,app_extra_size),

//         /// header version
//         .hdr_ver.ver_major = VERSION_MAJOR(APP_DESCRIPTOR_VERSION),
//         .hdr_ver.ver_minor = VERSION_MINOR(APP_DESCRIPTOR_VERSION),
//         .hdr_ver.ver_patch = VERSION_PATCH(APP_DESCRIPTOR_VERSION),
//         .hdr_ver.ver_fix   = VERSION_FIX(APP_DESCRIPTOR_VERSION),

//         /// app version
//         .app_ver.ver_major = VERSION_MAJOR(DFU_APP_VERSION),
//         .app_ver.ver_minor = VERSION_MINOR(DFU_APP_VERSION),
//         .app_ver.ver_patch = VERSION_PATCH(DFU_APP_VERSION),
//         .app_ver.ver_fix   = VERSION_FIX(DFU_APP_VERSION),

//         .app_name = DFU_APP_NAME,

//         .app_type = APP_TYPE_DFU_APP,
//         .app_size = FLASH_EMPTY, // need a way to get the size of the app

//         .app_verif_state = FLASH_EMPTY,
//         .app_boot_state = FLASH_EMPTY,

//         // as we are using 2 custom sizes
//         .app_extra_size = 2 * sizeof(_custom_data_t),

//         .extra_mem =
//             { [ESP_APP_DESC_STRUCT_SIZE ... (sizeof(app_custom_desc)-1) ] = FLASH_EMPTY_U8 },
//         // .app_crash = FLASH_EMPTY_U8
//     },
//     //    .memory =
//     // {
//     // },

//  (const esp_app_custom_desc_t) {
//     .magic_number = APP_DESC_MAGIC_NO,
//     .size = offsetof(esp_app_custom_desc_t,app_extra_size),

//     /// header version
//     .hdr_ver.ver_major = VERSION_MAJOR(APP_DESCRIPTOR_VERSION),
//     .hdr_ver.ver_minor = VERSION_MINOR(APP_DESCRIPTOR_VERSION),
//     .hdr_ver.ver_patch = VERSION_PATCH(APP_DESCRIPTOR_VERSION),
//     .hdr_ver.ver_fix   = VERSION_FIX(APP_DESCRIPTOR_VERSION),

//     /// app version
//     .app_ver.ver_major = VERSION_MAJOR(DFU_APP_VERSION),
//     .app_ver.ver_minor = VERSION_MINOR(DFU_APP_VERSION),
//     .app_ver.ver_patch = VERSION_PATCH(DFU_APP_VERSION),
//     .app_ver.ver_fix   = VERSION_FIX(DFU_APP_VERSION),

//     .app_name = DFU_APP_NAME,

//     .app_type = APP_TYPE_DFU_APP,
//     .app_size = FLASH_EMPTY, // need a way to get the size of the app

//     .app_verif_state = FLASH_EMPTY,
//     .app_boot_state = FLASH_EMPTY,

//     // as we are using 2 custom sizes
//     .app_extra_size = 2 * sizeof(_custom_data_t),

//     // .extra_mem = s
//     // .app_crash = FLASH_EMPTY_U8
// };
// };