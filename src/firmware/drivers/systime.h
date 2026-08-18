#pragma once
#include <stdint.h>

#define WAIT_TIMEOUT(cond,timeout)                \
do {                                              \
    systime_t __start = systime_get();            \
    while (cond) {                                \
        if (systime_get() - __start >= (timeout)) \
            return ERR_TIMEOUT;                   \
    }                                             \
} while (0)
typedef uint32_t systime_t;
int systime_init(systime_t ticks_per_second);
systime_t systime_get();
void sleep(systime_t ticks);
