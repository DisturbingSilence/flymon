#pragma once
#include <stdint.h>
typedef struct
{
    volatile uint16_t head;
    volatile uint16_t tail;
    volatile uint16_t size;
    uint16_t capacity;
    uint8_t* storage;
} ringbuffer_t;

#define IS_RINGBUFFER_EMPTY(rb) (rb.size == 0)
#define RINGBUFFER_FREE(rb) (rb.capacity - rb.size)

int ringbuffer_init(ringbuffer_t* rb,void* storage,uint16_t capacity);
int ringbuffer_write(ringbuffer_t* rb,uint8_t data);
int ringbuffer_read(ringbuffer_t* rb,uint8_t* data);
int ringbuffer_peek_at(ringbuffer_t* rb,uint16_t offset,uint8_t* data);
