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

typedef void(*dma_callback_t)();

int dma_init(const dma_config_t* cfg);
void dma_set_callback(uint32_t stream,dma_callback_t clbck);
int dma_start(DMA_TypeDef* dma,uint32_t stream,uint32_t src,uint32_t dst,uint32_t len);
