#include "ble.h"

#include "system.h"

#include "fuel_guage.h"

#include "esp_partition.h"

#include "tdcs.h"

static uint8_t buff[STRING_BUFFER_SIZE];
static uint16_t meta_Data_len = 0;
static uint8_t hardware_number_len = 0;

// peer device bt addr
esp_bd_addr_t peer_addr;
uint8_t conn_flag = disconnected;
////////// this is the advertisement service uuid

static uint8_t adv_service_uuid128[32] = {
  /* LSB <--------------------------------------------------------------------------------> MSB */
  // first uuid, 16bit, [12],[13] is the value
  0xfb,
  0x34,
  0x9b,
  0x5f,
  0x80,
  0x00,
  0x00,
  0x80,
  0x00,
  0x10,
  0x00,
  0x00,
  0xEE,
  0x00,
  0x00,
  0x00,
  // second uuid, 32bit, [12], [13], [14], [15] is the value
  0xfb,
  0x34,
  0x9b,
  0x5f,
  0x80,
  0x00,
  0x00,
  0x80,
  0x00,
  0x10,
  0x00,
  0x00,
  0xFF,
  0x00,
  0x00,
  0x00,
};

// The length of adv data must be less than 31 bytes
// static uint8_t test_manufacturer[TEST_MANUFACTURER_DATA_LEN] =  {0x12, 0x23, 0x45, 0x56};
// adv data
static esp_ble_adv_data_t adv_data = {
  .set_scan_rsp = true,
  .include_name = true,
  .include_txpower = true,
  .min_interval = 0x0006, // slave connection min interval, Time = min_interval * 1.25 msec
  .max_interval = 0x0200, // slave connection max interval, Time = max_interval * 1.25 msec
  .appearance = 0x00,     ///// this is the icon appreance of the ble device in our case HID generic
  .manufacturer_len = 0,  // TEST_MANUFACTURER_DATA_LEN,
  .p_manufacturer_data = NULL,
  .service_data_len = 0,
  .p_service_data = NULL,
  .service_uuid_len = sizeof(adv_service_uuid128),
  .p_service_uuid = adv_service_uuid128,
  .flag = (ESP_BLE_ADV_FLAG_GEN_DISC | ESP_BLE_ADV_FLAG_BREDR_NOT_SPT),
};
// scan response data
static esp_ble_adv_data_t scan_rsp_data = {
  .set_scan_rsp = false,
  .include_name = true,
  .include_txpower = true,
  //.min_interval = 0x0006,
  //.max_interval = 0x0010,
  .appearance = ESP_BLE_APPEARANCE_GENERIC_HEART_RATE,
  .manufacturer_len = 0,       // TEST_MANUFACTURER_DATA_LEN,
  .p_manufacturer_data = NULL, //&test_manufacturer[0],
  .service_data_len = 0,
  .p_service_data = NULL,
  .service_uuid_len = sizeof(adv_service_uuid128),
  .p_service_uuid = adv_service_uuid128,
  .flag = (ESP_BLE_ADV_FLAG_GEN_DISC | ESP_BLE_ADV_FLAG_BREDR_NOT_SPT),
};

static esp_ble_adv_params_t adv_params = {
  .adv_int_min = 0x20,
  .adv_int_max = 0x40,
  .adv_type = ADV_TYPE_IND,
  .own_addr_type = BLE_ADDR_TYPE_PUBLIC,
  //.peer_addr            =
  //.peer_addr_type       =
  .channel_map = ADV_CHNL_ALL,
  .adv_filter_policy = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY,
};

///////////////////////////////------------------------------------------------------------
/////////////////////////////////----------------servie declaration , charcterstics declaration and descritpors uuid

static volatile uint8_t prof_id = 0;

static uint16_t connection_id = 0;

static const uint16_t primary_service_uuid = ESP_GATT_UUID_PRI_SERVICE;

static const uint16_t character_declaration_uuid = ESP_GATT_UUID_CHAR_DECLARE;

////////// characteristics descriptors uuid
static const uint16_t charcter_user_desrcriptor = ESP_GATT_UUID_CHAR_DESCRIPTION;
////////////// char config uuid
static const uint16_t client_charcater_config_uuid = ESP_GATT_UUID_CHAR_CLIENT_CONFIG;
/////////////////// char presentation format
// static const uint16_t character_presen_format = ESP_GATT_UUID_CHAR_PRESENT_FORMAT;
//////////////////////// char report refernce
// static const uint16_t char_report_reference = 0x2908;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////// characteristic properties
//////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static const uint8_t char_w_notify = ESP_GATT_CHAR_PROP_BIT_WRITE | ESP_GATT_CHAR_PROP_BIT_NOTIFY;
static const uint8_t char_prop_write = ESP_GATT_CHAR_PROP_BIT_WRITE;
// static const uint8_t char_prop_r_w = ESP_GATT_CHAR_PROP_BIT_READ | ESP_GATT_CHAR_PROP_BIT_WRITE;
static const uint8_t char_prop_read = ESP_GATT_CHAR_PROP_BIT_READ;
static const uint8_t char_prop_r_notify = ESP_GATT_CHAR_PROP_BIT_READ | ESP_GATT_CHAR_PROP_BIT_NOTIFY;

static const uint8_t char_prop_r_indicate = ESP_GATT_CHAR_PROP_BIT_READ | ESP_GATT_CHAR_PROP_BIT_INDICATE;
static const uint8_t char_prop_r_w_indicate = ESP_GATT_CHAR_PROP_BIT_WRITE | ESP_GATT_CHAR_PROP_BIT_READ | ESP_GATT_CHAR_PROP_BIT_INDICATE;
// static const uint8_t char_prop_r_w_notify = ESP_GATT_CHAR_PROP_BIT_NOTIFY | ESP_GATT_CHAR_PROP_BIT_READ | ESP_GATT_CHAR_PROP_BIT_WRITE;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////// device information uuid
///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static const uint16_t device_info_uuid = ESP_GATT_UUID_DEVICE_INFO_SVC;

static const uint16_t manufact_name_char = 0x2A29;
static const uint16_t serial_num_char = 0x2A25;
static const uint16_t hardware_rev_char = 0x2A27;
static const uint16_t firmware_rev_char = 0x2A26;

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////................ server database description for device
/// information..........................................

