#include <bsp/STM32F411CEU6.h>

#include <app/config.h>

#include <drivers/ssd1306.h>
#include <drivers/i2c.h>
#include <drivers/w25qxx.h>
#include <drivers/mpu60x0.h>
#include <drivers/err.h>
#include <drivers/usart.h>
#include <drivers/ble.h>

#include <services/scheduler.h>
#include <services/buttons.h>
#include <services/ui.h>
#include <services/font.h>
#include <stdio.h>

#include <string.h>

#include "stm32f4xx_ll_gpio.h"

#define USART1_RX_BUFFER_SIZE 1024
#define USART1_TX_BUFFER_SIZE 1024
typedef struct
{
    // sys
    i2c_bus_t i2c1;
    spi_bus_t spi2;
    usart_bus_t usart1;

    dma_channel_t dma1stream6; // i2c1 tx
    dma_channel_t dma2stream2; // usart1 rx
    dma_channel_t dma2stream7; // usart1 tx
    w25qxx_flash_t flash;
    ssd1306_device_t oled;
    ble_device_t ble;
    mpu60x0_t mpu;
    ui_context_t ui_ctx;
    // stats
    int16_t gyro_x;
    int16_t gyro_y;
    int16_t gyro_z;
    int counter;

    uint8_t usart1_rxrb[USART1_RX_BUFFER_SIZE];
    uint8_t usart1_txrb[USART1_TX_BUFFER_SIZE];
    uint8_t bbuf[50];
} global_context_t;

static global_context_t g_ctx = {};

