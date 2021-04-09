/*
   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_bt.h"

#include "esp_gap_ble_api.h"
#include "esp_gatts_api.h"
#include "esp_bt_main.h"
#include "esp_gatt_common_api.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>  

#define GATTS_TABLE_TAG            "BLE"

#define PROFILE_NUM                 1
#define PROFILE_APP_IDX             0
#define ESP_APP_ID                  0x55
#define SVC_INST_ID                 0

TaskHandle_t xHandle = NULL;
SemaphoreHandle_t xSemaphore = NULL;

struct gatts_profile_inst {
    esp_gatts_cb_t gatts_cb;
    uint16_t gatts_if;
    uint16_t app_id;
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

static void gatts_profile_event_handler(esp_gatts_cb_event_t event,
					esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param);

/* One gatt-based profile one app_id and one gatts_if, this array will store the gatts_if returned by ESP_GATTS_REG_EVT */
static struct gatts_profile_inst thermometer_profile_tab[PROFILE_NUM] = {
    [PROFILE_APP_IDX] = {
        .gatts_cb = gatts_profile_event_handler,
        .gatts_if = ESP_GATT_IF_NONE,       /* Not get the gatt_if, so initial is ESP_GATT_IF_NONE */
    },
};

#include "service.h"
#include "advertisement.h"


extern uint8_t temprature_sens_read();

static void get_temp(void) {
    if( xSemaphoreTake( xSemaphore, ( TickType_t ) 10 ) == pdTRUE )
    {

        uint8_t unit = unit_char_value;
        temp_char_value.unit = unit;
        //temp_char_value.temperature = temprature_sens_read()*10;
        temp_char_value.temperature = esp_random() % 10000;
        if (unit == 'F') {
            temp_char_value.temperature = (temp_char_value.temperature * 1.8) + 32;
        }

        xSemaphoreGive( xSemaphore );
    }
}

static void send_temp_update(void) {
    get_temp();
    if( xSemaphoreTake( xSemaphore, ( TickType_t ) 10 ) == pdTRUE )
    {

        esp_err_t ret = esp_ble_gatts_send_indicate(thermometer_profile_tab[PROFILE_APP_IDX].gatts_if, 
             thermometer_profile_tab[PROFILE_APP_IDX].conn_id,
             thermometer_handle_table[IDX_CHAR_TEMP_VAL],
             sizeof(temp_char_value), (uint8_t*)&temp_char_value, false);
        
        xSemaphoreGive( xSemaphore );

        if (ret){
            ESP_LOGE(GATTS_TABLE_TAG, "Send indication, error code = %x", ret);
        }

    }
}

// Perform an action every 5 seconds.
void periodic_task( void * pvParameters )
{
    const TickType_t xPeriod =  5000 / portTICK_RATE_MS;
    uint32_t ulNotifiedValue;

    for( ;; )
    {
        xTaskNotifyWait(  0x00,      /* Don't clear any notification bits on entry. */
                          0x00,      /* Don't clear any notification bits on exit. */ 
                          &ulNotifiedValue, /* Notified value pass out in ulNotifiedValue. */
                          xPeriod );  /* Block up to xPeriod ticks */
        //ESP_LOGI(GATTS_TABLE_TAG, "notify flags: %d", ulNotifiedValue);
        if (( ulNotifiedValue & 0x01 ) == 0 ) continue;

        // Perform action here.
        ESP_LOGI(GATTS_TABLE_TAG, "send notification, conn:%d", thermometer_profile_tab[PROFILE_APP_IDX].conn_id);
        send_temp_update();
    }
}