static const esp_gatts_attr_db_t __packed device_info_db_table[DEVICE_INFO_NO_OF_ELE] =
  ///////////// this data base table is const type
  {
    /////////// service declaration
    // Service Declaration
    [DEVICE_INFO_SERVICE] = {{ESP_GATT_AUTO_RSP},
                             {ESP_UUID_LEN_16, u8(primary_service_uuid), ESP_GATT_PERM_READ, _2bytes, _2bytes, u8(device_info_uuid)}},

    /* Characteristic Declaration */
    [MANUFACTURER_NAME_CHAR] = {{ESP_GATT_AUTO_RSP},
                                {ESP_UUID_LEN_16, u8(character_declaration_uuid), ESP_GATT_PERM_READ, _1byte, _1byte, u8(char_prop_read)}},

    /* Characteristic Value */ // respond by app
    [MANUFACTURER_NAME_VAL] = {{ESP_GATT_AUTO_RSP},
                               {ESP_UUID_LEN_16,
                                u8(manufact_name_char),
                                ESP_GATT_PERM_READ,
                                sizeof(MANUFACTURER_NAME),
                                sizeof(MANUFACTURER_NAME),
                                u8(MANUFACTURER_NAME)}},

    /* Characteristic Declaration */
    [SERIAL_NUMBER_CAHR] = {{ESP_GATT_AUTO_RSP},
                            {ESP_UUID_LEN_16, u8(character_declaration_uuid), ESP_GATT_PERM_READ, _1byte, _1byte, u8(char_prop_read)}},

    /* Characteristic value */
    [SERIAL_NUMBER_VAL] = {{ESP_GATT_RSP_BY_APP},
                           {ESP_UUID_LEN_16, u8(serial_num_char), ESP_GATT_PERM_READ, MAX_ATTRIBUTE_SIZE, MAX_ATTRIBUTE_SIZE, buff}},

    /* Characteristic declaration */ // respond by app
    [HARDWARE_REVISION_CHAR] = {{ESP_GATT_AUTO_RSP},
                                {ESP_UUID_LEN_16, u8(character_declaration_uuid), ESP_GATT_PERM_READ, _1byte, _1byte, u8(char_prop_read)}},

    /* character value  */
    [HARDWARE_REVISION_VAL] = {{ESP_GATT_RSP_BY_APP},
                               {ESP_UUID_LEN_16, u8(hardware_rev_char), ESP_GATT_PERM_READ, MAX_ATTRIBUTE_SIZE, MAX_ATTRIBUTE_SIZE, buff}},

    ///////characteristic declrartion
    [FIRMWARE_REVISION_CHAR] = {{ESP_GATT_AUTO_RSP},
                                {ESP_UUID_LEN_16, u8(character_declaration_uuid), ESP_GATT_PERM_READ, _1byte, _1byte, u8(char_prop_read)}},

    /////////// charcteristic value
    [FIRMWARE_REVISION_VAL] = {{ESP_GATT_AUTO_RSP},
                               {ESP_UUID_LEN_16,
                                u8(firmware_rev_char),
                                ESP_GATT_PERM_READ,
                                sizeof(FIRMWARE_REV_STR),
                                sizeof(FIRMWARE_REV_STR),
                                u8(FIRMWARE_REV_STR)}}

};



static uint16_t device_info_db_handle[DEVICE_INFO_NO_OF_ELE];

static uint16_t gatt_if_device_info = ESP_GATT_IF_NONE;
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////------------------- battery profile
////////////////////////////////////////////////////////////////////////////

static const uint16_t battery_service = ESP_GATT_UUID_BATTERY_SERVICE_SVC;
static const uint16_t battery_char = ESP_GATT_UUID_BATTERY_LEVEL;

static const uint16_t batt_raw_char = 0xff00;
static const uint16_t batt_char_config = 0x00;

uint8_t batt_val = 23;
////////////////////////////////////////////////////  battery database table
////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static const esp_gatts_attr_db_t battery_db_table[BATT_NO_OF_ELE] = {
  // Service Declaration
  [BATTERY_SERVICE] = {{ESP_GATT_AUTO_RSP},
                       {ESP_UUID_LEN_16, u8(primary_service_uuid), ESP_GATT_PERM_READ, _2bytes, _2bytes, u8(battery_service)}},

  /* Characteristic Declaration */
  [BATTERY_CHAR] = {{ESP_GATT_AUTO_RSP},
                    {ESP_UUID_LEN_16, u8(character_declaration_uuid), ESP_GATT_PERM_READ, _1byte, _1byte, u8(char_prop_r_indicate)}},

  /* Characteristic Value */ // respond by app
  [BATTERY_VAL] = {{ESP_GATT_RSP_BY_APP}, {ESP_UUID_LEN_16, u8(battery_char), ESP_GATT_PERM_READ, sizeof(uint32_t), _1byte, u8(batt_val)}},

  /////////// battery presenatation format (output value unit )
  [BATT_CHAR_PRES_FORMAT] = {{ESP_GATT_AUTO_RSP},
                             {ESP_UUID_LEN_16,
                              u8(charcter_user_desrcriptor),
                              ESP_GATT_PERM_READ,
                              sizeof(BATTERY_PRES_FORMAT),
                              sizeof(BATTERY_PRES_FORMAT),
                              u8(BATTERY_PRES_FORMAT)}},

  ///////// battery client char config
  [BATT_CHAR_CONFIG] = {{ESP_GATT_AUTO_RSP},
                        {ESP_UUID_LEN_16, u8(client_charcater_config_uuid), Gatt_perm_r_w, _2bytes, _2bytes, u8(batt_char_config)}},

  ///////////// character declartaion battery custom paramter
  [BATT_RAW_PARAM_CHAR] = {{ESP_GATT_AUTO_RSP},
                           {ESP_UUID_LEN_16, u8(character_declaration_uuid), ESP_GATT_PERM_READ, _1byte, _1byte, u8(char_prop_r_notify)}},

  ////////// battery raw parameter values
  [BATT_RAW_PARM_VAL] = {{ESP_GATT_RSP_BY_APP},
                         {ESP_UUID_LEN_16, u8(batt_raw_char), ESP_GATT_PERM_READ, batt_param_max_size, _1byte, u8(batt_val)}},

  ///////// battery raw paramter  client char config
  [BATT_RAW_PARAM_CCFG] = {{ESP_GATT_AUTO_RSP},
                           {ESP_UUID_LEN_16, u8(client_charcater_config_uuid), Gatt_perm_r_w, _2bytes, _2bytes, u8(batt_char_config)}},

};

