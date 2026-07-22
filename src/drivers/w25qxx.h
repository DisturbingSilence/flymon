#pragma once
#include <stdint.h>

void w25qxx_init();
int w25qxx_read_jedec_id(uint8_t* manufacturer_id,uint8_t* memory_type,uint8_t* capacity);
