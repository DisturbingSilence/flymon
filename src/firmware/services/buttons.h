#pragma once
#include <stdint.h>

#define DEBOUNCE_STABLE_COUNT 10
#define BTN_SAMPLING_FREQUENCY 100

typedef enum
{
    BUTTON_NONE = 0,
    BUTTON_LEFT,
    BUTTON_RIGHT,
    BUTTON_SELECT
} button_t;
typedef void(*button_callback_t)(void* ctx,button_t btn);
int buttons_init(button_callback_t clbck,void* ctx);