static uint16_t battery_db_handle[BATT_NO_OF_ELE];

static uint16_t gatt_if_batt_srvc = ESP_GATT_IF_NONE;
///////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////------------ device firmware update ----------------/////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////

static const uint8_t dfu_serv[] = {0x50, 0xea, 0xda, 0x30, 0x88, 0x83, 0xb8, 0x9f, 0x60, 0x4f, 0x15, 0xf3, 0x01, 0x00, 0x40, 0x8e};

static const uint8_t dfu_char[] = {0x50, 0xea, 0xda, 0x30, 0x88, 0x83, 0xb8, 0x9f, 0x60, 0x4f, 0x15, 0xf3, 0x01, 0x00, 0x40, 0x8e};

static uint32_t dfu_val = 0;
static uint16_t dfu_client_char_config = 0;

static const esp_gatts_attr_db_t dfu_ota_db[DFU_NO_OF_ELE] = {
  // Service Declaration
  [DFU_SERVICE] = {{ESP_GATT_AUTO_RSP},
                   {ESP_UUID_LEN_16, u8(primary_service_uuid), ESP_GATT_PERM_READ, sizeof(dfu_serv), sizeof(dfu_serv), dfu_serv}},

  /* Characteristic Declaration */
  [DFU_CHAR] = {{ESP_GATT_AUTO_RSP},
                {ESP_UUID_LEN_16, u8(character_declaration_uuid), ESP_GATT_PERM_READ, _1byte, _1byte, u8(char_w_notify)}},

  /* Characteristic Value */ // respond by app
  [DFU_VAL] = {{ESP_GATT_RSP_BY_APP}, {ESP_UUID_LEN_128, dfu_char, ESP_GATT_PERM_READ, DFU_CHAR_MX_SIZE, sizeof(dfu_val), u8(dfu_val)}},
  ///////////////char config for DFU service
  [DFU_CHAR_CONFIG] = {{ESP_GATT_AUTO_RSP},
                       {ESP_UUID_LEN_16, u8(client_charcater_config_uuid), Gatt_perm_r_w, _2bytes, _2bytes, u8(dfu_client_char_config)}}

};

static uint16_t dfu_db_handle[DFU_NO_OF_ELE];

static uint16_t gatt_if_dfu_srv = ESP_GATT_IF_NONE;

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////--------------custom service for our job -------------////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////

static const uint16_t custom_serv = 0xff01;

static const uint16_t custom_char1 = 0xff02;
static const uint16_t custom_char2 = 0xff03;
static const uint16_t custom_char3 = 0xff04;
static const uint16_t custom_char4 = 0xff05;
static const uint16_t custom_char5 = 0xff06;
static const uint16_t custom_char6 = 0xff07;

uint32_t value = 0;

static uint16_t ccfg_val = 0;
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////..............server database description for the objective ................................
static const esp_gatts_attr_db_t custom_db_table[CUSTOM_NO_ELE] = {
  /////////// service declaration
  // Service Declaration
  [CUSTOM_SERVICE] = {{ESP_GATT_AUTO_RSP},
                      {ESP_UUID_LEN_16, u8(primary_service_uuid), ESP_GATT_PERM_READ, _2bytes, _2bytes, u8(custom_serv)}},

  /* Characteristic Declaration  electrode impedance */
  [ELECTRODE_IMP_CHAR] = {{ESP_GATT_AUTO_RSP},
                          {ESP_UUID_LEN_16, u8(character_declaration_uuid), ESP_GATT_PERM_READ, _1byte, _1byte, u8(char_prop_r_notify)}},

  /* Characteristic Value */ // respond by app
  [ELECTRODE_IMP_VAL] = {{ESP_GATT_RSP_BY_APP},
                         {ESP_UUID_LEN_16, u8(custom_char1), ESP_GATT_PERM_READ, custom_char_mx_size, sizeof(value), u8(value)}},

  /* Client Characteristic Configuration Descriptor */
  [ELECTRODE_IMP_CCFG] =
    {{ESP_GATT_AUTO_RSP},
     {ESP_UUID_LEN_16, u8(client_charcater_config_uuid), Gatt_perm_r_w, sizeof(ccfg_val), sizeof(ccfg_val), u8(ccfg_val)}},

  /* Characteristic Declaration  tdcs setting  */
  [TDCS_SETT_CHAR] = {{ESP_GATT_AUTO_RSP},
                      {ESP_UUID_LEN_16, u8(character_declaration_uuid), ESP_GATT_PERM_READ, _1byte, _1byte, u8(char_prop_write)}},

  /* Characteristic Value */ // respond by app
  [TDCS_SETT_VAL] = {{ESP_GATT_RSP_BY_APP},
                     {ESP_UUID_LEN_16, u8(custom_char2), ESP_GATT_PERM_WRITE, cstm_write_mx_size, sizeof(value), u8(value)}},

  /* Client Characteristic Configuration Descriptor */
  [TDCS_SETT_CCFG] = {{ESP_GATT_AUTO_RSP},
                      {ESP_UUID_LEN_16, u8(client_charcater_config_uuid), Gatt_perm_r_w, sizeof(ccfg_val), sizeof(ccfg_val), u8(ccfg_val)}},

  /* Characteristic Declaration  eeg data read*/
  [EEG_DATA_CHAR] = {{ESP_GATT_AUTO_RSP},
                     {ESP_UUID_LEN_16, u8(character_declaration_uuid), ESP_GATT_PERM_READ, _1byte, _1byte, u8(char_prop_r_notify)}},

  /* Characteristic Value */ // respond by app
  [EEG_DATA_VAL] = {{ESP_GATT_RSP_BY_APP},
                    {ESP_UUID_LEN_16, u8(custom_char3), ESP_GATT_PERM_READ, custom_char_mx_size, sizeof(value), u8(value)}},

  /* Client Characteristic Configuration Descriptor */
  [EEG_DATA_CCFG] = {{ESP_GATT_AUTO_RSP},
                     {ESP_UUID_LEN_16, u8(client_charcater_config_uuid), Gatt_perm_r_w, sizeof(ccfg_val), sizeof(ccfg_val), u8(ccfg_val)}},

  /* Characteristic Declaration  eeg settings  */
  [EEG_SETT_CHAR] = {{ESP_GATT_AUTO_RSP},
                     {ESP_UUID_LEN_16, u8(character_declaration_uuid), ESP_GATT_PERM_READ, _1byte, _1byte, u8(char_prop_write)}},

  /* Characteristic Value */ // respond by app
  [EEG_SETT_VAL] = {{ESP_GATT_RSP_BY_APP},
                    {ESP_UUID_LEN_16, u8(custom_char4), ESP_GATT_PERM_WRITE, cstm_write_mx_size, sizeof(value), u8(value)}},

  /* Client Characteristic Configuration Descriptor */
  [EEG_SETT_CCFG] = {{ESP_GATT_AUTO_RSP},
                     {ESP_UUID_LEN_16, u8(client_charcater_config_uuid), Gatt_perm_r_w, sizeof(ccfg_val), sizeof(ccfg_val), u8(ccfg_val)}},

  /* Characteristic Declaration  error code */
  [ERROR_CODE_CHAR] = {{ESP_GATT_AUTO_RSP},
                       {ESP_UUID_LEN_16, u8(character_declaration_uuid), ESP_GATT_PERM_READ, _1byte, _1byte, u8(char_prop_r_w_indicate)}},

  /* Characteristic Value */ // respond by app
  [ERROR_CODE_VAL] = {{ESP_GATT_RSP_BY_APP}, {ESP_UUID_LEN_16, u8(custom_char5), Gatt_perm_r_w, _10bytes, sizeof(value), u8(value)}},

  /* Client Characteristic Configuration Descriptor */
  [ERROR_CODE_CCFG] =
    {{ESP_GATT_AUTO_RSP},
     {ESP_UUID_LEN_16, u8(client_charcater_config_uuid), Gatt_perm_r_w, sizeof(ccfg_val), sizeof(ccfg_val), u8(ccfg_val)}},

  /* Characteristic Declaration  status code*/
  [STATUS_CODE_CHAR] = {{ESP_GATT_AUTO_RSP},
                        {ESP_UUID_LEN_16, u8(character_declaration_uuid), ESP_GATT_PERM_READ, _1byte, _1byte, u8(char_prop_r_w_indicate)}},

  /* Characteristic Value */ //  status of the device
  [STATUS_CODE_VAL] = {{ESP_GATT_RSP_BY_APP}, {ESP_UUID_LEN_16, u8(custom_char6), Gatt_perm_r_w, _10bytes, sizeof(value), u8(value)}},

  /* Client Characteristic Configuration Descriptor */
  [STATUS_CODE_CCFG] = {{ESP_GATT_AUTO_RSP},
                        {ESP_UUID_LEN_16,
                         u8(client_charcater_config_uuid),
                         ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
                         sizeof(ccfg_val),
                         sizeof(ccfg_val),
                         u8(ccfg_val)}},

};

