#include "ble.h"

#include "sys_attr.h"

#include "esp_gap_ble_api.h"
#include "esp_bt_defs.h"
#include "esp_bt_main.h"
#include "esp_bt.h"
#include "esp_gatts_api.h"
#include "esp_gatt_common_api.h"
#include "esp_wifi.h"

static uint8_t char_start_val[1] = {0};

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
static const UNUSED uint8_t char_w_notify = ESP_GATT_CHAR_PROP_BIT_WRITE | ESP_GATT_CHAR_PROP_BIT_NOTIFY;
static const UNUSED uint8_t char_prop_write = ESP_GATT_CHAR_PROP_BIT_WRITE;
// static const uint8_t char_prop_r_w = ESP_GATT_CHAR_PROP_BIT_READ | ESP_GATT_CHAR_PROP_BIT_WRITE;
static const UNUSED uint8_t char_prop_read = ESP_GATT_CHAR_PROP_BIT_READ;
static const UNUSED uint8_t char_prop_r_notify = ESP_GATT_CHAR_PROP_BIT_READ | ESP_GATT_CHAR_PROP_BIT_NOTIFY;

static const UNUSED uint8_t char_prop_r_indicate = ESP_GATT_CHAR_PROP_BIT_READ | ESP_GATT_CHAR_PROP_BIT_INDICATE;
static const UNUSED uint8_t char_prop_r_w_indicate = ESP_GATT_CHAR_PROP_BIT_WRITE | ESP_GATT_CHAR_PROP_BIT_READ | ESP_GATT_CHAR_PROP_BIT_INDICATE;
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
static const uint16_t device_number_char = 0x2A23;

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////................ server database description for device
/// information..........................................

