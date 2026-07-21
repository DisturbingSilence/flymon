#pragma once
#include <stdint.h>
#define LOCHAR 32
#define HICHAR 95
#define FONT_BWIDTH 1
#define FONT_HEIGHT 9
extern const uint8_t FONT[HICHAR-LOCHAR+1][FONT_HEIGHT*FONT_BWIDTH];