static uint16_t custom_db_handle[CUSTOM_NO_ELE]; ///////this will create store  handler for the every charcteristic and attributes

static uint16_t gatt_if_custom = ESP_GATT_IF_NONE;
/////////////////// this is the var in which ble data is stored

static uint8_t adv_config_done = 0;

/////////////////////////////////////////////// //////
// this value stores the input value from the smartphone

#define adv_config_flag      (1 << 0)
#define scan_rsp_config_flag (1 << 1)

// my_data ble_data;

// void example_write_event_env(esp_gatt_if_t gatts_if, my_data *data, esp_ble_gatts_cb_param_t *param)
// {

//     esp_gatt_status_t status = ESP_GATT_OK;
//     if (param->write.need_rsp)
//     {
//         if (param->write.is_prep)
//         {
//             if (data->buff == NULL)
//             {
//                 data->buff = (uint8_t *)calloc((param->write.len), sizeof(uint8_t));
//                 data->len = 0;
//                 if (data->buff == NULL)
//                 {
//                     ESP_LOGE(GATTS_TAG, "Gatt_server prep no mem\n");
//                     status = ESP_GATT_NO_RESOURCES;
//                 }
//             }
//             else
//             {
//                 if (param->write.offset > PREPARE_BUF_MAX_SIZE)
//                 {
//                     status = ESP_GATT_INVALID_OFFSET;
//                 }
//                 else if ((param->write.offset + param->write.len) > PREPARE_BUF_MAX_SIZE)
//                 {
//                     status = ESP_GATT_INVALID_ATTR_LEN;
//                 }
//             }

//             esp_gatt_rsp_t *gatt_rsp = (esp_gatt_rsp_t *)malloc(sizeof(esp_gatt_rsp_t));
//             gatt_rsp->attr_value.len = param->write.len;
//             gatt_rsp->attr_value.handle = param->write.handle;
//             gatt_rsp->attr_value.offset = param->write.offset;
//             gatt_rsp->attr_value.auth_req = ESP_GATT_AUTH_REQ_NONE;
//             memcpy(gatt_rsp->attr_value.value, param->write.value, param->write.len);
//             //////////// send the BLE response
//             esp_err_t response_err = esp_ble_gatts_send_response(gatts_if, param->write.conn_id, param->write.trans_id, status,
//             gatt_rsp); if (response_err != ESP_OK)
//             {
//                 ESP_LOGE(GATTS_TAG, "Send response error\n");
//             }
//             free(gatt_rsp);
//             if (status != ESP_GATT_OK)
//             {
//                 return;
//             }
//             memcpy(data->buff + param->write.offset, param->write.value, param->write.len);
//             data->len += param->write.len;
//         }
//         else
//         {
//         }
//     }
// }

// void example_exec_write_event_env(my_data *data, esp_ble_gatts_cb_param_t *param)
// {
//     if (param->exec_write.exec_write_flag == ESP_GATT_PREP_WRITE_EXEC)
//     {
//         esp_log_buffer_hex(GATTS_TAG, data->buff, data->len);
//     }
//     else
//     {
//         ESP_LOGI(GATTS_TAG, "ESP_GATT_PREP_WRITE_CANCEL");
//     }
//     if (data->buff != NULL)
//     {
//         free(data->buff);
//         data->buff = NULL;
//     }
//     data->len = 0;
// }

