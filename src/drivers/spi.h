#pragma once
#include <stdint.h>
#include "stm32f4xx_ll_spi.h"

typedef struct
{
    SPI_TypeDef* bus;
    uint32_t timeout;
} spi_bus_t;

int spi_init(spi_bus_t* bus);
int spi_transfer(spi_bus_t* bus,const uint8_t* tx,uint8_t* rx,uint32_t len);
int spi_read(spi_bus_t* bus,uint8_t* rx,uint32_t len);
int spi_write(spi_bus_t* busx,const uint8_t* tx,uint32_t len);
