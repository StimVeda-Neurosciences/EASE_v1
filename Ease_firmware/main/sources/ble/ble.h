#ifndef _BLE_H
#define _BLE_H


////////// esp32 BLE and gatt , gap specific API
#include "esp_gap_ble_api.h"
#include "esp_bt_defs.h"
#include "esp_bt_main.h"
#include "esp_bt.h"
#include "esp_gatts_api.h"
#include "esp_gatt_common_api.h"
#include "esp_wifi.h"

////// nvs lib
#include "nvs_flash.h"
#include "nvs.h"
#include "esp_spi_flash.h"


/////// esp32 bare metal supportive libs
#include "esp_system.h"

#include "esp_log.h"

#include "sdkconfig.h" /// this include all the esp32 settings and configurations that are configure through menuconfig

#define connected 1
#define disconnected 0


#define  CONN_SLAVE_LATENCY  4
#define  MAX_CONN_INT 0x10 // max_int = 0x7*1.25ms = 8.25ms
#define  MIN_CONN_INT 0x07// min_int = 0x10*1.25ms = 7.5ms
#define  CONN_SUPV_TIMEOUT 200  // timeout = 400*10ms = 4000ms


#define MAX_ATTRIBUTE_SIZE 100

#define STRING_BUFFER_SIZE 200

#define Gatt_perm_r_w (ESP_GATT_PERM_WRITE | ESP_GATT_PERM_READ)

#define ESP_ERR_GET_BIOS_ERR 0x01
/////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////commonly used macros ///////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////       string and values for the devcie information //////////////////////////


#define MANUFACTURER_NAME "Marbles.Health"

#define FIRMWARE_REV_STR "EA-v1.0.1"



#define GATTS_TAG "GATTS_DEMO"




#define CHAR_DECLARATION_SIZE (sizeof(uint8_t))

#define DEVICE_NAME            "EASE"
#define TEST_MANUFACTURER_DATA_LEN  17

#define GATTS_DEMO_CHAR_VAL_LEN_MAX 200

#define CHAR_VAL_SIZE (sizeof(uint8_t))


#define PREPARE_BUF_MAX_SIZE 1024



#define PROFILE_NUM 4

// #define PROFILE_A_ID 0

///////////////////device id profile 


#define PROFILE_DEVICE_INFORMATION 0

#define PROFILE_BATT_SERVICE 1

#define PROFILE_FIRM_UPDATE 2

#define PROFILE_TDC_EEG 3


// #define SRVC_IS_ID 0

#define SERVICE_DEVICE_INFORMATION 0

#define SERVICE_BATT_SERVICE 1

#define SERVICE_FIRM_UPDATE 2

#define SERVICE_TDC_EEG 3


///////////////////////////// enum to store attribute for device information////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////
enum
{
  DEVICE_INFO_SERVICE,
  MANUFACTURER_NAME_CHAR, 
  MANUFACTURER_NAME_VAL,
  SERIAL_NUMBER_CAHR,
  SERIAL_NUMBER_VAL,
  HARDWARE_REVISION_CHAR,
  HARDWARE_REVISION_VAL,
  FIRMWARE_REVISION_CHAR,
  FIRMWARE_REVISION_VAL,
  DEVICE_INFO_NO_OF_ELE
};

  ////////////////////////////////// enum to store the battery service profile //////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
#define BATTERY_PRES_FORMAT "Percentage 0 - 100"

enum
{
  BATTERY_SERVICE,
  BATTERY_CHAR,
  BATTERY_VAL,
  /////////// define the descriptors of the chars
  BATT_CHAR_PRES_FORMAT,
  BATT_CHAR_CONFIG,
  BATT_RAW_PARAM_CHAR,
  BATT_RAW_PARM_VAL,
  BATT_RAW_PARAM_CCFG,
  BATT_NO_OF_ELE

};

///////////////////////// device firmware update /////////////////////
/////////////////////////////////////////////////////////////////////////////////
#define DFU_CHAR_MX_SIZE 100
enum 
{
  DFU_SERVICE,
  DFU_CHAR,
  DFU_VAL,
  DFU_CHAR_CONFIG,
  DFU_NO_OF_ELE

};
///////////////// enum to srore attribute index value /////////////////////////
#define custom_char_mx_size 250

#define cstm_write_mx_size 20

