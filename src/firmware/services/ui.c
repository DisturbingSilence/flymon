#include <services/ui.h>
#include <services/font.h>
#include <drivers/err.h>

#include <stdio.h>
#include <string.h>

int ui_init(ui_context_t* ctx,ssd1306_device_t* screen,void* app_ctx,void(*btn_callback)(void*,button_t))
{
    if(!(ctx && screen && app_ctx && btn_callback)) return ERR_INV_ARG;
    ctx->screen = screen;
    ctx->app_context = app_ctx;
    ctx->state = UI_STATE_MAIN;
    ctx->is_dirty = true;
    ctx->on_button_pressed = btn_callback;
    memset(ctx->pages,0,sizeof(ctx->pages));

    return ERR_OK;
}
void ui_add_page(ui_context_t* ctx,ui_state_t state,const ui_page_t* page)
{
    if(!(ctx && page && state < UI_STATE_COUNT)) return;
    ctx->pages[state] = *page;
}
void ui_set_state(ui_context_t* ctx,ui_state_t new_state)
{
    if(!(ctx && new_state < UI_STATE_COUNT)) return;
    if(new_state == ctx->state) return;
    if(ctx->pages[new_state].on_enter)
        ctx->pages[new_state].on_enter(ctx->app_context);
    ctx->state = new_state;
    ctx->is_dirty = true;
}
void ui_render(ui_context_t* ctx)
{
    if(!ctx) return;
    if(!ctx->is_dirty) return;
    if(ctx->pages[ctx->state].on_render)
        ctx->pages[ctx->state].on_render(ctx->app_context);
    ssd1306_update(ctx->screen);
    ctx->is_dirty = false;
}
