#pragma once
#include <stdint.h>

typedef uint32_t systime_t;
int systime_init();
systime_t systime_get();
