#pragma once
#include <drivers/peripherals/spi.h>

#include <stdint.h>

#include "stm32f4xx_ll_gpio.h"

#define PAGE_SIZE 256
#define SECTOR_SIZE 16 // in pages
#define BLOCK32_SIZE 8 // in sectors
#define BLOCK64_SIZE 16

#define W25QXX_ST1_BUSY (1 << 0)
#define W25QXX_ST1_WEL (1 << 1)
#define W25QXX_ST1_BP0 (1 << 2)
#define W25QXX_ST1_BP1 (1 << 3)
#define W25QXX_ST1_BP2 (1 << 4)
#define W25QXX_ST1_TB (1 << 5)
#define W25QXX_ST1_SEC (1 << 6)
#define W25QXX_ST1_SRP (1 << 7)

typedef struct
{
    GPIO_TypeDef* cs_port;
    uint32_t cs_pinmask;
    spi_bus_t* spi_bus;

    uint8_t manufacturer_id;
    uint8_t memory_type;
    uint8_t capacity;

    uint32_t pages;
    uint32_t sectors;
    uint32_t blocks;
} w25qxx_flash_t;

int w25qxx_init(GPIO_TypeDef* cs_port,uint32_t pinmask,spi_bus_t* spi_bus,w25qxx_flash_t* flash);
int w25qxx_read_jedec_id(w25qxx_flash_t* flash,uint8_t* manufacturer_id,uint8_t* memory_type,uint8_t* capacity);
int w25qxx_write_enable(w25qxx_flash_t* flash);
int w25qxx_write_disable(w25qxx_flash_t* flash);
int w25qxx_read_streg1(w25qxx_flash_t* flash,uint8_t* s1);
int w25qxx_is_busy(w25qxx_flash_t* flash);
int w25qxx_is_wel(w25qxx_flash_t* flash);
int w25qxx_erase_sector(w25qxx_flash_t* flash,uint16_t sector);
int w25qxx_write(w25qxx_flash_t* flash,uint32_t page,uint32_t offs,const uint8_t* data,uint32_t len);
int w25qxx_read(w25qxx_flash_t* flash,uint32_t page,uint32_t offs,uint8_t* data,uint32_t len);
