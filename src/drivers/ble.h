#pragma once
#include <stdint.h>
#include <drivers/usart.h>
#include "stm32f4xx_ll_gpio.h"
typedef struct
{
    usart_bus_t* usart_bus;
    GPIO_TypeDef* pwrc_port;
    uint32_t pwrc_pinmask;
    uint8_t version[20];
    uint8_t broadcast_name[25];
} ble_device_t;

typedef struct
{
    usart_bus_t* bus;
    GPIO_TypeDef* pwrc_port;
    uint32_t pwrc_pinmask;
} ble_config_t;
int ble_init(ble_device_t* device,const ble_config_t* cfg);
int ble_read(ble_device_t* device,uint8_t* data,uint16_t* size);
int ble_write(ble_device_t* dev,const uint8_t* data,uint16_t size);
int ble_disconnect(ble_device_t* device);
uint32_t ble_available(ble_device_t* device);
