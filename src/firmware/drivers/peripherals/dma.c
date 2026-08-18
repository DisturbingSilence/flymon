#include <drivers/peripherals/dma.h>
#include <drivers/systime.h>
#include <drivers/err.h>

#include "core_cm4.h"
#include "stm32f4xx_ll_bus.h"

static dma_callback_t dma1_callbacks[8];
static void* dma1_contexts[8];

static dma_callback_t dma2_callbacks[8];
static void* dma2_contexts[8];

static void dma_clear_flags(DMA_TypeDef* dma,uint32_t stream);
int dma_init(dma_channel_t* channel,const dma_config_t* cfg)
{
    if(!(cfg && channel)) return ERR_INV_ARG;
    channel->instance = cfg->dma;
    channel->stream = cfg->stream;
    channel->timeout = cfg->timeout;
    if(cfg->dma == DMA1)
        LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_DMA1);
    else if(cfg->dma == DMA2)
        LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_DMA2);
    else
        return ERR_INV_ARG;

    LL_DMA_InitTypeDef dma_cfg =
    {
        .Direction = cfg->direction,
        .Mode = cfg->mode,
        .PeriphOrM2MSrcIncMode = LL_DMA_PERIPH_NOINCREMENT,
        .MemoryOrM2MDstIncMode = LL_DMA_MEMORY_INCREMENT,
        .PeriphOrM2MSrcDataSize = LL_DMA_PDATAALIGN_BYTE,
        .MemoryOrM2MDstDataSize = LL_DMA_MDATAALIGN_BYTE,
        .Channel = cfg->channel,
        .Priority = cfg->priority,
        .FIFOMode = LL_DMA_FIFOMODE_DISABLE,
        .FIFOThreshold = LL_DMA_FIFOTHRESHOLD_1_4,
        .MemBurst = LL_DMA_MBURST_SINGLE,
        .PeriphBurst = LL_DMA_PBURST_SINGLE
    };
    if(LL_DMA_Init(cfg->dma,cfg->stream,&dma_cfg) != SUCCESS)
    {
        return ERR_INIT_FAILURE;
    }
    LL_DMA_EnableIT_TC(cfg->dma,cfg->stream);
    NVIC_SetPriority(cfg->irq,1);
    NVIC_EnableIRQ(cfg->irq);
    dma_clear_flags(cfg->dma,cfg->stream);
    return ERR_OK;
}
static void dma_clear_flags(DMA_TypeDef* dma,uint32_t stream)
{
    switch (stream)
    {
    case LL_DMA_STREAM_0:
        LL_DMA_ClearFlag_TC0(dma);
        LL_DMA_ClearFlag_HT0(dma);
        LL_DMA_ClearFlag_TE0(dma);
        LL_DMA_ClearFlag_DME0(dma);
        LL_DMA_ClearFlag_FE0(dma);
        break;
    case LL_DMA_STREAM_1:
        LL_DMA_ClearFlag_TC1(dma);
        LL_DMA_ClearFlag_HT1(dma);
        LL_DMA_ClearFlag_TE1(dma);
        LL_DMA_ClearFlag_DME1(dma);
        LL_DMA_ClearFlag_FE1(dma);
        break;
    case LL_DMA_STREAM_2:
        LL_DMA_ClearFlag_TC2(dma);
        LL_DMA_ClearFlag_HT2(dma);
        LL_DMA_ClearFlag_TE2(dma);
        LL_DMA_ClearFlag_DME2(dma);
        LL_DMA_ClearFlag_FE2(dma);
        break;
    case LL_DMA_STREAM_3:
        LL_DMA_ClearFlag_TC3(dma);
        LL_DMA_ClearFlag_HT3(dma);
        LL_DMA_ClearFlag_TE3(dma);
        LL_DMA_ClearFlag_DME3(dma);
        LL_DMA_ClearFlag_FE3(dma);
        break;
    case LL_DMA_STREAM_4:
        LL_DMA_ClearFlag_TC4(dma);
        LL_DMA_ClearFlag_HT4(dma);
        LL_DMA_ClearFlag_TE4(dma);
        LL_DMA_ClearFlag_DME4(dma);
        LL_DMA_ClearFlag_FE4(dma);
        break;
    case LL_DMA_STREAM_5:
        LL_DMA_ClearFlag_TC5(dma);
        LL_DMA_ClearFlag_HT5(dma);
        LL_DMA_ClearFlag_TE5(dma);
        LL_DMA_ClearFlag_DME5(dma);
        LL_DMA_ClearFlag_FE5(dma);
        break;
    case LL_DMA_STREAM_6:
        LL_DMA_ClearFlag_TC6(dma);
        LL_DMA_ClearFlag_HT6(dma);
        LL_DMA_ClearFlag_TE6(dma);
        LL_DMA_ClearFlag_DME6(dma);
        LL_DMA_ClearFlag_FE6(dma);
        break;
    case LL_DMA_STREAM_7:
        LL_DMA_ClearFlag_TC7(dma);
        LL_DMA_ClearFlag_HT7(dma);
        LL_DMA_ClearFlag_TE7(dma);
        LL_DMA_ClearFlag_DME7(dma);
        LL_DMA_ClearFlag_FE7(dma);
        break;
    }
}
static void generic_dma_irq_handler(DMA_TypeDef* dma,uint32_t stream)
{
    void(**callback_array)(void* ctx);
    void** context_array;
    dma_callback_t callback = 0;
    void* ctx = 0;
    if(dma == DMA1)
    {
        callback_array = dma1_callbacks;
        context_array = dma1_contexts;
    }
    else
    {
        callback_array = dma2_callbacks;
        context_array = dma2_contexts;
    }
    switch (stream)
    {
    case LL_DMA_STREAM_0:
        if (LL_DMA_IsActiveFlag_TC0(dma))
        {
            LL_DMA_ClearFlag_TC0(dma);
            callback = callback_array[0];
            ctx = context_array[0];
        }
        break;
    case LL_DMA_STREAM_1:
        if (LL_DMA_IsActiveFlag_TC1(dma))
        {
            LL_DMA_ClearFlag_TC1(dma);
            callback = callback_array[1];
            ctx = context_array[1];
        }
        break;
    case LL_DMA_STREAM_2:
        if (LL_DMA_IsActiveFlag_TC2(dma))
        {
            LL_DMA_ClearFlag_TC2(dma);
            callback = callback_array[2];
            ctx = context_array[2];
        }
        break;
    case LL_DMA_STREAM_3:
        if (LL_DMA_IsActiveFlag_TC3(dma))
        {
            LL_DMA_ClearFlag_TC3(dma);
            callback = callback_array[3];
            ctx = context_array[3];
        }
        break;
    case LL_DMA_STREAM_4:
        if (LL_DMA_IsActiveFlag_TC4(dma))
        {
            LL_DMA_ClearFlag_TC4(dma);
            callback = callback_array[4];
            ctx = context_array[4];
        }
        break;
    case LL_DMA_STREAM_5:
        if (LL_DMA_IsActiveFlag_TC5(dma))
        {
            LL_DMA_ClearFlag_TC5(dma);
            callback = callback_array[5];
            ctx = context_array[5];
        }
        break;
    case LL_DMA_STREAM_6:
        if (LL_DMA_IsActiveFlag_TC6(dma))
        {
            LL_DMA_ClearFlag_TC6(dma);
            callback = callback_array[6];
            ctx = context_array[6];
        }
        break;

    case LL_DMA_STREAM_7:
        if (LL_DMA_IsActiveFlag_TC7(dma))
        {
            LL_DMA_ClearFlag_TC7(dma);
            callback = callback_array[7];
            ctx = context_array[7];
        }
        break;
    }
    if (callback) callback(ctx);
}