enum
{
    CUSTOM_SERVICE,
    ELECTRODE_IMP_CHAR,
    ELECTRODE_IMP_VAL,
    ELECTRODE_IMP_CCFG,
    TDCS_SETT_CHAR,
    TDCS_SETT_VAL,
    TDCS_SETT_CCFG,
    EEG_DATA_CHAR,
    EEG_DATA_VAL,
    EEG_DATA_CCFG,
    EEG_SETT_CHAR,
    EEG_SETT_VAL,
    EEG_SETT_CCFG,
    ERROR_CODE_CHAR,
    ERROR_CODE_VAL,
    ERROR_CODE_CCFG,
    STATUS_CODE_CHAR,
    STATUS_CODE_VAL,
    STATUS_CODE_CCFG,
    CUSTOM_NO_ELE
    
};




///////////////////////////// structure handle for the PRofile 

struct gatts_profile_inst {
    esp_gatts_cb_t gatts_cb;
    uint16_t gatts_if;
    uint16_t app_id; // profile id 
    uint16_t conn_id;
    uint16_t service_handle;
    esp_gatt_srvc_id_t service_id;
    uint16_t char_handle;
    esp_bt_uuid_t char_uuid;
    esp_gatt_perm_t perm;
    esp_gatt_char_prop_t property;
    uint16_t descr_handle;
    esp_bt_uuid_t descr_uuid;
};

typedef struct _MY_DATA_
{
  uint8_t *buff;
  uint16_t len;
}my_data;

// void example_write_event_env(esp_gatt_if_t gatts_if, prepare_type_env_t *prepare_write_env, esp_ble_gatts_cb_param_t *param);
// void example_exec_write_event_env(prepare_type_env_t *prepare_write_env, esp_ble_gatts_cb_param_t *param);

void gatts_profile_a_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param);


void gatts_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param);

void gap_event_handler(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param);

/// @brief 
/// @param  
/// @param  
/// @return 
esp_err_t esp_ble_send_notif_eeg(uint8_t  * , uint16_t  );

/// @brief 
/// @param  
/// @param  
/// @return 
esp_err_t esp_ble_send_notif_tdcs_curr(uint8_t *, uint16_t );

/// @brief 
/// @param buff 
/// @param size 
/// @return 
esp_err_t esp_ble_send_battery_data(uint8_t * buff, uint8_t size);

/// @brief 
/// @param err 
/// @return 
esp_err_t esp_ble_send_err_indication(uint8_t err);

/// @brief this is to send the device status through ble indication 
/// @param status 
/// @return succ/Failuren 
esp_err_t esp_ble_send_status_indication(uint8_t status);

// to turn off the bt controller 
void bt_controller_off(void);

void ble_start_advertise(void);

// to turn off the bt controller 
void bt_controller_off(void);

/// @brief to start the advertisement again 
/// @param  
void ble_start_advertise(void);

/// @brief to stop the advertisement 
/// @param  
void ble_stop_advertise(void);

/// @brief to disconnect the peer device 
/// @param  
void ble_disconnect_device(void);

/// @brief init the ble functionality 
/// @param  
void ble_init(void);

//////// please do not touch this 

// ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// /* Service */
// //////uart gatt service uuid 64 bit
// const uint8_t serv_uuid[] =
//     {

//         0x9e,
//         0xca,
//         0xdc,
//         0x24,
//         0x0e,
//         0xe5,
//         0xa9,
//         0xe0,
//         0x93,
//         0xf3,
//         0xa3,
//         0xb5,
//         0x01,
//         0x00,
//         0x40,
//         0x6e

// }; // UART service UUID

// ////////// uart RX characteristic uuid 64 bit
// const uint8_t char_rx_uuid[] =
//     {

//         0x9e,
//         0xca,
//         0xdc,
//         0x24,
//         0x0e,
//         0xe5,
//         0xa9,
//         0xe0,
//         0x93,
//         0xf3,
//         0xa3,
//         0xb5,
//         0x02,
//         0x00,
//         0x40,
//         0x6e

// };

// ////// uart tx characteristic uuid 64 bit
// const uint8_t char_tx_uuid[] =
//     {

//         0x9e,
//         0xca,
//         0xdc,
//         0x24,
//         0x0e,
//         0xe5,
//         0xa9,
//         0xe0,
//         0x93,
//         0xf3,
//         0xa3,
//         0xb5,
//         0x03,
//         0x00,
//         0x40,
//         0x6e

// };

#endif