////////////////////////// gatt event handler  //////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void gatts_events_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t* param) {

  /// variable for the functions
  // define the variables for the error and status codes
  static uint8_t status = status_idle;
  static uint32_t errorcode = error_none;

  switch (event) {
    case ESP_GATTS_REG_EVT: /////////// this is the register event we have to create a service table here
    {                       /////// you cannot declare a variable after case thats why a new scope if inserted here
      if (prof_id == SERVICE_DEVICE_INFORMATION) {

        esp_err_t set_dev_name_ret = esp_ble_gap_set_device_name(DEVICE_NAME);
        if (set_dev_name_ret) {
          ESP_LOGE(GATTS_TAG, "set device name failed, error code = %x", set_dev_name_ret);
        }
        // config adv data
        esp_err_t ret = esp_ble_gap_config_adv_data(&adv_data);
        if (ret) {
          ESP_LOGE(GATTS_TAG, "config adv data failed, error code = %x", ret);
        }
        adv_config_done |= adv_config_flag;
        // config scan response data
        ret = esp_ble_gap_config_adv_data(&scan_rsp_data);
        if (ret) {
          ESP_LOGE(GATTS_TAG, "config scan response data failed, error code = %x", ret);
        }

        adv_config_done |= scan_rsp_config_flag;

        if (gatts_if != 0)
          gatt_if_device_info = gatts_if;

        if (esp_ble_gatts_create_attr_tab(device_info_db_table, gatts_if, DEVICE_INFO_NO_OF_ELE, SERVICE_DEVICE_INFORMATION) != ESP_OK) {
          ESP_LOGE(GATTS_TAG, "failed attr table for device information service \r\n");
        }
      }

      else if (prof_id == PROFILE_BATT_SERVICE) {
        if (gatts_if != gatt_if_device_info)
          gatt_if_batt_srvc = gatts_if;
        if (esp_ble_gatts_create_attr_tab(battery_db_table, gatts_if, BATT_NO_OF_ELE, SERVICE_BATT_SERVICE) != ESP_OK) {
          ESP_LOGE(GATTS_TAG, "failed attr table for battery  service \r\n");
        }
      }

      else if (prof_id == PROFILE_FIRM_UPDATE) {
        if (gatts_if != 0 && gatts_if != gatt_if_batt_srvc)
          gatt_if_dfu_srv = gatts_if;
        if (esp_ble_gatts_create_attr_tab(dfu_ota_db, gatts_if, DFU_NO_OF_ELE, SERVICE_FIRM_UPDATE) != ESP_OK) {
          ESP_LOGE(GATTS_TAG, "failed attr table for DFU service \r\n");
        }
      }

      else if (prof_id == PROFILE_TDC_EEG) {
        if (gatts_if != 0 && gatts_if != gatt_if_dfu_srv)
          gatt_if_custom = gatts_if;
        if (esp_ble_gatts_create_attr_tab(custom_db_table, gatts_if, CUSTOM_NO_ELE, SERVICE_TDC_EEG) != ESP_OK) {
          ESP_LOGE(GATTS_TAG, "failed attr table for custom service \r\n");
        }
      }
      prof_id++;
    } break;
    case ESP_GATTS_READ_EVT: {
      ESP_LOGI(GATTS_TAG,
               "GATT_READ_EVT, gatt_interface  %d, trans_id %d, handle %d\n",
               gatts_if,
               param->read.trans_id,
               param->read.handle);
      esp_gatt_rsp_t rsp = {0};

      if (gatts_if == gatt_if_batt_srvc) {
        if (param->read.handle == battery_db_handle[BATTERY_VAL]) {

          rsp.attr_value.value[0] = batt_data.soc;
          rsp.attr_value.value[1] = batt_data.charging_status;

          rsp.attr_value.len = 2;
        } else if (param->read.handle == battery_db_handle[BATT_RAW_PARM_VAL]) {

          //// get the device status
          if (get_no_item_stat_q() > 0) {
            status = pop_stat_q();
          }
          batt_data.ease_state = status;

          rsp.attr_value.len = s(batt_data);
          memcpy(rsp.attr_value.value, (uint8_t*) &batt_data, s(batt_data));
        }
      }

      //////// Read event for the device information profile
      else if (gatts_if == gatt_if_device_info) {
        if (param->read.handle == device_info_db_handle[HARDWARE_REVISION_VAL]) {
          rsp.attr_value.len = hardware_number_len;
          memcpy(rsp.attr_value.value, buff, hardware_number_len);
        } else if (param->read.handle == device_info_db_handle[SERIAL_NUMBER_VAL]) {
          rsp.attr_value.len = meta_Data_len - hardware_number_len;
          memcpy(rsp.attr_value.value, buff + hardware_number_len + 1, meta_Data_len - hardware_number_len);
        }
      }

      else if (gatts_if == gatt_if_custom) {

        if (get_no_item_stat_q() > 0) {
          status = pop_stat_q();
        }
        if (get_no_item_err_q() > 0) {
          errorcode = pop_err_q();
        }
        if (param->read.handle == custom_db_handle[STATUS_CODE_VAL]) {
          rsp.attr_value.value[0] = status;
          rsp.attr_value.len = 1;
        } else if (param->read.handle == custom_db_handle[ERROR_CODE_VAL]) {
          rsp.attr_value.len = s(errorcode);
          memcpy(rsp.attr_value.value, &errorcode, s(errorcode));

          //////////// clear the error code afterwards
          errorcode = error_none;
        }
      }

      rsp.attr_value.handle = param->read.handle;
      esp_ble_gatts_send_response(gatts_if, param->read.conn_id, param->read.trans_id, ESP_GATT_OK, &rsp);
      break;
    }
    case ESP_GATTS_WRITE_EVT: /////////// gatt write event it is value recieve by the esp32
      if (!param->write.is_prep) {

        // the data length of gattc write  must be less than GATTS_DEMO_CHAR_VAL_LEN_MAX.
        ESP_LOGI(GATTS_TAG, "GATT_WRITE_EVT, handle = %d, value len = %d, value :\r\n", param->write.handle, param->write.len);
        esp_log_buffer_hex(GATTS_TAG, param->write.value, param->write.len);
        //   check the data for the attribute values

        if (custom_db_handle[EEG_SETT_VAL] == param->write.handle) {
          func_eeg_task data_eeg = {0};

          memcpy(&data_eeg, param->write.value, min(param->write.len, s(data_eeg)));

          //// reset the message buffer and then send the data
          reset_msg_buffer();
          push_msg_buff(u8(data_eeg), s(data_eeg));

          ////// send the message to the task
          ////// notify the task when ble data arrives
          if (data_eeg.rate == 1) {
            xTaskNotify(general_tsk_handle, STOP, eSetValueWithOverwrite);
          } else {
            xTaskNotify(general_tsk_handle, RUN_EEG, eSetValueWithOverwrite);
          }
        }

        else if (custom_db_handle[TDCS_SETT_VAL] == param->write.handle) {
          funct_tdcs_data data_tdcs = {0};

          memcpy(&data_tdcs, param->write.value, min(param->write.len, s(data_tdcs)));

          reset_msg_buffer();
          push_msg_buff(u8(data_tdcs), s(data_tdcs));

          ////// send the message to the task
          if ((data_tdcs.opcode == 8) || ((data_tdcs.opcode == 9))) {
            xTaskNotify(general_tsk_handle, STOP, eSetValueWithOverwrite);
          } else {
            xTaskNotify(general_tsk_handle, RUN_TDCS, eSetValueWithOverwrite);
          }
          ////// notify the task when ble data arrives
        }

        else if (custom_db_handle[STATUS_CODE_VAL] == param->write.handle) {
          if (param->write.value[0] == status_pwr_save) {
            printf("shuting down system\r\n");
            system_shutdown();
          }
        }

        else if (custom_db_handle[ERROR_CODE_VAL] == param->write.handle) {
          if (param->write.value[0] == ESP_ERR_GET_BIOS_ERR) {
            printf("getting bios errr\r\n");

            extern uint32_t device_run_bios_test(void);
            errorcode = device_run_bios_test();
          }
        }
        // data = atoi(data_val);
        // uint8_t value = atoi(data_val);
        //     if(value == 5)
        //     {
        // //// give the saved data back to the user
        //     uint64_t timeofeeg = get_eeg_time();
        //     uint64_t timeoftdcs = get_tdcs_time();
        //     int errcode = get_err_code();

        //     char retbuff[25];

        //     sprintf(retbuff,"EEG-%lld,TDCS-%lld,ERROR-%d\r\n",timeofeeg,timeoftdcs,errcode );
        //     send_notif((uint8_t *)retbuff, strlen(retbuff));
        //     }
        /////////////////////////////////////
        //////////////////////////////////////////////////////////////////

        // ///// always send okay reply :)
        esp_ble_gatts_send_response(gatts_if, param->write.conn_id, param->write.trans_id, ESP_GATT_OK, NULL);
      }
      break;

    case ESP_GATTS_EXEC_WRITE_EVT:
      ESP_LOGI(GATTS_TAG, "ESP_GATTS_EXEC_WRITE_EVT\r\n");
      // esp_ble_gatts_send_response(gatts_if, param->write.conn_id, param->write.trans_id, ESP_GATT_OK, NULL);
      // example_exec_write_event_env(&ble_data, param);
      break;
    case ESP_GATTS_MTU_EVT:
      ESP_LOGI(GATTS_TAG, "ESP_GATTS_MTU_EVT, MTU %d\r\n", param->mtu.mtu);
      break;
    case ESP_GATTS_CONF_EVT:
      // ESP_LOGI(GATTS_TAG, "ESP_GATTS_CONF_EVT, status = %d, attr_handle %d", param->conf.status, param->conf.handle);
      break;
    case ESP_GATTS_START_EVT:
      ESP_LOGI(GATTS_TAG, "SERVICE_START_EVT, status %d, service_handle %d\r\n", param->start.status, param->start.service_handle);
      break;
    case ESP_GATTS_CONNECT_EVT:
      ///////////// connection update have to be done only once
      if (gatts_if == gatt_if_device_info) {
        ESP_LOGI(GATTS_TAG, "connect the devicee , conn_id = %d\r\n", param->connect.conn_id);

        esp_log_buffer_hex(GATTS_TAG, param->connect.remote_bda, sizeof(esp_bd_addr_t));
        esp_ble_conn_update_params_t conn_params = {0};
        memcpy(conn_params.bda, param->connect.remote_bda, sizeof(esp_bd_addr_t)); /// copy the bluetooth device address
        /* For the iOS system, please refer to Apple official documents about the BLE connection parameters restrictions. */
        conn_params.latency = CONN_SLAVE_LATENCY;
        conn_params.max_int = MAX_CONN_INT;      // max_int = 0x7*1.25ms = 8.25ms
        conn_params.min_int = MIN_CONN_INT;      // min_int = 0x10*1.25ms = 7.5ms
        conn_params.timeout = CONN_SUPV_TIMEOUT; // timeout = 400*10ms = 4000ms
        // start sent the update connection parameters to the peer device,
        /// .......... update the connection paramter for the device
        esp_ble_gap_update_conn_params(&conn_params);

        memcpy(peer_addr, param->connect.remote_bda, sizeof(esp_bd_addr_t));
        conn_flag = connected;

        ////////// saving the connection id
        connection_id = param->connect.conn_id;

        // ///////////send the connected message to core 1
        xTaskNotify(general_tsk_handle, BLE_CONNECTED, eSetValueWithOverwrite);
        // set the power level for BLE advertisement
        esp_ble_tx_power_set(ESP_BLE_PWR_TYPE_CONN_HDL0, ESP_PWR_LVL_N12);
      }
      /////////////////////////////////////
      break;
    case ESP_GATTS_DISCONNECT_EVT:
      if (gatts_if == gatt_if_device_info) {
        ESP_LOGI(GATTS_TAG, "ESP_GATTS_DISCONNECT_EVT, reason = 0x%x\r\n", param->disconnect.reason);
        ESP_LOGI(GATTS_TAG, "disconnect the devicee , conn_id = %d\r\n", param->disconnect.conn_id);
        conn_flag = disconnected;
        ////////// send the disconnected messaage to core 1
        xTaskNotify(general_tsk_handle, BLE_DISCONNECTED, eSetValueWithOverwrite);
      }

      break;
    case ESP_GATTS_CREAT_ATTR_TAB_EVT: {
      if (param->add_attr_tab.status != ESP_GATT_OK) {
        ESP_LOGE(GATTS_TAG, "create attribute table got  failed, error code=0x%x\r\n", param->add_attr_tab.status);
        break;
      }

      if (gatts_if == gatt_if_device_info) {
        if (param->add_attr_tab.num_handle != DEVICE_INFO_NO_OF_ELE) {
          ESP_LOGE(GATTS_TAG,
                   "create attribute table abnormally, num_handle (%d) \
                        doesn't equal to HRS_IDX_NB(%d)\r\n",
                   param->add_attr_tab.num_handle,
                   DEVICE_INFO_NO_OF_ELE);
        } else {
          ESP_LOGI(GATTS_TAG, "create attribute table successfully, the number handle = %d\n", param->add_attr_tab.num_handle);
          memcpy(device_info_db_handle, param->add_attr_tab.handles, sizeof(device_info_db_handle));

          esp_ble_gatts_start_service(device_info_db_handle[DEVICE_INFO_SERVICE]);
        }
      } else if (gatts_if == gatt_if_batt_srvc) {

        if (param->add_attr_tab.num_handle != BATT_NO_OF_ELE) {
          ESP_LOGE(GATTS_TAG,
                   "create attribute table abnormally, num_handle (%d) \
                        doesn't equal to HRS_IDX_NB(%d)\r\n",
                   param->add_attr_tab.num_handle,
                   BATT_NO_OF_ELE);
        } else {
          ESP_LOGI(GATTS_TAG, "create attribute table successfully, the number handle = %d\n", param->add_attr_tab.num_handle);
          memcpy(battery_db_handle, param->add_attr_tab.handles, sizeof(battery_db_handle));

          esp_ble_gatts_start_service(battery_db_handle[BATTERY_SERVICE]);
        }
      } else if (gatts_if == gatt_if_dfu_srv) {

        if (param->add_attr_tab.num_handle != DFU_NO_OF_ELE) {
          ESP_LOGE(GATTS_TAG,
                   "create attribute table abnormally, num_handle (%d) \
                        doesn't equal to HRS_IDX_NB(%d)\r\n",
                   param->add_attr_tab.num_handle,
                   DFU_NO_OF_ELE);
        } else {
          ESP_LOGI(GATTS_TAG, "create attribute table successfully, the number handle = %d\n", param->add_attr_tab.num_handle);
          memcpy(dfu_db_handle, param->add_attr_tab.handles, sizeof(dfu_db_handle));

          esp_ble_gatts_start_service(dfu_db_handle[DFU_SERVICE]);
        }
      } else if (gatts_if == gatt_if_custom) {

        if (param->add_attr_tab.num_handle != CUSTOM_NO_ELE) {
          ESP_LOGE(GATTS_TAG,
                   "create attribute table abnormally, num_handle (%d) \
                        doesn't equal to HRS_IDX_NB(%d)\r\n",
                   param->add_attr_tab.num_handle,
                   CUSTOM_NO_ELE);
        } else {
          ESP_LOGI(GATTS_TAG, "create attribute table successfully, the number handle = %d\n", param->add_attr_tab.num_handle);
          memcpy(custom_db_handle, param->add_attr_tab.handles, sizeof(custom_db_handle));

          esp_ble_gatts_start_service(custom_db_handle[CUSTOM_SERVICE]);
        }
      }
      break;
    }
    case ESP_GATTS_STOP_EVT:
    case ESP_GATTS_OPEN_EVT:
    case ESP_GATTS_CANCEL_OPEN_EVT:
    case ESP_GATTS_CLOSE_EVT:
    case ESP_GATTS_LISTEN_EVT:
    case ESP_GATTS_CONGEST_EVT:
    case ESP_GATTS_UNREG_EVT:
    case ESP_GATTS_DELETE_EVT:
    default:
      break;
  }
}

