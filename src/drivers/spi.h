#pragma once

#include <stdint.h>
#include "stm32f4xx_ll_spi.h"

#define SPI_TIMEOUT 5 // in systime_ticks

int spi_init(SPI_TypeDef* spix);
int spi_transfer(SPI_TypeDef* spix,const uint8_t* tx,uint8_t* rx,uint32_t len);
int spi_read(SPI_TypeDef* spix,uint8_t* rx,uint32_t len);
int spi_write(SPI_TypeDef* spix,const uint8_t* tx,uint32_t len);

void spi_cs_select();
void spi_cs_deselect();
