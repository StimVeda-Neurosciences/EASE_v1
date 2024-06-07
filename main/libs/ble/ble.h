#pragma once 

#include <stdio.h>
#include <stdint.h>
#include "esp_err.h"

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

#define GATT_MAX_ATTR_STRING_LEN 50

#define CHAR_DECLARATION_SIZE (sizeof(uint8_t))

#define DEVICE_NAME            "EASE"

#define GATTS_DEMO_CHAR_VAL_LEN_MAX 200

#define CHAR_VAL_SIZE (sizeof(uint8_t))



#define PROFILE_NUM 3

// #define PROFILE_A_ID 0

///////////////////device id profile 


#define PROFILE_DEVICE_INFORMATION 0

#define PROFILE_BATT_SERVICE 1

// #define PROFILE_FIRM_UPDATE 2

#define PROFILE_TDC_EEG 2


// #define SRVC_IS_ID 0

#define SERVICE_DEVICE_INFORMATION 0

#define SERVICE_BATT_SERVICE 1

// #define SERVICE_FIRM_UPDATE 2

#define SERVICE_TDC_EEG 2


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
  DEVICE_NUMBER_CHAR,
  DEVICE_NUMBER_VAL,
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
// #define DFU_CHAR_MX_SIZE 100
// enum 
// {
//   DFU_SERVICE,
//   DFU_CHAR,
//   DFU_VAL,
//   DFU_CHAR_CONFIG,
//   DFU_NO_OF_ELE

// };
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

typedef struct _MY_DATA_
{
  uint8_t *buff;
  uint16_t len;
}my_data;

// void example_write_event_env(esp_gatt_if_t gatts_if, prepare_type_env_t *prepare_write_env, esp_ble_gatts_cb_param_t *param);
// void example_exec_write_event_env(prepare_type_env_t *prepare_write_env, esp_ble_gatts_cb_param_t *param);

/// @brief send the ble  notification to eeg char 
/// @param data 
/// @param len 
/// @return err/sucess
esp_err_t esp_ble_send_notif_eeg(uint8_t  * data  , uint16_t len);

/// @brief send the tdcs current notification 
/// @param data 
/// @param len 
/// @return err/succ
esp_err_t esp_ble_send_notif_tdcs_curr(uint8_t * data, uint16_t len);

/// @brief send the battery data 
/// @param data 
/// @param len 
/// @return succ/err
esp_err_t esp_ble_send_battery_data(uint8_t * data, uint16_t len);

/// @brief send the err indication 
/// @param err 
/// @return succ/errcode
esp_err_t esp_ble_send_err_indication(uint8_t err);


/// @brief send the error array 
/// @param error 
/// @param size 
/// @return succ/failure
esp_err_t esp_ble_send_error_array(uint8_t *error, uint8_t size);

/// @brief send the status to ble  
/// @param status
/// @return succ/errcode 
esp_err_t esp_ble_send_status_indication(uint8_t status);



/// @brief to start the advertisement again 
/// @param  
void ble_start_advertise(void);

/// @brief to stop the advertisement 
/// @param  
void ble_stop_advertise(void);

/// @brief to disconnect the peer device 
/// @param  
void ble_disconnect_device(void);

/// @brief start the communcation 
/// @param taskhandle 
void ble_start_driver(void * taskhandle);

/// @brief init the ble functionality 
/// @param  
void ble_driver_init(void);

/// @brief deinitialise the BLE driver 
/// @param  
void ble_driver_deinit(void);
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