///////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////// GAP event Handler /////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////

void gap_event_handler(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t* param) {
  switch (event) {
#ifdef CONFIG_SET_RAW_ADV_DATA
    case ESP_GAP_BLE_ADV_DATA_RAW_SET_COMPLETE_EVT:
      adv_config_done &= (~adv_config_flag);
      if (adv_config_done == 0) {
        esp_ble_gap_start_advertising(&adv_params);
      }
      break;
    case ESP_GAP_BLE_SCAN_RSP_DATA_RAW_SET_COMPLETE_EVT:
      adv_config_done &= (~scan_rsp_config_flag);
      if (adv_config_done == 0) {
        esp_ble_gap_start_advertising(&adv_params);
      }
      break;
#else
    case ESP_GAP_BLE_ADV_DATA_SET_COMPLETE_EVT:
      adv_config_done &= (~adv_config_flag);
      // if (adv_config_done == 0)
      // {
      //     esp_ble_gap_start_advertising(&adv_params);
      // }
      break;
    case ESP_GAP_BLE_SCAN_RSP_DATA_SET_COMPLETE_EVT:
      adv_config_done &= (~scan_rsp_config_flag);
      // if (adv_config_done == 0)
      // {
      //     esp_ble_gap_start_advertising(&adv_params);
      // }
      break;
#endif
    case ESP_GAP_BLE_ADV_START_COMPLETE_EVT:
      printf("advertising\r\n");
      // advertising start complete event to indicate advertising start successfully or failed
      if (param->adv_start_cmpl.status != ESP_BT_STATUS_SUCCESS) {
        ESP_LOGE(GATTS_TAG, "Advertising start failed\n");
      }
      break;
    case ESP_GAP_BLE_ADV_STOP_COMPLETE_EVT:
      if (param->adv_stop_cmpl.status != ESP_BT_STATUS_SUCCESS) {
        ESP_LOGE(GATTS_TAG, "Advertising stop failed\n");
      } else {
        ESP_LOGI(GATTS_TAG, "Stop adv successfully\n");
      }
      break;
    case ESP_GAP_BLE_UPDATE_CONN_PARAMS_EVT:
      ESP_LOGI(GATTS_TAG,
               "update connection params status = %d, min_int = %d, max_int = %d,conn_int = %d,latency = %d, timeout = %d",
               param->update_conn_params.status,
               param->update_conn_params.min_int,
               param->update_conn_params.max_int,
               param->update_conn_params.conn_int,
               param->update_conn_params.latency,
               param->update_conn_params.timeout);
      break;
    default:
      break;
  }
}

