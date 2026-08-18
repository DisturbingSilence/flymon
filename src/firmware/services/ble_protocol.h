#pragma once
#include <drivers/devices/ble.h>

typedef struct
{
    ble_device_t* ble;
} ble_protocol_t;

typedef struct
{
    ble_device_t* ble;
} ble_protocol_cfg_t;

int ble_protocol_init(ble_protocol_t* protocol,const ble_protocol_cfg_t* cfg);
