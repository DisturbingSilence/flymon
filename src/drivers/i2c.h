#pragma once
#include <stdint.h>
#include "stm32f4xx_ll_i2c.h"

typedef void(*dma_callback_t)();
typedef struct
{
    DMA_TypeDef* dma;
    uint32_t stream;

    uint8_t* prefix_data;
    uint32_t prefix_len;

    uint32_t src;
    uint32_t dst;
    uint32_t len;

    dma_callback_t on_complete_callback;
} dma_transfer_t;


int i2c_init(I2C_TypeDef* i2cx);
int i2c_write(I2C_TypeDef* i2cx,uint8_t addr,const uint8_t* buf,uint32_t len);
int i2c_dma_write(I2C_TypeDef* i2cx,uint8_t addr,const dma_transfer_t* dma_info);
int i2c_dma_finish(I2C_TypeDef* i2cx);

// used together
int i2c_start_transaction(I2C_TypeDef* i2cx,uint8_t addr);
int i2c_write_bytes(I2C_TypeDef* i2cx,const uint8_t* buf,uint32_t len);
int i2c_end_transaction(I2C_TypeDef* i2cx);
