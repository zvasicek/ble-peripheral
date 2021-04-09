#define DEVICE_NAME          "Thermometer"

/* Attributes State Machine */
enum
{
    IDX_SVC,
    IDX_CHAR_TEMP,
    IDX_CHAR_TEMP_VAL,
    IDX_CHAR_TEMP_CFG,
    IDX_CHAR_TEMP_FMT,

    IDX_CHAR_UNIT,
    IDX_CHAR_UNIT_VAL,

    IDX_SVC_END,
};

/* Data */
typedef struct {
    uint16_t temperature;
    uint8_t unit;
} __attribute__ ((packed)) temp_struct;

static temp_struct temp_char_value    = {0, 'C'};
static uint8_t unit_char_value   = {'C'};


/* Service */
static const uint8_t  GATTS_SERVICE_UUID[16]    = {0x03,0x00,0x13,0xAC,0x42,0x02,0xCD,0x8D,0xEB,0x11,0x3E,0x8E,0x56,0xF6,0x41,0x99};
static const uint16_t GATTS_CHAR_UUID_TEMP      = 0x2A6E;
static const uint8_t  GATTS_CHAR_UUID_UNIT[16]  = {0x03,0x00,0x13,0xAC,0x42,0x02,0xCD,0x8D,0xEB,0x11,0x3E,0x8E,0x38,0xFB,0x41,0x99};


static const uint16_t primary_service_uuid         = ESP_GATT_UUID_PRI_SERVICE; 
static const uint16_t character_declaration_uuid   = ESP_GATT_UUID_CHAR_DECLARE;
static const uint16_t character_format_uuid        = ESP_GATT_UUID_CHAR_PRESENT_FORMAT;
static const uint16_t character_client_config_uuid = ESP_GATT_UUID_CHAR_CLIENT_CONFIG;
//static const uint8_t char_prop_read                = ESP_GATT_CHAR_PROP_BIT_READ;
static const uint8_t char_prop_read_write          = ESP_GATT_CHAR_PROP_BIT_READ | ESP_GATT_CHAR_PROP_BIT_WRITE;
static const uint8_t char_prop_read_notify         = ESP_GATT_CHAR_PROP_BIT_READ | ESP_GATT_CHAR_PROP_BIT_NOTIFY;
static const uint8_t temperature_desc_ccc[2]      = {0x00, 0x00};
static const uint8_t temperature_desc_fmt[7]      = {0x0E, 0xFE, //signed 16-bit
                                                      0x2F, 0x27, //GATT Unit, temperature celsius 0x272F,  
                                                     //#0xAC, 0x27, #GATT Unit,0x27AC thermodynamic temperature (degree Fahrenheit)
                                                     0x01, 0x00, 0x00};

/* Full Database Description - Used to add attributes into the database */
static const esp_gatts_attr_db_t gatt_db[IDX_SVC_END] =
{
    // Service Declaration
    [IDX_SVC]        =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&primary_service_uuid, ESP_GATT_PERM_READ,
      sizeof(uint16_t), sizeof(GATTS_SERVICE_UUID), (uint8_t *)&GATTS_SERVICE_UUID}},

    /* Characteristic Declaration */
    [IDX_CHAR_TEMP]     =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_declaration_uuid, ESP_GATT_PERM_READ,
      sizeof(uint8_t),  sizeof(uint8_t), (uint8_t *)&char_prop_read_notify}},

    /* Characteristic Value */
    [IDX_CHAR_TEMP_VAL] =
    //a) response sent by APP
    {{ESP_GATT_RSP_BY_APP}, {ESP_UUID_LEN_16, (uint8_t *)&GATTS_CHAR_UUID_TEMP, ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
    //b) response sent automatically
//    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&GATTS_CHAR_UUID_TEMP, ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
      sizeof(temp_char_value) /* max data length */, sizeof(temp_char_value) /* current length */, (uint8_t *)&temp_char_value}},

    /* Client Characteristic Configuration Descriptor */
    [IDX_CHAR_TEMP_CFG]  =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_client_config_uuid, ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
      sizeof(uint16_t), sizeof(temperature_desc_ccc), (uint8_t *)temperature_desc_ccc}},

    [IDX_CHAR_TEMP_FMT]  =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_format_uuid, ESP_GATT_PERM_READ,
      sizeof(temperature_desc_fmt), sizeof(temperature_desc_fmt), (uint8_t *)temperature_desc_fmt}},

    /* Characteristic Declaration */
    [IDX_CHAR_UNIT]      =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_16, (uint8_t *)&character_declaration_uuid, ESP_GATT_PERM_READ,
      sizeof(uint8_t),  sizeof(uint8_t), (uint8_t *)&char_prop_read_write}},

    /* Characteristic Value */
    [IDX_CHAR_UNIT_VAL]  =
    {{ESP_GATT_AUTO_RSP}, {ESP_UUID_LEN_128, (uint8_t *)&GATTS_CHAR_UUID_UNIT, ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
      sizeof(unit_char_value) /* max data length */, sizeof(unit_char_value) /* current length */, (uint8_t *)&unit_char_value}},

};


uint16_t thermometer_handle_table[IDX_SVC_END];