static void gatts_profile_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param)
{
    switch (event) {
        case ESP_GATTS_REG_EVT:{
            esp_err_t set_dev_name_ret = esp_ble_gap_set_device_name(DEVICE_NAME);
            if (set_dev_name_ret){
                ESP_LOGE(GATTS_TABLE_TAG, "set device name failed, error code = %x", set_dev_name_ret);
            }

            esp_err_t set_dev_icon_ret = esp_ble_gap_config_local_icon(adv_data.appearance);
            if (set_dev_icon_ret){
                ESP_LOGE(GATTS_TABLE_TAG, "set device appearance failed, error code = %x", set_dev_icon_ret);
            }

            //config adv data
            esp_err_t ret = esp_ble_gap_config_adv_data(&adv_data);
            if (ret){
                ESP_LOGE(GATTS_TABLE_TAG, "config adv data failed, error code = %x", ret);
            }
            adv_config_done |= ADV_CONFIG_FLAG;
            //config scan response data
            ret = esp_ble_gap_config_adv_data(&scan_rsp_data);
            if (ret){
                ESP_LOGE(GATTS_TABLE_TAG, "config scan response data failed, error code = %x", ret);
            }
            adv_config_done |= SCAN_RSP_CONFIG_FLAG;
            esp_err_t create_attr_ret = esp_ble_gatts_create_attr_tab(gatt_db, gatts_if, IDX_SVC_END, SVC_INST_ID);
            if (create_attr_ret){
                ESP_LOGE(GATTS_TABLE_TAG, "create attr table failed, error code = %x", create_attr_ret);
            }
        }
       	    break;
        case ESP_GATTS_READ_EVT:
            ESP_LOGI(GATTS_TABLE_TAG, "ESP_GATTS_READ_EVT");
            if (thermometer_handle_table[IDX_CHAR_TEMP_VAL] == param->read.handle) {
                get_temp();
                if (gatt_db[IDX_CHAR_TEMP_VAL].attr_control.auto_rsp == ESP_GATT_RSP_BY_APP) {
                    //reponse by APP
                    esp_gatt_rsp_t rsp;
                    memset(&rsp, 0, sizeof(esp_gatt_rsp_t));
                    rsp.attr_value.handle = param->read.handle;
                    rsp.attr_value.len = sizeof(temp_char_value);
                    memcpy(rsp.attr_value.value, (uint8_t*)&temp_char_value, sizeof(temp_char_value));

                    esp_err_t ret = esp_ble_gatts_send_response(gatts_if, param->read.conn_id, param->read.trans_id, ESP_GATT_OK, &rsp);
                    if (ret){
                        ESP_LOGE(GATTS_TABLE_TAG, "set response failed, error code = %x", ret);
                    }
                } else {
                    //autoresponse
                    esp_err_t ret = esp_ble_gatts_set_attr_value(thermometer_handle_table[IDX_CHAR_TEMP_VAL],sizeof(temp_char_value),(uint8_t*)&temp_char_value);
                    if (ret){
                        ESP_LOGE(GATTS_TABLE_TAG, "set attr value failed, error code = %x", ret);
                    }
                }
            }
       	    break;
               
        case ESP_GATTS_WRITE_EVT:
            if (!param->write.is_prep){
                // the data length of gattc write  must be less than GATTS_DEMO_CHAR_VAL_LEN_MAX.
                ESP_LOGI(GATTS_TABLE_TAG, "GATT_WRITE_EVT, handle = %d, value len = %d, value :", param->write.handle, param->write.len);
                esp_log_buffer_hex(GATTS_TABLE_TAG, param->write.value, param->write.len);

                //handle indication and notification configuration
                if (thermometer_handle_table[IDX_CHAR_TEMP_CFG] == param->write.handle && param->write.len == 2){
                    uint16_t descr_value = param->write.value[1]<<8 | param->write.value[0];
                    if (descr_value == 0x0001){
                        ESP_LOGI(GATTS_TABLE_TAG, "notify enable");
                        //the size of notify_data[] need less than MTU size

                        vTaskDelay(100/portTICK_RATE_MS);
                        send_temp_update();
                        
                        xTaskNotify( xHandle, 0x01, eSetValueWithOverwrite );
//                        vTaskResume(xHandle); //don't use since it can stop withing a critical section => deadlock

                    /*
                    }else if (descr_value == 0x0002){
                        ESP_LOGI(GATTS_TABLE_TAG, "indicate enable");
                        uint8_t indicate_data[15];
                        for (int i = 0; i < sizeof(indicate_data); ++i)
                        {
                            indicate_data[i] = i % 0xff;
                        }
                        //the size of indicate_data[] need less than MTU size
                        esp_ble_gatts_send_indicate(gatts_if, param->write.conn_id, thermometer_handle_table[IDX_CHAR_TEMP_VAL],
                                            sizeof(indicate_data), indicate_data, true);
                    }*/
                    }else if (descr_value == 0x0000){
                        ESP_LOGI(GATTS_TABLE_TAG, "notify/indicate disable ");
                        xTaskNotify( xHandle, 0x00, eSetValueWithOverwrite );
//                        vTaskSuspend(xHandle);
                    }else{
                        ESP_LOGE(GATTS_TABLE_TAG, "unknown descr value");
                        esp_log_buffer_hex(GATTS_TABLE_TAG, param->write.value, param->write.len);
                    }

                //handle UNIT write
                } else if (thermometer_handle_table[IDX_CHAR_UNIT_VAL] == param->write.handle && param->write.len == 1){
                    uint8_t requnit = (param->write.value[0] == 'F') ? 'F' : 'C';
                    if (requnit != unit_char_value) {
                        //unit changed
                        unit_char_value = requnit;
                        send_temp_update();
                    }
                }
                /* send response when param->write.need_rsp is true*/
                if (param->write.need_rsp){
                    esp_ble_gatts_send_response(gatts_if, param->write.conn_id, param->write.trans_id, ESP_GATT_OK, NULL);
                }
            }else{
                /* prepare write not supported */
                ESP_LOGE(GATTS_TABLE_TAG, "prepare write not implemented");
            }
      	    break;
        case ESP_GATTS_EXEC_WRITE_EVT: 
            // the length of gattc prepare write data must be less than GATTS_DEMO_CHAR_VAL_LEN_MAX. 
            ESP_LOGE(GATTS_TABLE_TAG, "ESP_GATTS_EXEC_WRITE_EVT");
            break;
        case ESP_GATTS_MTU_EVT:
            ESP_LOGI(GATTS_TABLE_TAG, "ESP_GATTS_MTU_EVT, MTU %d", param->mtu.mtu);
            break;
        case ESP_GATTS_CONF_EVT:
            ESP_LOGI(GATTS_TABLE_TAG, "ESP_GATTS_CONF_EVT, status = %d, attr_handle %d", param->conf.status, param->conf.handle);
            break;
        case ESP_GATTS_START_EVT:
            ESP_LOGI(GATTS_TABLE_TAG, "SERVICE_START_EVT, status %d, service_handle %d", param->start.status, param->start.service_handle);
            break;
        case ESP_GATTS_CONNECT_EVT:
            ESP_LOGI(GATTS_TABLE_TAG, "ESP_GATTS_CONNECT_EVT, conn_id = %d", param->connect.conn_id);
            esp_log_buffer_hex(GATTS_TABLE_TAG, param->connect.remote_bda, 6);
            esp_ble_conn_update_params_t conn_params = {0};
            memcpy(conn_params.bda, param->connect.remote_bda, sizeof(esp_bd_addr_t));
            /* For the iOS system, please refer to Apple official documents about the BLE connection parameters restrictions. */
            conn_params.latency = 0;
            conn_params.max_int = 0x20;    // max_int = 0x20*1.25ms = 40ms
            conn_params.min_int = 0x10;    // min_int = 0x10*1.25ms = 20ms
            conn_params.timeout = 400;    // timeout = 400*10ms = 4000ms
            //start sent the update connection parameters to the peer device.
            esp_ble_gap_update_conn_params(&conn_params);
            break;
        case ESP_GATTS_DISCONNECT_EVT:
            ESP_LOGI(GATTS_TABLE_TAG, "ESP_GATTS_DISCONNECT_EVT, reason = 0x%x", param->disconnect.reason);
            xTaskNotify( xHandle, 0x00, eSetValueWithOverwrite );
//            vTaskSuspend(xHandle);
            esp_ble_gap_start_advertising(&adv_params);
            break;
        case ESP_GATTS_CREAT_ATTR_TAB_EVT:{
            if (param->add_attr_tab.status != ESP_GATT_OK){
                ESP_LOGE(GATTS_TABLE_TAG, "create attribute table failed, error code=0x%x", param->add_attr_tab.status);
            }
            else if (param->add_attr_tab.num_handle != IDX_SVC_END){
                ESP_LOGE(GATTS_TABLE_TAG, "create attribute table abnormally, num_handle (%d) \
                        doesn't equal to SVC_IDX_NB(%d)", param->add_attr_tab.num_handle, IDX_SVC_END);
            }
            else {
                ESP_LOGI(GATTS_TABLE_TAG, "create attribute table successfully, the number handle = %d\n",param->add_attr_tab.num_handle);
                memcpy(thermometer_handle_table, param->add_attr_tab.handles, sizeof(thermometer_handle_table));
                esp_ble_gatts_start_service(thermometer_handle_table[IDX_SVC]);
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


static void gatts_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param)
{

    /* If event is register event, store the gatts_if for each profile */
    if (event == ESP_GATTS_REG_EVT) {
        if (param->reg.status == ESP_GATT_OK) {
            thermometer_profile_tab[PROFILE_APP_IDX].gatts_if = gatts_if;
        } else {
            ESP_LOGE(GATTS_TABLE_TAG, "reg app failed, app_id %04x, status %d",
                    param->reg.app_id,
                    param->reg.status);
            return;
        }
    }
    do {
        int idx;
        for (idx = 0; idx < PROFILE_NUM; idx++) {
            /* ESP_GATT_IF_NONE, not specify a certain gatt_if, need to call every profile cb function */
            if (gatts_if == ESP_GATT_IF_NONE || gatts_if == thermometer_profile_tab[idx].gatts_if) {
                if (thermometer_profile_tab[idx].gatts_cb) {
                    thermometer_profile_tab[idx].gatts_cb(event, gatts_if, param);
                }
            }
        }
    } while (0);
}

void app_main(void)
{
    esp_err_t ret;

    xSemaphore = xSemaphoreCreateMutex();
    xTaskCreate(&periodic_task, "ble_task", 2048, NULL, 5, &xHandle);
//    vTaskSuspend( xHandle );


    ESP_LOGI(GATTS_TABLE_TAG, "Adv data len: %d, Scan resp data len: %d", ESP_BLE_ADV_DATA_LEN_MAX, ESP_BLE_SCAN_RSP_DATA_LEN_MAX); 

    /* Initialize NVS. */
    ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK( ret );

    ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT));

    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    ret = esp_bt_controller_init(&bt_cfg);
    if (ret) {
        ESP_LOGE(GATTS_TABLE_TAG, "%s enable controller failed: %s", __func__, esp_err_to_name(ret));
        return;
    }

    ret = esp_bt_controller_enable(ESP_BT_MODE_BLE);
    if (ret) {
        ESP_LOGE(GATTS_TABLE_TAG, "%s enable controller failed: %s", __func__, esp_err_to_name(ret));
        return;
    }

    ret = esp_bluedroid_init();
    if (ret) {
        ESP_LOGE(GATTS_TABLE_TAG, "%s init bluetooth failed: %s", __func__, esp_err_to_name(ret));
        return;
    }

    ret = esp_bluedroid_enable();
    if (ret) {
        ESP_LOGE(GATTS_TABLE_TAG, "%s enable bluetooth failed: %s", __func__, esp_err_to_name(ret));
        return;
    }

    ret = esp_ble_gatts_register_callback(gatts_event_handler);
    if (ret){
        ESP_LOGE(GATTS_TABLE_TAG, "gatts register error, error code = %x", ret);
        return;
    }

    ret = esp_ble_gap_register_callback(gap_event_handler);
    if (ret){
        ESP_LOGE(GATTS_TABLE_TAG, "gap register error, error code = %x", ret);
        return;
    }

    ret = esp_ble_gatts_app_register(ESP_APP_ID);
    if (ret){
        ESP_LOGE(GATTS_TABLE_TAG, "gatts app register error, error code = %x", ret);
        return;
    }

    esp_err_t local_mtu_ret = esp_ble_gatt_set_local_mtu(500);
    if (local_mtu_ret){
        ESP_LOGE(GATTS_TABLE_TAG, "set local  MTU failed, error code = %x", local_mtu_ret);
    }

}
