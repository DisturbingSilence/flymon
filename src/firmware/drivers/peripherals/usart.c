#include <drivers/peripherals/usart.h>
#include <drivers/ringbuffer.h>
#include <drivers/systime.h>
#include <drivers/err.h>

#include "stm32f4xx_ll_bus.h"
static void usart_tx_dma_complete(void *ctx);
static int usart_rx_dma_start(usart_bus_t* bus);
int usart_init(usart_bus_t* bus,const usart_config_t* cfg)
{
    if(!(bus && cfg)) return ERR_INV_ARG;
    bus->instance = cfg->instance;
    if(bus->instance == USART1)
        LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_USART1);
    else if(bus->instance == USART2)
        LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_USART2);
    else if(bus->instance == USART6)
        LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_USART6);
    else
        return ERR_INV_ARG;
    bus->tx_dma = cfg->tx_dma;
    bus->rx_dma = cfg->rx_dma;
    LL_USART_InitTypeDef usart_cfg =
    {
        .BaudRate = cfg->baudrate,
        .DataWidth = LL_USART_DATAWIDTH_8B,
        .StopBits = LL_USART_STOPBITS_1,
        .Parity = LL_USART_PARITY_NONE,
        .TransferDirection = cfg->transfer_direction,
        .HardwareFlowControl = LL_USART_HWCONTROL_NONE,
        .OverSampling = LL_USART_OVERSAMPLING_16
    };
    int err = LL_USART_Init(bus->instance,&usart_cfg);
    if(err != SUCCESS) return ERR_INIT_FAILURE;
    LL_USART_ClockInitTypeDef clock_cfg =
    {
        .ClockOutput = cfg->enable_clock ? LL_USART_CLOCK_ENABLE : LL_USART_CLOCK_DISABLE,
        .ClockPolarity = LL_USART_POLARITY_LOW,
        .ClockPhase = LL_USART_PHASE_1EDGE,
        .LastBitClockPulse = LL_USART_LASTCLKPULSE_NO_OUTPUT
    };
    err = LL_USART_ClockInit(bus->instance,&clock_cfg);
    if(err != SUCCESS) return ERR_INIT_FAILURE;

    err = ringbuffer_init(&bus->tx_rb,cfg->tx_buffer,cfg->tx_capacity);
    if(err != ERR_OK) return ERR_INIT_FAILURE;
    err = ringbuffer_init(&bus->rx_rb,cfg->rx_buffer,cfg->rx_capacity);
    if(err != ERR_OK) return ERR_INIT_FAILURE;

    if(cfg->transfer_direction & LL_USART_DIRECTION_TX)
    {
        if(!bus->tx_dma) return ERR_INV_ARG;
        if (LL_DMA_GetMode(bus->tx_dma->instance,bus->tx_dma->stream) != LL_DMA_MODE_NORMAL) return ERR_INV_ARG;
        LL_USART_EnableDMAReq_TX(bus->instance);
        dma_set_callback(bus->tx_dma,usart_tx_dma_complete,bus);
    }

    if(cfg->transfer_direction & LL_USART_DIRECTION_RX)
    {
        if(!bus->rx_dma) return ERR_INV_ARG;
        if (LL_DMA_GetMode(bus->rx_dma->instance,bus->rx_dma->stream) != LL_DMA_MODE_CIRCULAR) return ERR_INV_ARG;
        LL_USART_EnableDMAReq_RX(bus->instance);
    }

    LL_USART_Enable(bus->instance);
    WAIT_TIMEOUT(!LL_USART_IsEnabled(bus->instance),100);

    if (cfg->transfer_direction & LL_USART_DIRECTION_RX)
    {
        err = usart_rx_dma_start(bus);
        if (err != ERR_OK) return err;
    }
    return ERR_OK;
}
static int usart_rx_dma_start(usart_bus_t* bus)
{
    if (!(bus && bus->rx_dma)) return ERR_INV_ARG;
    bus->rx_dma_pos = 0;
    return dma_start(
        bus->rx_dma,
        (uint32_t)bus->rx_rb.storage,
        (uint32_t)&bus->instance->DR,
        bus->rx_rb.capacity
    );
}
static void usart_rx_dma_update(usart_bus_t *bus)
{
    if (!(bus && bus->rx_dma)) return;

    uint16_t capacity = bus->rx_rb.capacity;
    if (capacity == 0) return;
    uint16_t remaining = (uint16_t)dma_get_remaining(bus->rx_dma);

    uint16_t dma_pos = capacity - remaining;
    if (dma_pos >= capacity) dma_pos = 0;

    uint16_t old_pos = bus->rx_dma_pos;
    if (dma_pos == old_pos) return;
    if (dma_pos > old_pos)
    {
        uint16_t received = dma_pos - old_pos;
        uint16_t free = bus->rx_rb.capacity - bus->rx_rb.size;

        if (received > free)
        {
            uint16_t overflow = received - free;
            bus->rx_rb.head = (uint16_t)((bus->rx_rb.head + overflow) % bus->rx_rb.capacity);
            bus->rx_rb.size = bus->rx_rb.capacity;
        }
        else
        {
            bus->rx_rb.size += received;
        }
    }
    else
    {
        uint16_t received = (bus->rx_rb.capacity - old_pos) + dma_pos;
        if (received >= bus->rx_rb.capacity)
        {
            bus->rx_rb.head = dma_pos;
            bus->rx_rb.size = 0;
        }
        else
        {
            uint16_t free = bus->rx_rb.capacity - bus->rx_rb.size;
            if (received > free)
            {
                uint16_t overflow = received - free;
                bus->rx_rb.head = (uint16_t)((bus->rx_rb.head + overflow)% bus->rx_rb.capacity);
                bus->rx_rb.size = free;
            }
            bus->rx_rb.size += received;
        }
    }
    bus->rx_dma_pos = dma_pos;
}
static void usart_tx_start(usart_bus_t* bus)
{
    if (!bus || !bus->tx_dma) return;
    if (bus->is_dma_busy) return;
    if (IS_RINGBUFFER_EMPTY(bus->tx_rb)) return;

    uint16_t head = bus->tx_rb.head;
    uint16_t tail = bus->tx_rb.tail;
    uint16_t len = (tail > head ? tail : bus->tx_rb.capacity) - head;

    bus->tx_dma_len = len;
    bus->is_dma_busy = 1;
    int err = dma_start(
        bus->tx_dma,
        (uint32_t)&bus->tx_rb.storage[head],
        (uint32_t)&bus->instance->DR,
        len
    );

    if (err != ERR_OK)
    {
        bus->tx_dma_len = 0;
        bus->is_dma_busy = 0;

        PANIC(err);
    }
}
static void usart_tx_dma_complete(void *ctx)
{
    if(!ctx) return;

    usart_bus_t *bus = (usart_bus_t *)ctx;

    uint16_t len = bus->tx_dma_len;

    if (len > 0)
    {
        bus->tx_rb.head += len;
        if (bus->tx_rb.head >= bus->tx_rb.capacity) bus->tx_rb.head -= bus->tx_rb.capacity;
        bus->tx_rb.size -= len;
    }
    bus->tx_dma_len = 0;
    bus->is_dma_busy = 0;
    // start second transfer in case rb wrapped
    usart_tx_start(bus);
}

int usart_write(usart_bus_t* bus,const uint8_t* data,uint16_t size)
{
    if (!(bus && data)) return ERR_INV_ARG;
    if (size == 0) return ERR_OK;
    if (size > RINGBUFFER_FREE(bus->tx_rb)) return ERR_OVERFLOW;

    for (uint32_t i = 0; i < size; i++)
    {
        int err = ringbuffer_write(&bus->tx_rb,data[i]);
        if (err != ERR_OK) return err;
    }
    usart_tx_start(bus);

    return ERR_OK;
}

int usart_read(usart_bus_t* bus,uint8_t* data,uint16_t size)
{
    if (!(bus && data)) return ERR_INV_ARG;
    if (size == 0) return ERR_OK;

    usart_rx_dma_update(bus);
    // maybe return ERR_UNDEFLOW here?
    if (size > bus->rx_rb.size) return ERR_EMPTY;

    for (uint32_t i = 0; i < size; i++)
    {
        int err = ringbuffer_read(&bus->rx_rb,&data[i]);
        if (err != ERR_OK) return err;
    }
    return ERR_OK;
}
uint16_t usart_available(usart_bus_t* bus)
{
    if (!bus) return 0;

    usart_rx_dma_update(bus);
    return bus->rx_rb.size;
}
