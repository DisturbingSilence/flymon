#pragma once
#include <drivers/ssd1306.h>
#include <services/buttons.h>

typedef enum
{
    UI_STATE_MAIN = 0,
    UI_STATE_CALIBRATE,
    UI_STATE_TRANSMIT,
    UI_STATE_LIVE_VIEW,

    UI_STATE_COUNT
} ui_state_t;
typedef struct
{
    void(*on_enter)(void* app_context);
    void(*on_render)(void* app_context);
} ui_page_t;
typedef struct
{
    ssd1306_device_t* screen;
    void* app_context;
    void(*on_button_pressed)(void* ctx,button_t btn);
    ui_state_t state; // index into pages
    ui_page_t pages[UI_STATE_COUNT];
    uint8_t is_dirty;
} ui_context_t;
int ui_init(ui_context_t* ctx,ssd1306_device_t* screen,void* app_ctx,void(*btn_callback)(void*,button_t));
void ui_render(ui_context_t* ctx);
void ui_set_state(ui_context_t* ctx,ui_state_t new_state);
void ui_add_page(ui_context_t* ctx,ui_state_t state,const ui_page_t* page);
void ui_handle_button(ui_context_t* ctx,button_t btn);
