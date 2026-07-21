#include "err.h"
typedef struct
{
    uint32_t magic;
    uint32_t error_code;

    const char* file;
    uint32_t line;
} crash_record_t;

__attribute__((section(".noinit")))
crash_record_t crash_record;

void panic(uint32_t err,const char* file,uint32_t line)
{
    if(err == ERR_OK) return;

    crash_record.magic = 0xDEADBEEF;
    crash_record.error_code = err;
    crash_record.file = file;
    crash_record.line = line;
}
