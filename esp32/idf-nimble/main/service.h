#ifndef H_BLETEMP_SENSOR_
#define H_BLETEMP_SENSOR_

#include "nimble/ble.h"

#ifdef __cplusplus
extern "C" {
#endif

extern uint16_t tmp_temperature_handle;
extern uint16_t conn_handle;

/* Data */
typedef struct {
    uint16_t temperature;
    uint8_t unit;
} __attribute__ ((packed)) temp_struct;

typedef struct {
    uint8_t unit;
} __attribute__ ((packed)) unit_struct;

extern temp_struct temp_char_value;
extern unit_struct unit_char_value; 

struct ble_hs_cfg;
struct ble_gatt_register_ctxt;

void gatt_svr_register_cb(struct ble_gatt_register_ctxt *ctxt, void *arg);
int gatt_svr_init(void);

int send_temp_update(void);

#ifdef __cplusplus
}
#endif

#endif