esp_err_t esp_ble_send_notif_eeg(uint8_t* buff, uint16_t size) {
  if (conn_flag != connected)
    return ESP_ERR_INVALID_STATE;
  ////////////// sending the value of the char
  return esp_ble_gatts_send_indicate(gatt_if_custom, connection_id, custom_db_handle[EEG_DATA_VAL], size, buff, false);
}

esp_err_t esp_ble_send_notif_tdcs_curr(uint8_t* buff, uint16_t size) {
  if (conn_flag != connected)
    return ESP_ERR_INVALID_STATE;
  ///// send the electrode impedance value to the smartphone
  return esp_ble_gatts_send_indicate(gatt_if_custom, connection_id, custom_db_handle[ELECTRODE_IMP_VAL], size, buff, false);
}

esp_err_t esp_ble_send_battery_data(uint8_t* buff, uint8_t size) {
  if (conn_flag != connected)
    return ESP_ERR_INVALID_STATE;
  ////// send the battery soc and percentage data to smartphone
  return esp_ble_gatts_send_indicate(gatt_if_batt_srvc, connection_id, battery_db_handle[BATTERY_VAL], size, buff, true);
}

esp_err_t esp_ble_send_err_indication(uint8_t err_code) {
  if (conn_flag != connected)
    return ESP_ERR_INVALID_STATE;
  return esp_ble_gatts_send_indicate(gatt_if_custom, connection_id, custom_db_handle[ERROR_CODE_VAL], 1, &err_code, true);
}

