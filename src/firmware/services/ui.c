#include <services/ui.h>
#include <services/font.h>
#include <drivers/err.h>

#include <stdio.h>
#include <string.h>

int ui_init(ui_context_t* ctx,ssd1306_device_t* screen,void* app_ctx,void(*on_render)(void*),void(*on_btn_press)(void*,button_t))
{
    if(!(ctx && screen && app_ctx && on_render && on_btn_press)) return ERR_INV_ARG;
    ctx->screen = screen;
    ctx->app_context = app_ctx;
    ctx->state = UI_STATE_MAIN;
    ctx->on_render = on_render;
    ctx->on_button_press = on_btn_press;
    return ERR_OK;
}
void ui_set_state(ui_context_t* ctx,ui_state_t new_state)
{
    if(!(ctx && new_state < UI_STATE_COUNT)) return;
    if(new_state == ctx->state) return;
    ctx->state = new_state;
}
void ui_render(ui_context_t* ctx)
{
    if(!ctx) return;
    if(ctx->on_render) ctx->on_render(ctx->app_context);
    ssd1306_update(ctx->screen);
}
void ui_handle_button(ui_context_t* ctx,button_t btn)
{
    if(!ctx || btn == BUTTON_NONE) return;
    if(ctx->on_button_press)
        ctx->on_button_press(ctx->app_context,btn);
}
