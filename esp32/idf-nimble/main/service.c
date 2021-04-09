/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "host/ble_hs.h"
#include "host/ble_uuid.h"
#include "services/gap/ble_svc_gap.h"
#include "services/gatt/ble_svc_gatt.h"
#include "service.h"
#include "freertos/semphr.h"

uint16_t tmp_temperature_handle;
uint16_t conn_handle;

SemaphoreHandle_t xSemaphore = NULL; 

/* Service UUID */
static const ble_uuid128_t gatt_svr_svc_sec_test_uuid =
    BLE_UUID128_INIT(0x03,0x00,0x13,0xAC,0x42,0x02,0xCD,0x8D,0xEB,0x11,0x3E,0x8E,0x56,0xF6,0x41,0x99); 

/* Unit Characteristic UUID */
static const ble_uuid128_t gatt_svr_char_unit_uuid =
    BLE_UUID128_INIT(0x03,0x00,0x13,0xAC,0x42,0x02,0xCD,0x8D,0xEB,0x11,0x3E,0x8E,0x38,0xFB,0x41,0x99); 
static const char *unit_descr = "Temperature unit"; 
static const char temp_descr[7] = {0x0E, 0xFE, //signed 16-bit
                                   0x2F, 0x27, //GATT Unit, temperature celsius 0x272F,  
                                   //0xAC, 0x27, //GATT Unit,0x27AC thermodynamic temperature (degree Fahrenheit)
                                   0x01, 0x00, 0x00};


/* Temperature Characteristic UUID */
static const ble_uuid16_t gatt_svr_char_temp_uuid = 
    BLE_UUID16_INIT(0x2A6E); 


temp_struct temp_char_value    = {0, 'C'};
unit_struct unit_char_value    = {'C'};


static int
ble_gatts_descriptor_access(uint16_t conn_handle,
                                     uint16_t attr_handle,
                                     struct ble_gatt_access_ctxt *ctxt,
                                     void *arg);


static int
gatt_svr_chr_access(uint16_t conn_handle, uint16_t attr_handle,
                                struct ble_gatt_access_ctxt *ctxt, void *arg);

static void get_temp(void);

static const struct ble_gatt_svc_def gatt_svr_svcs[] = {
    {
        /* Service: Thermometer */
        .type = BLE_GATT_SVC_TYPE_PRIMARY,
        .uuid =  &gatt_svr_svc_sec_test_uuid.u, 
        .characteristics = (struct ble_gatt_chr_def[])
        { {
                /* Characteristic: Temperature measurement */
                .uuid = &gatt_svr_char_temp_uuid.u, 
                .access_cb = gatt_svr_chr_access,
                .val_handle = &tmp_temperature_handle,
                .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_NOTIFY,
                .descriptors = (struct ble_gatt_dsc_def[]) { {
                    .uuid = BLE_UUID16_DECLARE(0x2904), //Characteristic User Descriptor
                    .att_flags = BLE_ATT_F_READ,
                    .access_cb = ble_gatts_descriptor_access,
                    }, 
                    {
                    0 /* no more descriptors */
                    } 
                }, 
            }, {
                /* Characteristic: Temperature Unit */
                .uuid = &gatt_svr_char_unit_uuid.u,
                .access_cb = gatt_svr_chr_access,
                .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_WRITE,
                .descriptors = (struct ble_gatt_dsc_def[]) { {
                    .uuid = BLE_UUID16_DECLARE(0x2901), //Characteristic User Descriptor
                    .att_flags = BLE_ATT_F_READ, // | BLE_ATT_F_WRITE,
                    .access_cb = ble_gatts_descriptor_access,
                    }, {
                    0 /* no more descriptors */
                    } 
                }, 
            }, {
                0, /* No more characteristics in this service */
            },
        }
    },


    {
        0, /* No more services */
    },
};

static int
gatt_svr_chr_write(struct os_mbuf *om, uint16_t min_len, uint16_t max_len,
                   void *dst, uint16_t *len)
{
    uint16_t om_len;
    int rc;

    om_len = OS_MBUF_PKTLEN(om);
    if (om_len < min_len || om_len > max_len) {
        return BLE_ATT_ERR_INVALID_ATTR_VALUE_LEN;
    }

    rc = ble_hs_mbuf_to_flat(om, dst, max_len, len);
    if (rc != 0) {
        return BLE_ATT_ERR_UNLIKELY;
    }

    return 0;
}


static int
gatt_svr_chr_access(uint16_t conn_handle, uint16_t attr_handle,
                                struct ble_gatt_access_ctxt *ctxt, void *arg)
{
    const ble_uuid_t *uuid;
    unit_struct tmpdata;
    int rc;

    uuid = ctxt->chr->uuid; 

