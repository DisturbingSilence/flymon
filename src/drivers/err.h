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
    ERR_IO
};
#define PANIC(err) if(err != ERR_OK) fatal_error(err);
#define RET_ERR(err) if(err != ERR_OK) return err;
void panic(uint32_t err,const char* file,uint32_t line);
void fatal_error(uint32_t errcode);
