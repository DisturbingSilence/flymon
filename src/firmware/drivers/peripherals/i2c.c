#include <drivers/peripherals/i2c.h>
#include <drivers/err.h>
#include <drivers/systime.h>

#include "stm32f4xx_ll_bus.h"

static inline void __i2c_wait_stop(i2c_bus_t* bus)
{
    while (LL_I2C_IsActiveFlag_BUSY(bus->instance));
}
static void __i2c_abort(i2c_bus_t* bus)
{
    I2C_TypeDef* i2c = bus->instance;
    LL_I2C_DisableDMAReq_TX(i2c);
    LL_I2C_ClearFlag_AF(i2c);
    LL_I2C_ClearFlag_BERR(i2c);
    LL_I2C_ClearFlag_ARLO(i2c);
    LL_I2C_ClearFlag_OVR(i2c);
    LL_I2C_GenerateStopCondition(i2c);
    while(LL_I2C_IsActiveFlag_BUSY(i2c));
}
int i2c_init(i2c_bus_t* bus,uint32_t clock_speed)
{
    if(!bus) return ERR_INV_ARG;
    if(bus->instance == I2C1)
        LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_I2C1);
    else if(bus->instance == I2C2)
        LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_I2C2);
    else if(bus->instance == I2C3)
        LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_I2C3);
    else
        return ERR_INV_ARG;
    LL_I2C_InitTypeDef cfg =
    {
        .PeripheralMode = LL_I2C_MODE_I2C,
        .ClockSpeed = clock_speed,
        .DutyCycle = LL_I2C_DUTYCYCLE_2,
        .OwnAddress1 = 0,
        .TypeAcknowledge = LL_I2C_ACK,
        .OwnAddrSize = LL_I2C_OWNADDRESS1_7BIT
    };
    if(LL_I2C_Init(bus->instance, &cfg) != SUCCESS) return ERR_INIT_FAILURE;
    LL_I2C_Enable(bus->instance);
    return ERR_OK;
}
static int i2c_wait_not_busy(i2c_bus_t* bus)
{
    systime_t start = systime_get();
    while(LL_I2C_IsActiveFlag_BUSY(bus->instance))
    {
        if(systime_get() - start >= bus->timeout) return ERR_TIMEOUT;
    }
    return ERR_OK;
}
static int i2c_send_start(i2c_bus_t* bus)
{
    LL_I2C_GenerateStartCondition(bus->instance);
    systime_t start = systime_get();
    while(!LL_I2C_IsActiveFlag_SB(bus->instance))
    {
        if(systime_get() - start >= bus->timeout) return ERR_TIMEOUT;
    }
    return ERR_OK;
}
static int i2c_send_addr(i2c_device_t* dev, bool read)
{
    uint8_t addr = dev->address | (read ? 1 : 0);
    LL_I2C_TransmitData8(dev->bus->instance, addr);
    systime_t start = systime_get();
    while(!LL_I2C_IsActiveFlag_ADDR(dev->bus->instance))
    {
        if(LL_I2C_IsActiveFlag_AF(dev->bus->instance))
        {
            LL_I2C_ClearFlag_AF(dev->bus->instance);
            return ERR_INV_DEVICE;
        }
        if(systime_get() - start >= dev->bus->timeout)
        {
            __i2c_abort(dev->bus);
            return ERR_TIMEOUT;
        }
    }
    LL_I2C_ClearFlag_ADDR(dev->bus->instance);
    return ERR_OK;
}
int i2c_begin(i2c_device_t* dev, bool read)
{
    if(!dev) return ERR_INV_ARG;
    RET_ERR(i2c_wait_not_busy(dev->bus));
    RET_ERR(i2c_send_start(dev->bus));
    int ret = i2c_send_addr(dev, read);
    if(ret != ERR_OK)
    {
        LL_I2C_GenerateStopCondition(dev->bus->instance);
        __i2c_wait_stop(dev->bus);
        return ret;
    }
    return ERR_OK;
}
int i2c_end(i2c_bus_t* bus)
{
    if(!bus) return ERR_INV_ARG;
    LL_I2C_GenerateStopCondition(bus->instance);
    __i2c_wait_stop(bus);
    return ERR_OK;
}
int i2c_write_bytes(i2c_device_t* dev,const uint8_t* buf,uint32_t len)
{
    if(!dev || !buf) return ERR_INV_ARG;
    for(uint32_t i = 0; i < len; i++)
    {
        LL_I2C_TransmitData8(dev->bus->instance, buf[i]);
        WAIT_TIMEOUT(!LL_I2C_IsActiveFlag_BTF(dev->bus->instance),dev->bus->timeout);
    }
    return ERR_OK;
}
int i2c_read_bytes(i2c_device_t* dev, uint8_t* buf, uint32_t len)
{
    if (!(dev && buf)) return ERR_INV_ARG;
    I2C_TypeDef* i2c = dev->bus->instance;
    for(uint32_t i = 0; i < len; i++)
    {
        if(i == len - 1)
        {
            LL_I2C_AcknowledgeNextData(i2c,LL_I2C_NACK);
        }
        else
        {
            LL_I2C_AcknowledgeNextData(i2c,LL_I2C_ACK);
        }
        systime_t start = systime_get();
        while(!LL_I2C_IsActiveFlag_RXNE(i2c))
        {
            if(systime_get() - start >= dev->bus->timeout)
            {
                return ERR_TIMEOUT;
            }
            if(LL_I2C_IsActiveFlag_BERR(i2c) || LL_I2C_IsActiveFlag_ARLO(i2c) || LL_I2C_IsActiveFlag_OVR(i2c))
            {
                return ERR_IO;
            }
        }
        buf[i] = LL_I2C_ReceiveData8(i2c);
    }
    LL_I2C_AcknowledgeNextData(i2c, LL_I2C_ACK);
    return ERR_OK;
}
int i2c_write(i2c_device_t* dev,const uint8_t* buf,uint32_t len)
{
    if (!(dev && buf)) return ERR_INV_ARG;
    int ret = i2c_begin(dev,false);
    if(ret != ERR_OK)
    {
        __i2c_abort(dev->bus);
        return ret;
    }
    ret = i2c_write_bytes(dev,buf,len);
    if(ret != ERR_OK)
    {
        __i2c_abort(dev->bus);
        return ret;
    }
    i2c_end(dev->bus);
    return ERR_OK;
}
int i2c_probe(i2c_bus_t* bus, uint8_t address)
{
    i2c_device_t dev =
    {
        .bus = bus,
        .address = address
    };
    RET_ERR(i2c_wait_not_busy(bus));
    RET_ERR(i2c_send_start(bus));
    int ret = i2c_send_addr(&dev, false);

    if(ret == ERR_OK)
    {
        LL_I2C_GenerateStopCondition(bus->instance);
        while(LL_I2C_IsActiveFlag_BUSY(bus->instance));
    }
    return ret;
}
int i2c_read(i2c_device_t* dev,uint8_t* buf,uint32_t len)
{
    if (!(dev && buf)) return ERR_INV_ARG;
    int ret = i2c_begin(dev,true);
    if(ret != ERR_OK)
    {
        __i2c_abort(dev->bus);
        return ret;
    }
    ret = i2c_read_bytes(dev,buf,len);
    if(ret != ERR_OK)
    {
        __i2c_abort(dev->bus);
        return ret;
    }
    i2c_end(dev->bus);
    return ERR_OK;
}
int i2c_write_read(i2c_device_t* dev,const uint8_t* tx,uint32_t tx_len,uint8_t* rx,uint32_t rx_len)
{
    int ret;
    ret = i2c_begin(dev,false);
    if(ret != ERR_OK) goto fail;
    ret = i2c_write_bytes(dev,tx,tx_len);
    if(ret != ERR_OK) goto fail;
    ret = i2c_send_start(dev->bus);
    if(ret != ERR_OK) goto fail;
    ret = i2c_send_addr(dev,true);
    if(ret != ERR_OK) goto fail;
    ret = i2c_read_bytes(dev,rx,rx_len);
    if(ret != ERR_OK) goto fail;
    i2c_end(dev->bus);
    return ERR_OK;
fail:
    __i2c_abort(dev->bus);
    return ret;
}
int i2c_dma_write(i2c_device_t* dev,dma_transfer_t* dma_info)
{
    if(!(dev && dma_info)) return ERR_INV_ARG;
    int ret = i2c_begin(dev,false);
    if(ret != ERR_OK) goto fail;
    if(dma_info->prefix_data)
    {
        ret = i2c_write_bytes(dev,dma_info->prefix_data, dma_info->prefix_len);
        if(ret != ERR_OK) goto fail;
    }
    if(dma_info->on_tx_complete_callback)
    {
        dma_set_callback(dev->bus->tx_dma,dma_info->on_tx_complete_callback,dma_info->ctx);
    }
    LL_I2C_EnableDMAReq_TX(dev->bus->instance);
    ret = dma_start(dev->bus->tx_dma,dma_info->src, dma_info->dst,dma_info->len);
    if(ret != ERR_OK)goto fail;
    return ERR_OK;
fail:
    __i2c_abort(dev->bus);
    return ret;
}
int i2c_dma_finish(i2c_device_t* dev)
{
    if(!dev)return ERR_INV_ARG;
    LL_I2C_DisableDMAReq_TX(dev->bus->instance);
    while(!LL_I2C_IsActiveFlag_BTF(dev->bus->instance));
    LL_I2C_GenerateStopCondition(dev->bus->instance);
    while(LL_I2C_IsActiveFlag_BUSY(dev->bus->instance));
    return ERR_OK;
}