/// @brief this is to send the device status through ble indication
/// @param status
/// @return succ/Failuren
esp_err_t esp_ble_send_status_indication(uint8_t status) {
  if (conn_flag != connected)
    return ESP_ERR_INVALID_STATE;
  return esp_ble_gatts_send_indicate(gatt_if_custom, connection_id, custom_db_handle[STATUS_CODE_VAL], 1, &status, true);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////

static void esp_ble_get_device_info_strings(void) {
  /// getting the iterator from the partion finder
  esp_partition_iterator_t meta_data_part = esp_partition_find(ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_DATA_FAT, "meta_data");

  esp_partition_t* my_partion;

  /// check for null
  if (meta_data_part != NULL) {
    my_partion = esp_partition_verify(esp_partition_get(meta_data_part));

    // /// print about the info
    // printf("siz %d ad %x",my_partion->size, my_partion->address);

    /// get the data in a buffer
    esp_partition_read(my_partion, 0, buff, STRING_BUFFER_SIZE);

    meta_Data_len = strlen((char*) buff);
  } else {
    printf("part iter null\r\n");
  }

  /// copy the data into hardware and serial number
  for (int i = 0; i < meta_Data_len; i++) {
    if (buff[i] == ',') {
      hardware_number_len = i;
      break;
    }
  }
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void ble_init(void) {

  esp_err_t ret;
  // Initialize NVS.
  ret = nvs_flash_init();
  if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    ESP_ERROR_CHECK(nvs_flash_erase());
    ret = nvs_flash_init();
  }
  ESP_ERROR_CHECK(ret);

  ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT));

  // // turn off the power domain of the BT and Wifi system
  // esp_wifi_bt_power_domain_off();

  esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
  ret = esp_bt_controller_init(&bt_cfg);
  if (ret) {
    ESP_LOGE(GATTS_TAG, "%s initialize controller failed: %s\n", __func__, esp_err_to_name(ret));
    return;
  }

  ret = esp_bt_controller_enable(ESP_BT_MODE_BLE);
  if (ret) {
    ESP_LOGE(GATTS_TAG, "%s enable controller failed: %s\n", __func__, esp_err_to_name(ret));
    return;
  }
  ret = esp_bluedroid_init();
  if (ret) {
    ESP_LOGE(GATTS_TAG, "%s init bluetooth failed: %s\n", __func__, esp_err_to_name(ret));
    return;
  }
  ret = esp_bluedroid_enable();
  if (ret) {
    ESP_LOGE(GATTS_TAG, "%s enable bluetooth failed: %s\n", __func__, esp_err_to_name(ret));
    return;
  }

  ret = esp_ble_gatts_register_callback(gatts_events_handler);
  if (ret) {
    ESP_LOGE(GATTS_TAG, "gatts register error, error code = %x", ret);
    return;
  }
  ret = esp_ble_gap_register_callback(gap_event_handler);
  if (ret) {
    ESP_LOGE(GATTS_TAG, "gap register error, error code = %x", ret);
    return;
  }

  // set the power level for BLE advertisement
  // esp_ble_tx_power_set(ESP_BLE_PWR_TYPE_ADV, ESP_PWR_LVL_N12);

  ////////// registering app for event handlers
  if (esp_ble_gatts_app_register(PROFILE_DEVICE_INFORMATION) != ESP_OK)
    printf("error in init the device info profile\r\n");

  if (esp_ble_gatts_app_register(PROFILE_BATT_SERVICE) != ESP_OK)
    printf("error in init the batt servc profile\r\n");

  if (esp_ble_gatts_app_register(PROFILE_FIRM_UPDATE) != ESP_OK)
    printf("error in init the dfu profile\r\n");

  if (esp_ble_gatts_app_register(PROFILE_TDC_EEG) != ESP_OK)
    printf("error in init the custom profile\r\n");

  ///// may be the max mtu size is 500 bytes
  esp_err_t local_mtu_ret = esp_ble_gatt_set_local_mtu(500);
  if (local_mtu_ret) {
    ESP_LOGE(GATTS_TAG, "set local  MTU failed, error code = %x\r\n", local_mtu_ret);
  }

  // esp_ble_get_device_info_strings();
  /////// above code id to implement BLE in peripheral mode , create the service , charcteristics , and handling events
}

/// @brief to start the advertisement again
/// @param
void ble_start_advertise(void) {
  esp_ble_gap_start_advertising(&adv_params);
}

/// @brief to stop the advertisement
/// @param
void ble_stop_advertise(void) {
  esp_ble_gap_stop_advertising();
}

/// @brief to disconnect the peer device
/// @param
void ble_disconnect_device(void) {
  if (conn_flag == connected) {
    esp_ble_gap_disconnect(peer_addr);
  }
}

void bt_controller_off(void) {
  esp_wifi_stop();
  esp_bluedroid_disable();
  esp_bluedroid_deinit();
  esp_bt_controller_disable();
  esp_bt_controller_deinit();
  esp_wifi_bt_power_domain_off();
}