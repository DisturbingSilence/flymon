#pragma once
#include <drivers/peripherals/dma.h>
#include <drivers/ringbuffer.h>

#include <stdint.h>

#include "stm32f4xx_ll_usart.h"

typedef struct
{
    USART_TypeDef* instance;
    ringbuffer_t rx_rb;
    ringbuffer_t tx_rb;

    dma_channel_t* tx_dma;
    dma_channel_t* rx_dma;
    uint8_t is_dma_busy;
    uint16_t tx_dma_len;
    uint16_t rx_dma_pos;
} usart_bus_t;

typedef struct
{
    USART_TypeDef* instance;
    uint32_t baudrate;
    uint32_t transfer_direction;
    uint8_t enable_clock;
    void* rx_buffer;
    uint32_t rx_capacity;
    void* tx_buffer;
    uint32_t tx_capacity;
    dma_channel_t* rx_dma;
    dma_channel_t* tx_dma;
} usart_config_t;
int usart_init(usart_bus_t* bus,const usart_config_t* cfg);
int usart_write(usart_bus_t* bus,const uint8_t* data,uint16_t size);
int usart_read(usart_bus_t* bus,uint8_t* data,uint16_t size);
uint16_t usart_available(usart_bus_t* bus);
