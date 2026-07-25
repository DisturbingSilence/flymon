#include "spi.h"
#include "stm32f4xx_ll_gpio.h"
#include "stm32f4xx_ll_bus.h"

#include "systime.h"
#include "err.h"

int spi_init(SPI_TypeDef* spix)
{
    if(!spix) return ERR_INV_ARG;

    if(spix == SPI1)
        LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_SPI1);
    else if(spix == SPI2)
        LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_SPI2);
    else if(spix == SPI3)
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
    if(LL_SPI_Init(spix,&spi_cfg) != SUCCESS)
    {
        return ERR_INIT_FAILURE;
    }
    LL_SPI_Enable(spix);
    return ERR_OK;
}

int spi_transfer(SPI_TypeDef* spix,const uint8_t* tx,uint8_t* rx,uint32_t len)
{
    if (!spix) return ERR_INV_ARG;
    if (len == 0) return ERR_OK;
    while(len--)
    {
         WAIT_TIMEOUT(!LL_SPI_IsActiveFlag_TXE(spix),SPI_TIMEOUT);
         LL_SPI_TransmitData8(spix,tx ? *tx++ : 0xFF);
         WAIT_TIMEOUT(!LL_SPI_IsActiveFlag_RXNE(spix),SPI_TIMEOUT);
         uint8_t rx_byte = LL_SPI_ReceiveData8(spix);
         if (rx) *rx++ = rx_byte;
    }
    WAIT_TIMEOUT(!LL_SPI_IsActiveFlag_TXE(spix),SPI_TIMEOUT);
    WAIT_TIMEOUT(LL_SPI_IsActiveFlag_BSY(spix),SPI_TIMEOUT);
    return ERR_OK;
}
int spi_read(SPI_TypeDef* spix,uint8_t* rx,uint32_t len)
{
    return spi_transfer(spix,0,rx,len);
}
int spi_write(SPI_TypeDef* spix,const uint8_t* tx,uint32_t len)
{
    return spi_transfer(spix,tx,0,len);
}
