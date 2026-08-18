#pragma once
#include <drivers/peripherals/dma.h>

#include <stdint.h>

#include "stm32f4xx_ll_i2c.h"

typedef void(*dma_callback_t)(void*);
typedef struct
{
    uint8_t* prefix_data;
    uint32_t prefix_len;

    uint32_t src;
    uint32_t dst;
    uint32_t len;

    dma_callback_t on_tx_complete_callback;
    void* ctx;
} dma_transfer_t;

typedef struct
{
    I2C_TypeDef* instance;

    uint32_t timeout;
    dma_channel_t* tx_dma; //optional
} i2c_bus_t;

typedef struct
{
    i2c_bus_t* bus;
    uint8_t address;
} i2c_device_t;

int i2c_init(i2c_bus_t* dev,uint32_t clock_speed);

int i2c_write_bytes(i2c_device_t* dev,const uint8_t* buf,uint32_t len);
int i2c_read_bytes(i2c_device_t* dev,uint8_t* buf,uint32_t len);
int i2c_write(i2c_device_t* dev,const uint8_t* buf,uint32_t len);
int i2c_read(i2c_device_t* dev,uint8_t* buf,uint32_t len);
int i2c_write_read(i2c_device_t* mdev,const uint8_t* tx,uint32_t tx_len,uint8_t* rx,uint32_t rx_len);
int i2c_dma_finish(i2c_device_t* dev);
int i2c_dma_write(i2c_device_t *dev,dma_transfer_t *dma);
int i2c_probe(i2c_bus_t* bus,uint8_t address);
