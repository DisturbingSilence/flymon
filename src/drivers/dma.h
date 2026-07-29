#pragma once
#include <stdint.h>
#include "stm32f4xx_ll_dma.h"


typedef struct
{
    DMA_TypeDef* dma;
    uint32_t stream;
    uint32_t channel;
    uint32_t direction;
    uint32_t priority;
} dma_config_t;

typedef struct
{
    DMA_TypeDef* instance;
    uint32_t stream;
    uint32_t timeout;
} dma_channel_t;

typedef void(*dma_callback_t)(void*);
int dma_init(dma_channel_t* bus,const dma_config_t* cfg);
void dma_set_callback(dma_channel_t* bus,dma_callback_t clbck,void* ctx);
int dma_start(dma_channel_t* bus,uint32_t src,uint32_t dst,uint32_t len);