const esp_gatts_attr_db_t device_info_db_table[DEVICE_INFO_NO_OF_ELE] =
    ///////////// this data base table is const type
    {
        /////////// service declaration
        // Service Declaration
        [DEVICE_INFO_SERVICE] = {{ESP_GATT_AUTO_RSP},
                                 {ESP_UUID_LEN_16, u8_ptr(primary_service_uuid), ESP_GATT_PERM_READ, _2bytes, _2bytes, u8_ptr(device_info_uuid)}},

        /* Characteristic Declaration */
        [MANUFACTURER_NAME_CHAR] = {{ESP_GATT_AUTO_RSP},
                                    {ESP_UUID_LEN_16, u8_ptr(character_declaration_uuid), ESP_GATT_PERM_READ, _1byte, _1byte, u8_ptr(char_prop_read)}},

        /* Characteristic Value */ // respond by app
        [MANUFACTURER_NAME_VAL] = {{ESP_GATT_RSP_BY_APP},
                                   {ESP_UUID_LEN_16,
                                    u8_ptr(manufact_name_char),
                                    ESP_GATT_PERM_READ,
                                    GATT_MAX_ATTR_STRING_LEN,
                                    _1byte,
                                    char_start_val}},

        /* Characteristic Declaration */
        [SERIAL_NUMBER_CAHR] = {{ESP_GATT_AUTO_RSP},
                                {ESP_UUID_LEN_16, u8_ptr(character_declaration_uuid), ESP_GATT_PERM_READ, _1byte, _1byte, u8_ptr(char_prop_read)}},

        /* Characteristic value */
        [SERIAL_NUMBER_VAL] = {{ESP_GATT_RSP_BY_APP},
                               {ESP_UUID_LEN_16, u8_ptr(serial_num_char), ESP_GATT_PERM_READ, MAX_ATTRIBUTE_SIZE, _1byte, char_start_val}},

        /* Characteristic declaration */ // respond by app
        [HARDWARE_REVISION_CHAR] = {{ESP_GATT_AUTO_RSP},
                                    {ESP_UUID_LEN_16, u8_ptr(character_declaration_uuid), ESP_GATT_PERM_READ, _1byte, _1byte, u8_ptr(char_prop_read)}},

        /* character value  */
        [HARDWARE_REVISION_VAL] = {{ESP_GATT_RSP_BY_APP},
                                   {ESP_UUID_LEN_16, u8_ptr(hardware_rev_char), ESP_GATT_PERM_READ, MAX_ATTRIBUTE_SIZE, _1byte, char_start_val}},

        ///////characteristic declrartion
        [FIRMWARE_REVISION_CHAR] = {{ESP_GATT_AUTO_RSP},
                                    {ESP_UUID_LEN_16, u8_ptr(character_declaration_uuid), ESP_GATT_PERM_READ, _1byte, _1byte, u8_ptr(char_prop_read)}},

        /////////// charcteristic value
        [FIRMWARE_REVISION_VAL] = {{ESP_GATT_RSP_BY_APP},
                                   {ESP_UUID_LEN_16,
                                    u8_ptr(firmware_rev_char),
                                    ESP_GATT_PERM_READ,
                                    MAX_ATTRIBUTE_SIZE,
                                    _1byte,
                                    char_start_val}},

        [DEVICE_NUMBER_CHAR] = {{ESP_GATT_AUTO_RSP},
                                {ESP_UUID_LEN_16, u8_ptr(character_declaration_uuid), ESP_GATT_PERM_READ, _1byte, _1byte, u8_ptr(char_prop_read)}},

        [DEVICE_NUMBER_VAL] =     {{ESP_GATT_RSP_BY_APP},
                                    {ESP_UUID_LEN_16,
                                    u8_ptr(device_number_char),
                                    ESP_GATT_PERM_READ,
                                    MAX_ATTRIBUTE_SIZE,
                                    _1byte,
                                    char_start_val}},

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
const esp_gatts_attr_db_t battery_db_table[BATT_NO_OF_ELE] = {
    // Service Declaration
    [BATTERY_SERVICE] = {{ESP_GATT_AUTO_RSP},
                         {ESP_UUID_LEN_16, u8_ptr(primary_service_uuid), ESP_GATT_PERM_READ, _2bytes, _2bytes, u8_ptr(battery_service)}},

    /* Characteristic Declaration */
    [BATTERY_CHAR] = {{ESP_GATT_AUTO_RSP},
                      {ESP_UUID_LEN_16, u8_ptr(character_declaration_uuid), ESP_GATT_PERM_READ, _1byte, _1byte, u8_ptr(char_prop_r_indicate)}},

    /* Characteristic Value */ // respond by app
    [BATTERY_VAL] = {{ESP_GATT_RSP_BY_APP}, {ESP_UUID_LEN_16, u8_ptr(battery_char), ESP_GATT_PERM_READ, sizeof(uint32_t), _1byte, u8_ptr(batt_val)}},

    /////////// battery presenatation format (output value unit )
    [BATT_CHAR_PRES_FORMAT] = {{ESP_GATT_AUTO_RSP},
                               {ESP_UUID_LEN_16,
                                u8_ptr(charcter_user_desrcriptor),
                                ESP_GATT_PERM_READ,
                                sizeof(BATTERY_PRES_FORMAT),
                                sizeof(BATTERY_PRES_FORMAT),
                                u8_ptr(BATTERY_PRES_FORMAT)}},

    ///////// battery client char config
    [BATT_CHAR_CONFIG] = {{ESP_GATT_AUTO_RSP},
                          {ESP_UUID_LEN_16, u8_ptr(client_charcater_config_uuid), Gatt_perm_r_w, _2bytes, _2bytes, u8_ptr(batt_char_config)}},

    ///////////// character declartaion battery custom paramter
    [BATT_RAW_PARAM_CHAR] = {{ESP_GATT_AUTO_RSP},
                             {ESP_UUID_LEN_16, u8_ptr(character_declaration_uuid), ESP_GATT_PERM_READ, _1byte, _1byte, u8_ptr(char_prop_r_notify)}},

    ////////// battery raw parameter values
    [BATT_RAW_PARM_VAL] = {{ESP_GATT_RSP_BY_APP},
                           {ESP_UUID_LEN_16, u8_ptr(batt_raw_char), ESP_GATT_PERM_READ, MAX_ATTRIBUTE_SIZE, _1byte, u8_ptr(batt_val)}},

    ///////// battery raw paramter  client char config
    [BATT_RAW_PARAM_CCFG] = {{ESP_GATT_AUTO_RSP},
                             {ESP_UUID_LEN_16, u8_ptr(client_charcater_config_uuid), Gatt_perm_r_w, _2bytes, _2bytes, u8_ptr(batt_char_config)}},

};


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
const esp_gatts_attr_db_t custom_db_table[CUSTOM_NO_ELE] = {
    /////////// service declaration
    // Service Declaration
    [CUSTOM_SERVICE] = {{ESP_GATT_AUTO_RSP},
                        {ESP_UUID_LEN_16, u8_ptr(primary_service_uuid), ESP_GATT_PERM_READ, _2bytes, _2bytes, u8_ptr(custom_serv)}},

    /* Characteristic Declaration  electrode impedance */
    [ELECTRODE_IMP_CHAR] = {{ESP_GATT_AUTO_RSP},
                            {ESP_UUID_LEN_16, u8_ptr(character_declaration_uuid), ESP_GATT_PERM_READ, _1byte, _1byte, u8_ptr(char_prop_r_notify)}},

    /* Characteristic Value */ // respond by app
    [ELECTRODE_IMP_VAL] = {{ESP_GATT_RSP_BY_APP},
                           {ESP_UUID_LEN_16, u8_ptr(custom_char1), ESP_GATT_PERM_READ, custom_char_mx_size, sizeof(value), u8_ptr(value)}},

    /* Client Characteristic Configuration Descriptor */
    [ELECTRODE_IMP_CCFG] =
        {{ESP_GATT_AUTO_RSP},
         {ESP_UUID_LEN_16, u8_ptr(client_charcater_config_uuid), Gatt_perm_r_w, sizeof(ccfg_val), sizeof(ccfg_val), u8_ptr(ccfg_val)}},

    /* Characteristic Declaration  tdcs setting  */
    [TDCS_SETT_CHAR] = {{ESP_GATT_AUTO_RSP},
                        {ESP_UUID_LEN_16, u8_ptr(character_declaration_uuid), ESP_GATT_PERM_READ, _1byte, _1byte, u8_ptr(char_prop_write)}},

    /* Characteristic Value */ // respond by app
    [TDCS_SETT_VAL] = {{ESP_GATT_RSP_BY_APP},
                       {ESP_UUID_LEN_16, u8_ptr(custom_char2), ESP_GATT_PERM_WRITE, cstm_write_mx_size, sizeof(value), u8_ptr(value)}},

    /* Client Characteristic Configuration Descriptor */
    [TDCS_SETT_CCFG] = {{ESP_GATT_AUTO_RSP},
                        {ESP_UUID_LEN_16, u8_ptr(client_charcater_config_uuid), Gatt_perm_r_w, sizeof(ccfg_val), sizeof(ccfg_val), u8_ptr(ccfg_val)}},

    /* Characteristic Declaration  eeg data read*/
    [EEG_DATA_CHAR] = {{ESP_GATT_AUTO_RSP},
                       {ESP_UUID_LEN_16, u8_ptr(character_declaration_uuid), ESP_GATT_PERM_READ, _1byte, _1byte, u8_ptr(char_prop_r_notify)}},

    /* Characteristic Value */ // respond by app
    [EEG_DATA_VAL] = {{ESP_GATT_RSP_BY_APP},
                      {ESP_UUID_LEN_16, u8_ptr(custom_char3), ESP_GATT_PERM_READ, custom_char_mx_size, sizeof(value), u8_ptr(value)}},

    /* Client Characteristic Configuration Descriptor */
    [EEG_DATA_CCFG] = {{ESP_GATT_AUTO_RSP},
                       {ESP_UUID_LEN_16, u8_ptr(client_charcater_config_uuid), Gatt_perm_r_w, sizeof(ccfg_val), sizeof(ccfg_val), u8_ptr(ccfg_val)}},

    /* Characteristic Declaration  eeg settings  */
    [EEG_SETT_CHAR] = {{ESP_GATT_AUTO_RSP},
                       {ESP_UUID_LEN_16, u8_ptr(character_declaration_uuid), ESP_GATT_PERM_READ, _1byte, _1byte, u8_ptr(char_prop_write)}},

    /* Characteristic Value */ // respond by app
    [EEG_SETT_VAL] = {{ESP_GATT_RSP_BY_APP},
                      {ESP_UUID_LEN_16, u8_ptr(custom_char4), ESP_GATT_PERM_WRITE, cstm_write_mx_size, sizeof(value), u8_ptr(value)}},

    /* Client Characteristic Configuration Descriptor */
    [EEG_SETT_CCFG] = {{ESP_GATT_AUTO_RSP},
                       {ESP_UUID_LEN_16, u8_ptr(client_charcater_config_uuid), Gatt_perm_r_w, sizeof(ccfg_val), sizeof(ccfg_val), u8_ptr(ccfg_val)}},

    /* Characteristic Declaration  error code */
    [ERROR_CODE_CHAR] = {{ESP_GATT_AUTO_RSP},
                         {ESP_UUID_LEN_16, u8_ptr(character_declaration_uuid), ESP_GATT_PERM_READ, _1byte, _1byte, u8_ptr(char_prop_r_w_indicate)}},

    /* Characteristic Value */ // respond by app
    [ERROR_CODE_VAL] = {{ESP_GATT_RSP_BY_APP}, {ESP_UUID_LEN_16, u8_ptr(custom_char5), Gatt_perm_r_w, custom_char_mx_size, sizeof(value), u8_ptr(value)}},

    /* Client Characteristic Configuration Descriptor */
    [ERROR_CODE_CCFG] =
        {{ESP_GATT_AUTO_RSP},
         {ESP_UUID_LEN_16, u8_ptr(client_charcater_config_uuid), Gatt_perm_r_w, sizeof(ccfg_val), sizeof(ccfg_val), u8_ptr(ccfg_val)}},

    /* Characteristic Declaration  status code*/
    [STATUS_CODE_CHAR] = {{ESP_GATT_AUTO_RSP},
                          {ESP_UUID_LEN_16, u8_ptr(character_declaration_uuid), ESP_GATT_PERM_READ, _1byte, _1byte, u8_ptr(char_prop_r_w_indicate)}},

    /* Characteristic Value */ //  status of the device
    [STATUS_CODE_VAL] = {{ESP_GATT_RSP_BY_APP}, {ESP_UUID_LEN_16, u8_ptr(custom_char6), Gatt_perm_r_w, _10bytes, sizeof(value), u8_ptr(value)}},

    /* Client Characteristic Configuration Descriptor */
    [STATUS_CODE_CCFG] = {{ESP_GATT_AUTO_RSP},
                          {ESP_UUID_LEN_16,
                           u8_ptr(client_charcater_config_uuid),
                           ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
                           sizeof(ccfg_val),
                           sizeof(ccfg_val),
                           u8_ptr(ccfg_val)}},

};