static void ble_rx_task(void* ctx)
{
    global_context_t* gctx = (global_context_t*)ctx;
    uint16_t size = 49;
    ble_read(&gctx->ble,gctx->bbuf,&size);
}
void oled_task(void* ctx)
{
    global_context_t* gctx = (global_context_t*)ctx;
    ssd1306_device_t* oled_dev = &gctx->oled;
    ssd1306_clear(oled_dev,1);
    ssd1306_draw_text(oled_dev,"BLE DATA:",0,0);
    ssd1306_draw_text(oled_dev,(char*)gctx->bbuf,0,9);
    ssd1306_update(oled_dev);
    //ui_render(&gctx->ui_ctx);
}
static void mpu_task(void* ctx)
{
    global_context_t* global_context = (global_context_t*)ctx;
    mpu60x0_t* mpu = &global_context->mpu;
    int ret = mpu60x0_read_gyroscope(mpu,&global_context->gyro_x,&global_context->gyro_y,&global_context->gyro_z);
}
static void buttons_callback(void* app_context,button_t btn)
{
    global_context_t* ctx = (global_context_t*)app_context;
    if(btn == BUTTON_NONE) return;
    ctx->ui_ctx.on_button_pressed(app_context,btn);
}
static void init_sys(global_context_t* ctx)
{
    board_init();
    board_init_i2c1_pins();
    board_init_spi2_pins();
    board_init_button_input_pins();
    board_init_usart1_pins();
    LL_GPIO_SetOutputPin(GPIOC,LL_GPIO_PIN_13);
    PANIC(systime_init(1000));
    dma_config_t i2c_tx_dma_cfg =
    {
        .dma = DMA1,
        .stream = LL_DMA_STREAM_6,
        .channel = LL_DMA_CHANNEL_1,
        .direction = LL_DMA_DIRECTION_MEMORY_TO_PERIPH,
        .priority = LL_DMA_PRIORITY_LOW,
        .mode = LL_DMA_MODE_NORMAL,
        .irq = DMA1_Stream6_IRQn,
        .timeout = 3
    };
    dma_config_t usart1_rx_dma_cfg =
    {
        .dma = DMA2,
        .stream = LL_DMA_STREAM_2,
        .channel = LL_DMA_CHANNEL_4,
        .direction = LL_DMA_DIRECTION_PERIPH_TO_MEMORY,
        .priority = LL_DMA_PRIORITY_LOW,
        .mode = LL_DMA_MODE_CIRCULAR,
        .irq = DMA2_Stream2_IRQn,
        .timeout = 100
    };
    dma_config_t usart1_tx_dma_cfg =
    {
        .dma = DMA2,
        .stream = LL_DMA_STREAM_7,
        .channel = LL_DMA_CHANNEL_4,
        .direction = LL_DMA_DIRECTION_MEMORY_TO_PERIPH,
        .priority = LL_DMA_PRIORITY_LOW,
        .mode = LL_DMA_MODE_NORMAL,
        .irq = DMA2_Stream7_IRQn,
        .timeout = 100
    };
    PANIC(dma_init(&ctx->dma1stream6,&i2c_tx_dma_cfg));
    PANIC(dma_init(&ctx->dma2stream2,&usart1_rx_dma_cfg));
    PANIC(dma_init(&ctx->dma2stream7,&usart1_tx_dma_cfg));
    ctx->i2c1 = (i2c_bus_t){
        .instance = I2C1,
        .timeout = 100,
        .tx_dma = &ctx->dma1stream6
    };
    ctx->spi2 = (spi_bus_t){
        .bus = SPI2,
        .timeout = 50
    };
    PANIC(i2c_init(&ctx->i2c1,100000U));
    PANIC(spi_init(&ctx->spi2));

    usart_config_t usart1_cfg =
    {
        .instance = USART1,
        .baudrate = 9600,
        .transfer_direction = LL_USART_DIRECTION_TX_RX,
        .enable_clock = LL_USART_CLOCK_DISABLE,
        .rx_buffer = g_ctx.usart1_rxrb,
        .rx_capacity = USART1_RX_BUFFER_SIZE,
        .tx_buffer = g_ctx.usart1_txrb,
        .tx_capacity = USART1_TX_BUFFER_SIZE,
        .rx_dma = &ctx->dma2stream2,
        .tx_dma = &ctx->dma2stream7
    };
    PANIC(usart_init(&ctx->usart1,&usart1_cfg));
    memset(ctx->bbuf,0,sizeof(ctx->bbuf));
}
static void init_peripherals(global_context_t* ctx)
{
    ctx->oled.i2c_bus.bus = &ctx->i2c1;
    ctx->oled.i2c_bus.address = SSD1306_I2C_ADDR;
    ctx->mpu.i2c_bus.bus = &ctx->i2c1;
    ctx->mpu.i2c_bus.address = MPU60X0_I2C_ADDR;
    //PANIC(w25qxx_init(GPIOB,LL_GPIO_PIN_12,spi2,flash));
    PANIC(ssd1306_init(&ctx->oled));
    sleep(10);

    PANIC(ble_init(&ctx->ble,&ctx->usart1,GPIOB,LL_GPIO_PIN_5));
    sleep(10);
    /*PANIC(mpu60x0_init(mpu));
    PANIC(mpu60x0_set_sample_rate(mpu,1000));
    PANIC(mpu60x0_configure_sensors(mpu,FS_SEL_250,AFS_SEL_2));*/
}
void ui_main_on_render(void* app_context)
{
    global_context_t* ctx = (global_context_t*)app_context;
    char buf[50];
    sprintf(buf,"MAIN %c %i %c",ctx->ui_ctx.state > 0 ? '<' : ' ',ctx->ui_ctx.state,ctx->ui_ctx.state < (UI_STATE_COUNT - 1) ? '>' : ' ');
    ssd1306_draw_text(&ctx->oled,buf,0,0);
    sprintf(buf,"Device: %s\nFirmware: %s",PROJECT_NAME,PROJECT_VERSION);
    ssd1306_draw_text(&ctx->oled,buf,0,FONT_HEIGHT);
}
void ui_calibrate_on_render(void* app_context)
{
    global_context_t* ctx = (global_context_t*)app_context;
    char buf[50];
    sprintf(buf,"CALIRATION %c %i %c",ctx->ui_ctx.state > 0 ? '<' : ' ',ctx->ui_ctx.state,ctx->ui_ctx.state < (UI_STATE_COUNT - 1) ? '>' : ' ');
    ssd1306_draw_text(&ctx->oled,buf,0,0);
}
void ui_live_view_on_render(void* app_context)
{
    global_context_t* ctx = (global_context_t*)app_context;
    char buf[50];
    sprintf(buf,"LIVE_STATS %c %i %c",ctx->ui_ctx.state > 0 ? '<' : ' ',ctx->ui_ctx.state,ctx->ui_ctx.state < (UI_STATE_COUNT - 1) ? '>' : ' ');
    ssd1306_draw_text(&ctx->oled,buf,0,0);
}
void ui_transmit_on_render(void* app_context)
{
    global_context_t* ctx = (global_context_t*)app_context;
    char buf[50];
    sprintf(buf,"TRANSMIT %c %i %c",ctx->ui_ctx.state > 0 ? '<' : ' ',ctx->ui_ctx.state,ctx->ui_ctx.state < (UI_STATE_COUNT - 1) ? '>' : ' ');
    ssd1306_draw_text(&ctx->oled,buf,0,0);
}
void ui_on_button_pressed(void* app_context,button_t btn)
{
    global_context_t* ctx = (global_context_t*)app_context;
    ui_state_t state = ctx->ui_ctx.state;
    if(btn == BUTTON_LEFT)
    {
        if(state > UI_STATE_MAIN) state--;
    }
    else if(btn == BUTTON_RIGHT)
    {
        if(state < UI_STATE_COUNT) state++;
    }
    ui_set_state(&ctx->ui_ctx,state);
    /*global_context_t* ctx = (global_context_t*)app_context;
    char buf[50];
    sprintf(buf,"main_page");
    ssd1306_draw_text(ctx->oled,buf,0,SSD1306_HEIGHT / 2);
    */
}
int main()
{
    /*i2c_bus_t i2c1 = {0};
    spi_bus_t spi2 = {0};
    dma_channel_t dma1stream6 = {0};
    w25qxx_flash_t flash = {0};
    ssd1306_device_t oled = {0};
    mpu60x0_t gyro = {0};*/
    memset(&g_ctx,0,sizeof(g_ctx));
    init_sys(&g_ctx);
    init_peripherals(&g_ctx);

    ui_init(&g_ctx.ui_ctx,&g_ctx.oled,&g_ctx,ui_on_button_pressed);
    ui_page_t page;

    page = (ui_page_t){
        .on_render = ui_main_on_render
    };
    ui_add_page(&g_ctx.ui_ctx,UI_STATE_MAIN,&page);
    page = (ui_page_t){
        .on_render = ui_calibrate_on_render
    };
    ui_add_page(&g_ctx.ui_ctx,UI_STATE_CALIBRATE,&page);
    page = (ui_page_t){
        .on_render = ui_transmit_on_render
    };
    ui_add_page(&g_ctx.ui_ctx,UI_STATE_TRANSMIT,&page);
    page = (ui_page_t){
        .on_render = ui_live_view_on_render
    };
    ui_add_page(&g_ctx.ui_ctx,UI_STATE_LIVE_VIEW,&page);

    sleep(20);
    //w25qxx_read(&flash,0,0,rx_buf,sizeof(to_write));
    PANIC(buttons_init(buttons_callback,&g_ctx));
    scheduler_add_task(oled_task,1000,&g_ctx);
    scheduler_add_task(ble_rx_task,100,&g_ctx);
    //scheduler_add_task(mpu_task,10,&g_ctx);
    scheduler_run();
}
