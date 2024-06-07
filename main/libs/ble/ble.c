#include <string.h>
#include "ble.h"

#include "sys_attr.h"

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
// #include "esp_spi_flash.h"

/////// esp32 bare metal supportive libs
#include "esp_system.h"
#include "esp_log.h"

#include "batt.h"
#include "eeg.h"
#include "tdcs.h"
#include "flash_op.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// ======================define some useful macros================================
// =======------------------------------------------------------------------------
#define ESP_BLE_ERR_CHECK(x)                                                    \
    if (x)                                                                      \
    {                                                                           \
        ESP_LOGE(TAG, "BLE func %s Failed %s", __func__, esp_err_to_name(err)); \
        assert(false);                                                          \
    }

// ================== structure for gatt ptofile instance
struct gatts_profile_inst
{
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

// peer device bt addr
esp_bd_addr_t peer_addr;
uint8_t conn_flag = disconnected;
////////// this is the advertisement service uuid

static const char *TAG = "BLE";
static const char *GATTS_TAG = "GATTS_DEMO";

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

static uint16_t connection_id = 0;

static TaskHandle_t general_Taskhandle = NULL;

extern const esp_gatts_attr_db_t device_info_db_table[DEVICE_INFO_NO_OF_ELE];
static uint16_t device_info_db_handle[DEVICE_INFO_NO_OF_ELE];
static uint16_t gatt_if_device_info = ESP_GATT_IF_NONE;

extern const esp_gatts_attr_db_t battery_db_table[BATT_NO_OF_ELE];
static uint16_t battery_db_handle[BATT_NO_OF_ELE];
static uint16_t gatt_if_batt_srvc = ESP_GATT_IF_NONE;

extern const esp_gatts_attr_db_t custom_db_table[CUSTOM_NO_ELE];
static uint16_t custom_db_handle[CUSTOM_NO_ELE]; ///////this will create store  handler for the every charcteristic and attributes
static uint16_t gatt_if_custom = ESP_GATT_IF_NONE;
/////////////////// this is the var in which ble data is stored

static uint8_t adv_config_done = 0;

/////////////////////////////////////////////// //////
// this value stores the input value from the smartphone

#define adv_config_flag (1 << 0)
#define scan_rsp_config_flag (1 << 1)

////////////////////////// gatt event handler  //////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void gatts_events_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param)
{
    switch (event)
    {
    case ESP_GATTS_REG_EVT: /////////// this is the register event we have to create a service table here
    {
        /////// you cannot declare a variable after case thats why a new scope if inserted here
        if (param->reg.app_id == SERVICE_DEVICE_INFORMATION)
        {
            gatt_if_device_info = gatts_if;

            if (esp_ble_gatts_create_attr_tab(device_info_db_table, gatts_if, DEVICE_INFO_NO_OF_ELE, SERVICE_DEVICE_INFORMATION) !=
                ESP_OK)
            {
                ESP_LOGE(GATTS_TAG, "failed attr table for device information service \r\n");
            }
        }

        else if (param->reg.app_id == PROFILE_BATT_SERVICE)
        {
            gatt_if_batt_srvc = gatts_if;
            if (esp_ble_gatts_create_attr_tab(battery_db_table, gatts_if, BATT_NO_OF_ELE, SERVICE_BATT_SERVICE) != ESP_OK)
            {
                ESP_LOGE(GATTS_TAG, "failed attr table for battery  service \r\n");
            }
        }

        else if (param->reg.app_id == PROFILE_TDC_EEG)
        {
            gatt_if_custom = gatts_if;
            if (esp_ble_gatts_create_attr_tab(custom_db_table, gatts_if, CUSTOM_NO_ELE, SERVICE_TDC_EEG) != ESP_OK)
            {
                ESP_LOGE(GATTS_TAG, "failed attr table for custom service \r\n");
            }
        }
    }
    break;

    //// read event GATT
    case ESP_GATTS_READ_EVT:
    {
        // define the variables for the error and status codes
        static uint8_t status = STATUS_NONE;
        static uint32_t errorcode = ERR_NONE;
        ESP_LOGI(GATTS_TAG,
                 "GATT_READ_EVT, gatt_interface  %u, trans_id %u, handle %u\n",
                 gatts_if,
                 param->read.trans_id,
                 param->read.handle);
        esp_gatt_rsp_t rsp = {0};

        if (gatts_if == gatt_if_device_info)
        {
            uint8_t index = flash_app_info_none;
            // need to know the hardware revision
            if (param->read.handle == device_info_db_handle[HARDWARE_REVISION_VAL])
            {
                index = flash_app_info_Hardware_num;
            }
            // serial number
            else if (param->read.handle == device_info_db_handle[SERIAL_NUMBER_VAL])
            {
                index = flash_app_info_serial_num;
            }
            // firmware version
            else if (param->read.handle == device_info_db_handle[FIRMWARE_REVISION_VAL])
            {
                index = flash_app_info_firmware_num;
            }
            else if (param->read.handle == device_info_db_handle[DEVICE_NUMBER_VAL])
            {
                index = flash_app_info_device_num;
            }
            else if(param->read.handle == device_info_db_handle[MANUFACTURER_NAME_VAL])
            {
                index = flash_app_info_app_manuf_name;
            }

            const char * str =  flash_app_get_info(index);
            rsp.attr_value.len = strlen(str);
            memcpy(rsp.attr_value.value,str, strlen(str));
        }

        else if (gatts_if == gatt_if_batt_srvc)
        {
            if (param->read.handle == battery_db_handle[BATTERY_VAL])
            {

                rsp.attr_value.value[0] = batt_get_soc();
                rsp.attr_value.value[1] = batt_get_chg_status();
                rsp.attr_value.len = 2;
            }
            else if (param->read.handle == battery_db_handle[BATT_RAW_PARM_VAL])
            {

                //// get the device status
                if (sys_get_no_item_stat_q() > 0)
                {
                    status = sys_pop_stat_q();
                }
                rsp.attr_value.value[0] = status;

                // copy the data
                batt_get_data((batt_data_struct_t *)&rsp.attr_value.value[1]);
                rsp.attr_value.len = sizeof(batt_data_struct_t) + 1;
            }
        }

        else if (gatts_if == gatt_if_custom)
        {

            if (param->read.handle == custom_db_handle[STATUS_CODE_VAL])
            {

                if (sys_get_no_item_stat_q() > 0)
                {
                    status = sys_pop_stat_q();
                }
                rsp.attr_value.value[0] = status;
                rsp.attr_value.len = 1;
            }
            else if (param->read.handle == custom_db_handle[ERROR_CODE_VAL])
            {
                if (sys_get_no_item_err_q() > 0)
                {
                    errorcode = sys_pop_err_q();
                }
                rsp.attr_value.len = sizeof(errorcode);
                memcpy(rsp.attr_value.value, &errorcode, sizeof(errorcode));
                //////////// clear the error code afterwards
                errorcode = ERR_NONE;
            }
        }

        rsp.attr_value.handle = param->read.handle;
        esp_ble_gatts_send_response(gatts_if, param->read.conn_id, param->read.trans_id, ESP_GATT_OK, &rsp);
        break;
    }
    case ESP_GATTS_WRITE_EVT: /////////// gatt write event it is value recieve by the esp32
        if (!param->write.is_prep)
        {
            // the data length of gattc write  must be less than GATTS_DEMO_CHAR_VAL_LEN_MAX.
            ESP_LOGI(GATTS_TAG, "GATT_WRITE_EVT, handle = %u, value len = %u, value :\r\n", param->write.handle, param->write.len);
            esp_log_buffer_hex(GATTS_TAG, param->write.value, param->write.len);
            //   check the data for the attribute values

            if (custom_db_handle[EEG_SETT_VAL] == param->write.handle)
            {
                eeg_cmd_struct_t eeg_cmd = {0};

                memcpy(&eeg_cmd, param->write.value, MIN_OF(param->write.len, sizeof(eeg_cmd)));

                //// reset the message buffer and then send the data
                sys_reset_msg_buffer();
                sys_push_msg_buff(u8_ptr(eeg_cmd), sizeof(eeg_cmd));

                ////// send the message to the task
                ////// notify the task when ble data arrives
                if (eeg_cmd.rate == 1)
                {
                    xTaskNotify(general_Taskhandle, DEV_STATE_STOP, eSetValueWithOverwrite);
                }
                else
                {
                    xTaskNotify(general_Taskhandle, DEV_STATE_RUN_EEG, eSetValueWithOverwrite);
                }
            }

            else if (custom_db_handle[TDCS_SETT_VAL] == param->write.handle)
            {
                tdcs_cmd_struct_t tdcs_cmd = {0};

                memcpy(&tdcs_cmd, param->write.value, MIN_OF(param->write.len, sizeof(tdcs_cmd)));

                sys_reset_msg_buffer();
                sys_push_msg_buff(u8_ptr(tdcs_cmd), sizeof(tdcs_cmd));

                ////// send the message to the task
                if ((tdcs_cmd.opcode == 8) || ((tdcs_cmd.opcode == 9)))
                {
                    xTaskNotify(general_Taskhandle, DEV_STATE_STOP, eSetValueWithOverwrite);
                }
                else
                {
                    xTaskNotify(general_Taskhandle, DEV_STATE_RUN_TDCS, eSetValueWithOverwrite);
                }
                ////// notify the task when ble data arrives
            }

            else if (custom_db_handle[STATUS_CODE_VAL] == param->write.handle)
            {
                if (param->write.value[0] == STATUS_PWR_OFF)
                {
                    ESP_LOGW(TAG, "shuting down system");
                    system_shutdown();
                }
            }

            else if (custom_db_handle[ERROR_CODE_VAL] == param->write.handle)
            {
                if (param->write.value[0] == ESP_ERR_GET_BIOS_ERR)
                {
                    ESP_LOGE(TAG, "getting bios errr");
                    uint32_t errors[SYS_ERRORS_MAX_NUMS];
                    extern esp_err_t device_get_bios_err(uint32_t arr[], uint8_t num);
                    device_get_bios_err(errors, SYS_ERRORS_MAX_NUMS);
                    esp_ble_send_error_array((uint8_t *)errors, sizeof(errors));
                }
            }

            esp_ble_gatts_send_response(gatts_if, param->write.conn_id, param->write.trans_id, ESP_GATT_OK, NULL);
        }
        break;

    case ESP_GATTS_EXEC_WRITE_EVT:
        ESP_LOGI(GATTS_TAG, "ESP_GATTS_EXEC_WRITE_EVT\r\n");
        // esp_ble_gatts_send_response(gatts_if, param->write.conn_id, param->write.trans_id, ESP_GATT_OK, NULL);
        // example_exec_write_event_env(&ble_data, param);
        break;
    case ESP_GATTS_MTU_EVT:
        ESP_LOGI(GATTS_TAG, "ESP_GATTS_MTU_EVT, MTU %u\r\n", param->mtu.mtu);
        break;
    case ESP_GATTS_CONF_EVT:
        // ESP_LOGI(GATTS_TAG, "ESP_GATTS_CONF_EVT, status = %d, attr_handle %d", param->conf.status, param->conf.handle);
        break;
    case ESP_GATTS_START_EVT:
        ESP_LOGI(GATTS_TAG, "SERVICE_START_EVT, status %d, service_handle %d\r\n", param->start.status, param->start.service_handle);
        break;
    case ESP_GATTS_CONNECT_EVT:
        ///////////// connection update have to be done only once
        if (gatts_if == gatt_if_device_info)
        {
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
            xTaskNotify(general_Taskhandle, DEV_STATE_BLE_CONNECTED, eSetValueWithOverwrite);
            // set the power level for BLE advertisement
            esp_ble_tx_power_set(ESP_BLE_PWR_TYPE_CONN_HDL0, ESP_PWR_LVL_N12);
        }
        /////////////////////////////////////
        break;
    case ESP_GATTS_DISCONNECT_EVT:
        if (gatts_if == gatt_if_device_info)
        {
            ESP_LOGI(GATTS_TAG, "ESP_GATTS_DISCONNECT_EVT, reason = 0x%x\r\n", param->disconnect.reason);
            ESP_LOGI(GATTS_TAG, "disconnect the devicee , conn_id = %d\r\n", param->disconnect.conn_id);
            conn_flag = disconnected;
            ////////// send the disconnected messaage to core 1
            xTaskNotify(general_Taskhandle, DEV_STATE_BLE_DISCONNECTED, eSetValueWithOverwrite);
        }

        break;
    case ESP_GATTS_CREAT_ATTR_TAB_EVT:
    {
        if (param->add_attr_tab.status != ESP_GATT_OK)
        {
            ESP_LOGE(GATTS_TAG, "create attribute table got  failed, error code=0x%x\r\n", param->add_attr_tab.status);
            break;
        }

        if (gatts_if == gatt_if_device_info)
        {
            if (param->add_attr_tab.num_handle != DEVICE_INFO_NO_OF_ELE)
            {
                ESP_LOGE(GATTS_TAG,
                         "create attribute table abnormally, num_handle (%d) \
                        doesn't equal to HRS_IDX_NB(%d)\r\n",
                         param->add_attr_tab.num_handle,
                         DEVICE_INFO_NO_OF_ELE);
            }
            else
            {
                ESP_LOGI(GATTS_TAG, "create attribute table successfully, the number handle = %d\n", param->add_attr_tab.num_handle);
                memcpy(device_info_db_handle, param->add_attr_tab.handles, sizeof(device_info_db_handle));

                esp_ble_gatts_start_service(device_info_db_handle[DEVICE_INFO_SERVICE]);
            }
        }
        else if (gatts_if == gatt_if_batt_srvc)
        {

            if (param->add_attr_tab.num_handle != BATT_NO_OF_ELE)
            {
                ESP_LOGE(GATTS_TAG,
                         "create attribute table abnormally, num_handle (%d) \
                        doesn't equal to HRS_IDX_NB(%d)\r\n",
                         param->add_attr_tab.num_handle,
                         BATT_NO_OF_ELE);
            }
            else
            {
                ESP_LOGI(GATTS_TAG, "create attribute table successfully, the number handle = %d\n", param->add_attr_tab.num_handle);
                memcpy(battery_db_handle, param->add_attr_tab.handles, sizeof(battery_db_handle));

                esp_ble_gatts_start_service(battery_db_handle[BATTERY_SERVICE]);
            }
        }
        // } else if (gatts_if == gatt_if_dfu_srv) {

        //   if (param->add_attr_tab.num_handle != DFU_NO_OF_ELE) {
        //     ESP_LOGE(GATTS_TAG,
        //              "create attribute table abnormally, num_handle (%d)
        //                   doesn't equal to HRS_IDX_NB(%d)\r\n",
        //              param->add_attr_tab.num_handle,
        //              DFU_NO_OF_ELE);
        //   } else {
        //     ESP_LOGI(GATTS_TAG, "create attribute table successfully, the number handle = %d\n", param->add_attr_tab.num_handle);
        //     memcpy(dfu_db_handle, param->add_attr_tab.handles, sizeof(dfu_db_handle));

        //     esp_ble_gatts_start_service(dfu_db_handle[DFU_SERVICE]);
        //   }
        // }
        else if (gatts_if == gatt_if_custom)
        {

            if (param->add_attr_tab.num_handle != CUSTOM_NO_ELE)
            {
                ESP_LOGE(GATTS_TAG,
                         "create attribute table abnormally, num_handle (%d) \
                        doesn't equal to HRS_IDX_NB(%d)\r\n",
                         param->add_attr_tab.num_handle,
                         CUSTOM_NO_ELE);
            }
            else
            {
                ESP_LOGI(GATTS_TAG, "create attribute table successfully, the number handle = %d\n", param->add_attr_tab.num_handle);
                memcpy(custom_db_handle, param->add_attr_tab.handles, sizeof(custom_db_handle));

                esp_ble_gatts_start_service(custom_db_handle[CUSTOM_SERVICE]);
            }
        }
        break;
    }
    case ESP_GATTS_SET_ATTR_VAL_EVT:
        ESP_LOGW(TAG, "setting char val %d,", param->set_attr_val.status);
        break;
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

void gap_event_handler(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param)
{
    switch (event)
    {
#ifdef CONFIG_SET_RAW_ADV_DATA
    case ESP_GAP_BLE_ADV_DATA_RAW_SET_COMPLETE_EVT:
        adv_config_done &= (~adv_config_flag);
        if (adv_config_done == 0)
        {
            esp_ble_gap_start_advertising(&adv_params);
        }
        break;
    case ESP_GAP_BLE_SCAN_RSP_DATA_RAW_SET_COMPLETE_EVT:
        adv_config_done &= (~scan_rsp_config_flag);
        if (adv_config_done == 0)
        {
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
        ESP_LOGW(TAG, "advertising");
        // advertising start complete event to indicate advertising start successfully or failed
        if (param->adv_start_cmpl.status != ESP_BT_STATUS_SUCCESS)
        {
            ESP_LOGE(GATTS_TAG, "Advertising start failed\n");
        }
        break;
    case ESP_GAP_BLE_ADV_STOP_COMPLETE_EVT:
        if (param->adv_stop_cmpl.status != ESP_BT_STATUS_SUCCESS)
        {
            ESP_LOGE(GATTS_TAG, "Advertising stop failed\n");
        }
        else
        {
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

esp_err_t esp_ble_send_notif_eeg(uint8_t *buff, uint16_t size)
{
    if (conn_flag != connected)
        return ESP_ERR_INVALID_STATE;
    ////////////// sending the value of the char
    return esp_ble_gatts_send_indicate(gatt_if_custom, connection_id, custom_db_handle[EEG_DATA_VAL], size, buff, false);
}

esp_err_t esp_ble_send_notif_tdcs_curr(uint8_t *buff, uint16_t size)
{
    if (conn_flag != connected)
        return ESP_ERR_INVALID_STATE;
    ///// send the electrode impedance value to the smartphone
    return esp_ble_gatts_send_indicate(gatt_if_custom, connection_id, custom_db_handle[ELECTRODE_IMP_VAL], size, buff, false);
}

esp_err_t esp_ble_send_battery_data(uint8_t *buff, uint16_t size)
{
    if (conn_flag != connected)
        return ESP_ERR_INVALID_STATE;
    ////// send the battery soc and percentage data to smartphone
    return esp_ble_gatts_send_indicate(gatt_if_batt_srvc, connection_id, battery_db_handle[BATTERY_VAL], size, buff, true);
}

esp_err_t esp_ble_send_err_indication(uint8_t err_code)
{
    if (conn_flag != connected)
        return ESP_ERR_INVALID_STATE;
    return esp_ble_gatts_send_indicate(gatt_if_custom, connection_id, custom_db_handle[ERROR_CODE_VAL], 1, &err_code, true);
}

/// @brief send the error array
/// @param error
/// @param size
/// @return succ/failure
esp_err_t esp_ble_send_error_array(uint8_t *error, uint8_t size)
{
    if (conn_flag != connected)
        return ESP_ERR_INVALID_STATE;
    return esp_ble_gatts_send_indicate(gatt_if_custom, connection_id, custom_db_handle[ERROR_CODE_VAL], size, error, true);
}

esp_err_t esp_ble_send_err_string(uint8_t *err_buff, uint16_t size)
{
    if (conn_flag != connected)
        return ESP_ERR_INVALID_STATE;
    return esp_ble_gatts_send_indicate(gatt_if_custom, connection_id, custom_db_handle[ERROR_CODE_VAL], size, err_buff, true);
}

/// @brief this is to send the device status through ble indication
/// @param status
/// @return succ/Failuren
esp_err_t esp_ble_send_status_indication(uint8_t status)
{
    if (conn_flag != connected)
        return ESP_ERR_INVALID_STATE;
    return esp_ble_gatts_send_indicate(gatt_if_custom, connection_id, custom_db_handle[STATUS_CODE_VAL], 1, &status, true);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief start the communcation
/// @param taskhandle
void ble_start_driver(void *taskhandle)
{

    esp_err_t set_dev_name_ret = esp_ble_gap_set_device_name(DEVICE_NAME);
    if (set_dev_name_ret)
    {
        ESP_LOGE(GATTS_TAG, "set device name failed, error code = %x", set_dev_name_ret);
    }
    // config adv data
    esp_err_t ret = esp_ble_gap_config_adv_data(&adv_data);
    if (ret)
    {
        ESP_LOGE(GATTS_TAG, "config adv data failed, error code = %x", ret);
    }
    adv_config_done |= adv_config_flag;
    // config scan response data
    ret = esp_ble_gap_config_adv_data(&scan_rsp_data);
    if (ret)
    {
        ESP_LOGE(GATTS_TAG, "config scan response data failed, error code = %x", ret);
    }

    adv_config_done |= scan_rsp_config_flag;

    general_Taskhandle = (TaskHandle_t)taskhandle;
}

/// @brief init the ble functionality
/// @param
void ble_driver_init(void)
{

    esp_err_t err;
    // Initialize NVS.
    err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);

    ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT));

    // // turn off the power domain of the BT and Wifi system
    // esp_wifi_bt_power_domain_off();

    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    err = esp_bt_controller_init(&bt_cfg);
    ESP_BLE_ERR_CHECK(err);

    err = esp_bt_controller_enable(ESP_BT_MODE_BLE);
    ESP_BLE_ERR_CHECK(err);

    err = esp_bluedroid_init();
    ESP_BLE_ERR_CHECK(err);

    err = esp_bluedroid_enable();
    ESP_BLE_ERR_CHECK(err);

    err = esp_ble_gatts_register_callback(gatts_events_handler);
    ESP_BLE_ERR_CHECK(err);

    err = esp_ble_gap_register_callback(gap_event_handler);
    ESP_BLE_ERR_CHECK(err);

    // set the power level for BLE advertisement
    esp_ble_tx_power_set(ESP_BLE_PWR_TYPE_ADV, ESP_PWR_LVL_N12);

    ////////// registering app for event handlers
    if (esp_ble_gatts_app_register(PROFILE_DEVICE_INFORMATION) != ESP_OK)
        ESP_LOGE(TAG, "error in init the device info profile\r\n");

    if (esp_ble_gatts_app_register(PROFILE_BATT_SERVICE) != ESP_OK)
        ESP_LOGE(TAG, "error in init the batt servc profile\r\n");

    if (esp_ble_gatts_app_register(PROFILE_TDC_EEG) != ESP_OK)
        ESP_LOGE(TAG, "error in init the custom profile\r\n");

    ///// may be the max mtu size is 500 bytes
    esp_err_t local_mtu_ret = esp_ble_gatt_set_local_mtu(500);
    if (local_mtu_ret)
    {
        ESP_LOGE(GATTS_TAG, "set local  MTU failed, error code = %x\r\n", local_mtu_ret);
    }

    /////// above code id to implement BLE in peripheral mode , create the service , charcteristics , and handling events
}

/// @brief to start the advertisement again
/// @param
void ble_start_advertise(void)
{
    if (conn_flag == disconnected)
    {
        esp_ble_gap_start_advertising(&adv_params);
    }
}

/// @brief to stop the advertisement
/// @param
void ble_stop_advertise(void)
{
    esp_ble_gap_stop_advertising();
}

/// @brief to disconnect the peer device
/// @param
void ble_disconnect_device(void)
{
    if (conn_flag == connected)
    {
        esp_ble_gap_disconnect(peer_addr);
    }
}

/// @brief deinitialise the BLE driver
/// @param
void ble_driver_deinit(void)
{
    esp_wifi_stop();
    esp_bluedroid_disable();
    esp_bluedroid_deinit();
    esp_bt_controller_disable();
    esp_bt_controller_deinit();
    esp_wifi_bt_power_domain_off();
}
