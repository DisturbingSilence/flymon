#pragma once
#include <stdint.h>
#include <drivers/dma.h>
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
    I2C_TypeDef* bus;

    uint32_t timeout;
    uint32_t address;

    dma_channel_t* tx_dma; //optional
} i2c_bus_t;

int i2c_init(i2c_bus_t* dev);

int i2c_write_bytes(i2c_bus_t* dev,const uint8_t* buf,uint32_t len);
int i2c_read_bytes(i2c_bus_t* dev,uint8_t* buf,uint32_t len);
int i2c_write(i2c_bus_t* dev,const uint8_t* buf,uint32_t len);
int i2c_read(i2c_bus_t* dev,uint8_t* buf,uint32_t len);
int i2c_write_read(i2c_bus_t* mdev,const uint8_t* tx,uint32_t tx_len,uint8_t* rx,uint32_t rx_len);
int i2c_dma_finish(i2c_bus_t* dev);
int i2c_dma_write(i2c_bus_t *dev,dma_transfer_t *dma);
//int i2c_start(i2c_bus_t* dev);
/*
int i2c_write(I2C_TypeDef* i2cx,uint8_t addr,const uint8_t* buf,uint32_t len);
int i2c_read(I2C_TypeDef* i2cx,uint8_t addr,uint8_t* buf,uint32_t len);
int i2c_dma_write(I2C_TypeDef* i2cx,uint8_t addr,const dma_transfer_t* dma_info);
int i2c_dma_finish(I2C_TypeDef* i2cx);


// used together
int i2c_start_transaction(I2C_TypeDef* i2cx,uint8_t addr,bool read);
int i2c_write_bytes(I2C_TypeDef* i2cx,const uint8_t* buf,uint32_t len);
int i2c_read_bytes(I2C_TypeDef* i2cx,uint8_t* buf,uint32_t len);
int i2c_end_transaction(I2C_TypeDef* i2cx);*/
