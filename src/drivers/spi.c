#include "spi.h"
#include "stm32f4xx_ll_gpio.h"
#include "stm32f4xx_ll_bus.h"

#include "systime.h"
#include "err.h"

int spi_init(spi_bus_t* bus)
{
    if(!bus) return ERR_INV_ARG;

    if(bus->bus == SPI1)
        LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_SPI1);
    else if(bus->bus == SPI2)
        LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_SPI2);
    else if(bus->bus == SPI3)
        LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_SPI3);
    else
        return ERR_INV_ARG;

    LL_SPI_InitTypeDef spi_cfg =
    {
        .TransferDirection = LL_SPI_FULL_DUPLEX,
        .Mode = LL_SPI_MODE_MASTER,
        .DataWidth = LL_SPI_DATAWIDTH_8BIT,
        .ClockPolarity = LL_SPI_POLARITY_LOW,
        .ClockPhase = LL_SPI_PHASE_1EDGE,
        .NSS = LL_SPI_NSS_SOFT,
        .BaudRate = LL_SPI_BAUDRATEPRESCALER_DIV2,
        .BitOrder = LL_SPI_MSB_FIRST,
        .CRCCalculation = LL_SPI_CRCCALCULATION_DISABLE,
        .CRCPoly = 7
    };
    if(LL_SPI_Init(bus->bus,&spi_cfg) != SUCCESS)
    {
        return ERR_INIT_FAILURE;
    }
    LL_SPI_Enable(bus->bus);
    return ERR_OK;
}

int spi_transfer(spi_bus_t* bus,const uint8_t* tx,uint8_t* rx,uint32_t len)
{
    if (!bus) return ERR_INV_ARG;
    if (len == 0) return ERR_OK;
    if(!(rx && tx)) return ERR_INV_ARG;
    while(len--)
    {
         WAIT_TIMEOUT(!LL_SPI_IsActiveFlag_TXE(bus->bus),bus->timeout);
         LL_SPI_TransmitData8(bus->bus,tx ? *tx++ : 0xFF);
         WAIT_TIMEOUT(!LL_SPI_IsActiveFlag_RXNE(bus->bus),bus->timeout);
         uint8_t rx_byte = LL_SPI_ReceiveData8(bus->bus);
         if (rx) *rx++ = rx_byte;
    }
    WAIT_TIMEOUT(!LL_SPI_IsActiveFlag_TXE(bus->bus),bus->timeout);
    WAIT_TIMEOUT(LL_SPI_IsActiveFlag_BSY(bus->bus),bus->timeout);
    return ERR_OK;
}
int spi_read(spi_bus_t* bus,uint8_t* rx,uint32_t len)
{
    return spi_transfer(bus,0,rx,len);
}
int spi_write(spi_bus_t* bus,const uint8_t* tx,uint32_t len)
{
    return spi_transfer(bus,tx,0,len);
}
