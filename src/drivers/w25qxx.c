#include "w25qxx.h"
#include "stm32f4xx_ll_spi.h"
#include "spi.h"
#include "err.h"
void w25qxx_init()
{
    spi_init(SPI2);
    //void spi_write(SPI_TypeDef* spix,uint8_t data);
}
int w25qxx_read_jedec_id(uint8_t* manufacturer_id,uint8_t* memory_type,uint8_t* capacity)
{
    uint8_t tx_buf[4] = {0x9F,0x00,0x00,0x00};
    uint8_t rx_buf[4] = {0};
    spi_cs_select();
    int status = spi_transfer(SPI2,tx_buf,rx_buf,4);
    spi_cs_deselect();
    if (status == ERR_OK)
    {
        if (manufacturer_id) *manufacturer_id = rx_buf[1];
        if (memory_type)     *memory_type     = rx_buf[2];
        if (capacity)        *capacity        = rx_buf[3];
    }
    return status;
}
