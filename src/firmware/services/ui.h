#pragma once
#include <drivers/devices/ssd1306.h>
#include <services/buttons.h>

typedef enum
{
    UI_STATE_MAIN = 0,
    UI_STATE_CALIBRATE,
    UI_STATE_LIVE_VIEW,

    UI_STATE_COUNT
} ui_state_t;
typedef struct
{
    ssd1306_device_t* screen;
    void* app_context;
    ui_state_t state;
    void(*on_render)(void* app_context);
    void(*on_button_press)(void* app_context,button_t btn);
} ui_context_t;
int ui_init(ui_context_t* ctx,ssd1306_device_t* screen,void* app_ctx,void(*on_render)(void*),void(*on_btn_press)(void*,button_t));
void ui_render(ui_context_t* ctx);
void ui_handle_button(ui_context_t* ctx,button_t btn);
void ui_set_state(ui_context_t* ctx,ui_state_t new_state);
