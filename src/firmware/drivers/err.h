#pragma once
#include <stdint.h>
enum
{
    ERR_OK = 0,
    ERR_INIT_FAILURE,
    ERR_TIMEOUT,
    ERR_BUSY,
    ERR_INV_ARG,
    ERR_INV_DEVICE,
    ERR_IO,

    ERR_SCHEDULER_TOO_MANY_TASKS,

    ERR_OVERFLOW,
    ERR_UNDERFLOW,
    ERR_EMPTY,
    ERR_INCOMPLETE,
    ERR_INV_MAGIC
};
#define PANIC(err) do { \
    int _err = (err);   \
    if(_err != ERR_OK)  \
        fatal_error(_err); \
} while(0)
#define RET_ERR(err) if(err != ERR_OK) return err;
void panic(uint32_t err,const char* file,uint32_t line);
void fatal_error(uint32_t errcode);