void DMA1_Stream6_IRQHandler() { generic_dma_irq_handler(DMA1,LL_DMA_STREAM_6); }
void DMA2_Stream2_IRQHandler() { generic_dma_irq_handler(DMA2,LL_DMA_STREAM_2); }
void DMA2_Stream7_IRQHandler() { generic_dma_irq_handler(DMA2,LL_DMA_STREAM_7); }

void dma_set_callback(dma_channel_t* channel,dma_callback_t clbck,void* ctx)
{
    if(!(channel && clbck)) return;
    if(channel->stream > LL_DMA_STREAM_7) return;

    if(channel->instance == DMA1)
    {
        dma1_callbacks[channel->stream] = clbck;
        dma1_contexts[channel->stream] = ctx;
    }
    else if(channel->instance == DMA2)
    {
        dma2_callbacks[channel->stream] = clbck;
        dma2_contexts[channel->stream] = ctx;
    }
}
int dma_start(dma_channel_t* channel,uint32_t src,uint32_t dst,uint16_t len)
{
    if(!(channel && len > 0 && dst && src)) return ERR_INV_ARG;
    LL_DMA_DisableStream(channel->instance,channel->stream);
    WAIT_TIMEOUT(LL_DMA_IsEnabledStream(channel->instance,channel->stream),channel->timeout);
    dma_clear_flags(channel->instance,channel->stream);
    LL_DMA_SetPeriphAddress(channel->instance,channel->stream,dst);
    LL_DMA_SetMemoryAddress(channel->instance,channel->stream,src);

    LL_DMA_SetDataLength(channel->instance,channel->stream,len);
    LL_DMA_EnableStream(channel->instance,channel->stream);
    return ERR_OK;
}
uint16_t dma_get_remaining(const dma_channel_t* channel)
{
    if (!channel) return 0;
    return (uint16_t)LL_DMA_GetDataLength(channel->instance,channel->stream);
}
