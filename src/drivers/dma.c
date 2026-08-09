#include "dma.h"
#include "err.h"
#include "stm32f4xx_ll_bus.h"
#include <drivers/systime.h>
#include "core_cm4.h"
static dma_callback_t dma1_tx_callbacks[8] = {};
static void* dma1_context_ptrs[8] = {};

int dma_init(dma_channel_t* channel,const dma_config_t* cfg)
{
    if(!(cfg && channel)) return ERR_INV_ARG;
    channel->instance = cfg->dma;
    channel->stream = cfg->stream;
    if(cfg->dma == DMA1)
    {
        LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_DMA1);
    }
    else if(cfg->dma == DMA2)
    {
        LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_DMA2);
    }
    else
    {
        return ERR_INV_ARG;
    }

    LL_DMA_InitTypeDef dma_cfg =
    {
        .Direction = cfg->direction,
        .Mode = LL_DMA_MODE_NORMAL,
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
    NVIC_SetPriority(DMA1_Stream6_IRQn,1);
    NVIC_EnableIRQ(DMA1_Stream6_IRQn);
    return ERR_OK;
}
void DMA1_Stream6_IRQHandler()
{
    if(LL_DMA_IsActiveFlag_TC6(DMA1))
    {
        LL_DMA_ClearFlag_TC6(DMA1);
        if(dma1_tx_callbacks[6]) dma1_tx_callbacks[6](dma1_context_ptrs[6]);
    }
}
void dma_set_callback(dma_channel_t* channel,dma_callback_t clbck,void* ctx)
{
    if(!(channel && clbck)) return;
    if(channel->stream > 7) return;

    dma1_tx_callbacks[channel->stream] = clbck;
    dma1_context_ptrs[channel->stream] = ctx;
}
int dma_start(dma_channel_t* channel,uint32_t src,uint32_t dst,uint32_t len)
{
    LL_DMA_DisableStream(channel->instance,channel->stream);
    WAIT_TIMEOUT(LL_DMA_IsEnabledStream(channel->instance,channel->stream),channel->timeout);
    if (channel->instance == DMA1 && channel->stream == LL_DMA_STREAM_6)
    {
        LL_DMA_ClearFlag_TC6(DMA1);
        LL_DMA_ClearFlag_HT6(DMA1);
        LL_DMA_ClearFlag_TE6(DMA1);
        LL_DMA_ClearFlag_DME6(DMA1);
        LL_DMA_ClearFlag_FE6(DMA1);
    }
    LL_DMA_SetPeriphAddress(channel->instance,channel->stream,dst);
    LL_DMA_SetMemoryAddress(channel->instance,channel->stream,src);

    LL_DMA_SetDataLength(channel->instance,channel->stream,len);
    LL_DMA_EnableStream(channel->instance,channel->stream);
    return ERR_OK;
}
