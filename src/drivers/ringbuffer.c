#include <drivers/ringbuffer.h>
#include <drivers/err.h>

int ringbuffer_init(ringbuffer_t* rb,void* storage,uint16_t capacity)
{
    if(!(rb && storage)) return ERR_INV_ARG;
    if(capacity == 0) return ERR_INV_ARG;
    rb->head = 0;
    rb->size = 0;
    rb->tail = 0;
    rb->capacity = capacity;
    rb->storage = storage;

    return ERR_OK;
}
int ringbuffer_write(ringbuffer_t* rb,uint8_t data)
{
    if(!rb) return ERR_INV_ARG;
    if (rb->size >= rb->capacity) return ERR_OVERFLOW;

    rb->storage[rb->tail] = data;
    rb->tail++;
    if (rb->tail >= rb->capacity) rb->tail = 0;
    rb->size++;

    return ERR_OK;
}
int ringbuffer_read(ringbuffer_t* rb,uint8_t* data)
{
    if(!(rb && data)) return ERR_INV_ARG;
    if (rb->size == 0) return ERR_EMPTY;

    *data = rb->storage[rb->head];
    rb->head++;
    if (rb->head >= rb->capacity) rb->head = 0;
    rb->size--;

    return ERR_OK;
}
int ringbuffer_peek_at(ringbuffer_t* rb,uint16_t offset,uint8_t* data)
{
    if(!(rb && data)) return ERR_INV_ARG;
    if (rb->size == 0) return ERR_EMPTY;
    if(rb->size <= offset) return ERR_INV_ARG;

    *data = rb->storage[(rb->head + offset) % rb->capacity];

    return ERR_OK;
}
