#include "i2c.h"
#include "stm32f4xx_ll_bus.h"
#include "err.h"
#include "dma.h"
int i2c_init(I2C_TypeDef* i2cx)
{
    if(!i2cx) return ERR_INV_ARG;

    if(i2cx == I2C1)
    {
         LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_I2C1);
    }
    else if(i2cx == I2C2)
    {
        LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_I2C2);
    }
    else if(i2cx == I2C3)
    {
        LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_I2C3);
    }
    else
    {
        return ERR_INV_ARG;
    }

    LL_I2C_InitTypeDef i2c1_cfg =
    {
        .PeripheralMode = LL_I2C_MODE_I2C,
        .ClockSpeed = 100000,
        .DutyCycle = LL_I2C_DUTYCYCLE_2,
        .OwnAddress1 = 0,
        .TypeAcknowledge = LL_I2C_ACK,
        .OwnAddrSize = LL_I2C_OWNADDRESS1_7BIT
    };
    if(LL_I2C_Init(i2cx,&i2c1_cfg) != SUCCESS)
    {
        return ERR_INIT_FAILURE;
    }
    LL_I2C_Enable(i2cx);
    return ERR_OK;
}
int i2c_start_transaction(I2C_TypeDef* i2cx,uint8_t addr)
{
    if(!i2cx) return ERR_INV_ARG;
    uint32_t timeout = 100000UL;
    while(LL_I2C_IsActiveFlag_BUSY(i2cx))
    {
        if(--timeout == 0) return ERR_TIMEOUT;
    }
    timeout = 100000UL;
    LL_I2C_GenerateStartCondition(i2cx);
    while(!LL_I2C_IsActiveFlag_SB(i2cx))
    {
        if(--timeout == 0) return ERR_TIMEOUT;
    }
    timeout = 100000UL;
    LL_I2C_TransmitData8(i2cx,addr);
    while(!LL_I2C_IsActiveFlag_ADDR(i2cx))
    {
        if(--timeout == 0) return ERR_TIMEOUT;
    }
    timeout = 100000UL;
    LL_I2C_ClearFlag_ADDR(i2cx);
    while(!LL_I2C_IsActiveFlag_TXE(i2cx))
    {
        if(--timeout == 0) return ERR_TIMEOUT;
    }
    return ERR_OK;
}
int i2c_write_bytes(I2C_TypeDef* i2cx,const uint8_t* buf,uint32_t len)
{
    if(!(i2cx && buf)) return ERR_INV_ARG;
    uint32_t timeout;
    for(const uint8_t* byte = buf;byte < buf + len;byte++)
    {
        timeout = 100000UL;
        LL_I2C_TransmitData8(i2cx,*byte);
        while(!LL_I2C_IsActiveFlag_TXE(i2cx))
        {
            if(--timeout == 0) return ERR_TIMEOUT;
        }
        timeout = 100000UL;
        while(!LL_I2C_IsActiveFlag_BTF(i2cx))
        {
            if(--timeout == 0) return ERR_TIMEOUT;
        }
    }
    return ERR_OK;
}
int i2c_end_transaction(I2C_TypeDef* i2cx)
{
    if(!i2cx) return ERR_INV_ARG;
    LL_I2C_GenerateStopCondition(i2cx);
    return ERR_OK;
}
int i2c_write(I2C_TypeDef* i2cx,uint8_t addr,const uint8_t* buf,uint32_t len)
{
    if(!(i2cx && buf)) return ERR_INV_ARG;
    RET_ERR(i2c_start_transaction(i2cx,addr));
    RET_ERR(i2c_write_bytes(i2cx,buf,len));
    RET_ERR(i2c_end_transaction(i2cx));
    return ERR_OK;
}
int i2c_dma_write(I2C_TypeDef* i2cx,uint8_t addr,const dma_transfer_t* dma_info)
{
    if(!dma_info || !i2cx) return ERR_INV_ARG;
    RET_ERR(i2c_start_transaction(i2cx,addr));
    if(dma_info->prefix_data)
    {
        RET_ERR(i2c_write_bytes(i2cx,dma_info->prefix_data,dma_info->prefix_len));
    }
    if(dma_info->on_complete_callback)
    {
        dma_set_callback(dma_info->stream,dma_info->on_complete_callback);
    }
    LL_I2C_EnableDMAReq_TX(i2cx);
    uint32_t err = dma_start(dma_info->dma,dma_info->stream,dma_info->src,dma_info->dst,dma_info->len);

    if(err != ERR_OK)
    {
        LL_I2C_DisableDMAReq_TX(i2cx);
        i2c_end_transaction(i2cx);
        return err;
    }
    return ERR_OK;
}
int i2c_dma_finish(I2C_TypeDef* i2cx)
{
    if(!i2cx) return ERR_INV_ARG;
    LL_I2C_DisableDMAReq_TX(i2cx);
    return i2c_end_transaction(i2cx);
}
