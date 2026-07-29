#include "i2c.h"
#include "stm32f4xx_ll_bus.h"
#include "err.h"
#include "dma.h"
#include "systime.h"


int i2c_init(i2c_bus_t* device)
{
    if(!device) return ERR_INV_ARG;
    if(device->bus == I2C1)
    {
         LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_I2C1);
    }
    else if(device->bus == I2C2)
    {
        LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_I2C2);
    }
    else if(device->bus == I2C3)
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
    if(LL_I2C_Init(device->bus,&i2c1_cfg) != SUCCESS)
    {
        return ERR_INIT_FAILURE;
    }
    LL_I2C_Enable(device->bus);
    return ERR_OK;
}
static int i2c_wait_not_busy(i2c_bus_t* dev)
{
    WAIT_TIMEOUT(LL_I2C_IsActiveFlag_BUSY(dev->bus),dev->timeout);
    return ERR_OK;
}
static int i2c_send_start(i2c_bus_t* dev)
{
    LL_I2C_GenerateStartCondition(dev->bus);
    WAIT_TIMEOUT(!LL_I2C_IsActiveFlag_SB(dev->bus),dev->timeout);
    return ERR_OK;
}
static int i2c_send_addr(i2c_bus_t* dev,bool read_mode)
{
    LL_I2C_TransmitData8(dev->bus,dev->address | read_mode);
    WAIT_TIMEOUT(!LL_I2C_IsActiveFlag_ADDR(dev->bus),dev->timeout);
    LL_I2C_ClearFlag_ADDR(dev->bus);
    return ERR_OK;
}
int i2c_begin(i2c_bus_t* dev,bool read)
{
    if (!dev) return ERR_INV_ARG;
    RET_ERR(i2c_wait_not_busy(dev));
    RET_ERR(i2c_send_start(dev));
    RET_ERR(i2c_send_addr(dev, read));
    return ERR_OK;
}
int i2c_end(i2c_bus_t* dev)
{
    if(!dev) return ERR_INV_ARG;
    LL_I2C_GenerateStopCondition(dev->bus);
    return ERR_OK;
}
int i2c_write_bytes(i2c_bus_t* dev,const uint8_t* buf,uint32_t len)
{
    if (!(dev && buf))return ERR_INV_ARG;
    for (uint32_t i = 0; i < len; i++)
    {
        LL_I2C_TransmitData8(dev->bus, buf[i]);
        WAIT_TIMEOUT(!LL_I2C_IsActiveFlag_TXE(dev->bus),dev->timeout);
        WAIT_TIMEOUT(!LL_I2C_IsActiveFlag_BTF(dev->bus),dev->timeout);
    }
    return ERR_OK;
}
int i2c_read_bytes(i2c_bus_t* dev,uint8_t* buf,uint32_t len)
{
    if (!(dev && buf))return ERR_INV_ARG;
    for (uint32_t i = 0; i < len; i++)
    {
        WAIT_TIMEOUT(!LL_I2C_IsActiveFlag_RXNE(dev->bus), dev->timeout);
        buf[i] = LL_I2C_ReceiveData8(dev->bus);
    }
    return ERR_OK;
}
int i2c_write(i2c_bus_t* dev,const uint8_t* buf,uint32_t len)
{
    if (!(dev && buf)) return ERR_INV_ARG;
    RET_ERR(i2c_begin(dev,false));
    RET_ERR(i2c_write_bytes(dev,buf,len));
    i2c_end(dev);
    return ERR_OK;
}
int i2c_read(i2c_bus_t* dev,uint8_t* buf,uint32_t len)
{
    if (!(dev && buf)) return ERR_INV_ARG;
    RET_ERR(i2c_begin(dev,true));
    RET_ERR(i2c_read_bytes(dev,buf,len));
    i2c_end(dev);
    return ERR_OK;
}
int i2c_write_read(i2c_bus_t* dev,const uint8_t* tx,uint32_t tx_len,uint8_t* rx,uint32_t rx_len)
{
    RET_ERR(i2c_begin(dev,false));
    RET_ERR(i2c_write_bytes(dev,tx,tx_len));
    RET_ERR(i2c_send_start(dev));
    RET_ERR(i2c_send_addr(dev,true));
    RET_ERR(i2c_read_bytes(dev,rx,rx_len));
    i2c_end(dev);
    return ERR_OK;
}
int i2c_dma_write(i2c_bus_t* dev,dma_transfer_t* dma_info)
{
    if(!(dma_info && dev)) return ERR_INV_ARG;
    RET_ERR(i2c_begin(dev,false));
    if(dma_info->prefix_data)
    {
        RET_ERR(i2c_write_bytes(dev,dma_info->prefix_data,dma_info->prefix_len));
    }
    if(dma_info->on_tx_complete_callback)
    {
        dma_set_callback(dev->tx_dma,dma_info->on_tx_complete_callback,dma_info->ctx);
    }
    LL_I2C_EnableDMAReq_TX(dev->bus);
    uint32_t err = dma_start(dev->tx_dma,dma_info->src,dma_info->dst,dma_info->len);
    if(err != ERR_OK)
    {
        LL_I2C_DisableDMAReq_TX(dev->bus);
        i2c_end(dev);
        return err;
    }
    return ERR_OK;
}
int i2c_dma_finish(i2c_bus_t* dev)
{
    if(!dev) return ERR_INV_ARG;
    LL_I2C_DisableDMAReq_TX(dev->bus);
    return i2c_end(dev);
}