    /* Determine which characteristic is being accessed by examining its
     * 128-bit UUID.
     */
    if (ble_uuid_cmp(uuid, &gatt_svr_char_unit_uuid.u) == 0) {

        switch (ctxt->op) {
        case BLE_GATT_ACCESS_OP_READ_CHR:
            rc = os_mbuf_append(ctxt->om, &unit_char_value, sizeof unit_char_value);
            return rc == 0 ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;

        case BLE_GATT_ACCESS_OP_WRITE_CHR:

            rc = gatt_svr_chr_write(ctxt->om,
                                    sizeof unit_char_value,
                                    sizeof unit_char_value,
                                    &tmpdata, NULL);

            if (unit_char_value.unit != tmpdata.unit) {
                //unit changed
                unit_char_value.unit = tmpdata.unit;
                send_temp_update();
            }

            return rc;

        default:
            assert(0);
            return BLE_ATT_ERR_UNLIKELY;
        }


    } else if (ble_uuid_cmp(uuid, &gatt_svr_char_temp_uuid.u) == 0) {

        ESP_LOGI("BLE","Read temp");

        assert(ctxt->op == BLE_GATT_ACCESS_OP_READ_CHR);
        get_temp();

        rc = os_mbuf_append(ctxt->om, &temp_char_value, sizeof temp_char_value);
        return rc == 0 ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;
    }

    assert(0);
    return BLE_ATT_ERR_UNLIKELY;
}

static int
ble_gatts_descriptor_access(uint16_t conn_handle,
                                     uint16_t attr_handle,
                                     struct ble_gatt_access_ctxt *ctxt,
                                     void *arg)
{
    int rc;
    uint16_t uuid;
    uuid = ble_uuid_u16(ctxt->chr->uuid);
     

    if (ctxt->op != BLE_GATT_ACCESS_OP_READ_DSC) {
        ESP_LOGE("BLE", "Invalid operation on Read-only Descriptor");
        return BLE_ATT_ERR_UNLIKELY;
    }

    //ESP_LOGI("BLE", "ble_gatts_descriptor_access OP:%d", ctxt->op);

    if (uuid == 0x2901) {
        rc = os_mbuf_append(ctxt->om, unit_descr, strlen(unit_descr));
        return rc == 0 ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;
    } else if (uuid == 0x2904) {
        rc = os_mbuf_append(ctxt->om, temp_descr, sizeof(temp_descr));
        return rc == 0 ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;
    }

    return BLE_ATT_ERR_UNLIKELY;
}

static void get_temp(void) {
    if( xSemaphoreTake( xSemaphore, ( TickType_t ) 100 ) == pdTRUE )
    {

        uint8_t unit = unit_char_value.unit;
        temp_char_value.unit = unit;
        //temp_char_value.temperature = temprature_sens_read()*10;
        temp_char_value.temperature = esp_random() % 10000;
        if (unit == 'F') {
            temp_char_value.temperature = (temp_char_value.temperature * 1.8) + 32;
        }

        xSemaphoreGive( xSemaphore );
    }
}

int send_temp_update(void) {
    struct os_mbuf *om;
    int rc;

    get_temp();
    if( xSemaphoreTake( xSemaphore, ( TickType_t ) 100 /* blocks at most 100 ticks */ ) == pdTRUE ) //portMAX_DELAY 
    {
        om = ble_hs_mbuf_from_flat((void *)&temp_char_value, sizeof(temp_char_value));
        xSemaphoreGive( xSemaphore );
        rc = ble_gattc_notify_custom(conn_handle, tmp_temperature_handle, om); //frees om
    } else {
        return 0;
    }
    return rc;
} 


void
gatt_svr_register_cb(struct ble_gatt_register_ctxt *ctxt, void *arg)
{
    char buf[BLE_UUID_STR_LEN];

    switch (ctxt->op) {
    case BLE_GATT_REGISTER_OP_SVC:
        ESP_LOGI("BLE", "registered service %s with handle=%d\n",
                    ble_uuid_to_str(ctxt->svc.svc_def->uuid, buf),
                    ctxt->svc.handle);
        break;

    case BLE_GATT_REGISTER_OP_CHR:
        ESP_LOGI("BLE", "registering characteristic %s with "
                    "def_handle=%d val_handle=%d\n",
                    ble_uuid_to_str(ctxt->chr.chr_def->uuid, buf),
                    ctxt->chr.def_handle,
                    ctxt->chr.val_handle);
        break;

    case BLE_GATT_REGISTER_OP_DSC:
        ESP_LOGI("BLE", "registering descriptor %s with handle=%d\n",
                    ble_uuid_to_str(ctxt->dsc.dsc_def->uuid, buf),
                    ctxt->dsc.handle);
        break;

    default:
        assert(0);
        break;
    }
}

int
gatt_svr_init(void)
{
    int rc;

    xSemaphore = xSemaphoreCreateMutex(); 

    ble_svc_gap_init();
    ble_svc_gatt_init();

    rc = ble_gatts_count_cfg(gatt_svr_svcs);
    if (rc != 0) {
        return rc;
    }

    rc = ble_gatts_add_svcs(gatt_svr_svcs);
    if (rc != 0) {
        return rc;
    }

    return 0;
}

