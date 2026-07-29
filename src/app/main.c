#include <drivers/ssd1306.h>
#include <services/scheduler.h>
#include <drivers/i2c.h>
#include <bsp/STM32F411CEU6.h>
#include <stdio.h>
#include <drivers/w25qxx.h>
#include <drivers/err.h>
#include "stm32f4xx_ll_gpio.h"

static int counter = 0;
static uint8_t to_write[] = {0xDE,0xAD,0xBE,0xEF};
static uint8_t rx_buf[sizeof(to_write)] = {};

ssd1306_device_t oled_dev;
void oled_task()
{
    ssd1306_clear(&oled_dev,1);
    ssd1306_set_window(&oled_dev,0,7,0,127);
    counter++;
    char buf[50];
    char* pos = buf + sprintf(buf,"VCC:");
    for (int i = 0 ; i < sizeof(to_write);i++)
    {
        pos += sprintf(pos,"0X%02X,",to_write[i]);
    }
    /*if (mfr_id != 0)
    {
        sprintf(buf,"TEST:#%d\nMF=0X%02X\nCAP=0X%02X\nMT=0X%02X",counter,
            mfr_id,capacity,mem_type);
    }
    else
    {
        sprintf(buf,"FAILED TO READ JEDEC\n ID");
    }*/
    ssd1306_draw_text(&oled_dev,buf,0,0);
    ssd1306_update(&oled_dev);
}
int main()
{
    board_init();
    board_init_i2c1_pins();
    board_init_spi2_pins();
    LL_GPIO_SetOutputPin(GPIOC,LL_GPIO_PIN_13);
    PANIC(systime_init(1000));
    w25qxx_flash_t flash;
    PANIC(w25qxx_init(GPIOB,LL_GPIO_PIN_12,SPI2,&flash));
    dma_config_t cfg =
    {
        .dma = DMA1,
        .stream = LL_DMA_STREAM_6,
        .channel = LL_DMA_CHANNEL_1,
        .direction = LL_DMA_DIRECTION_MEMORY_TO_PERIPH,
        .priority = LL_DMA_PRIORITY_LOW,
    };
    dma_channel_t dma1stream6;
    dma1stream6.timeout = 1;
    PANIC(dma_init(&dma1stream6,&cfg));
    i2c_bus_t oled_i2c_bus =
    {
        .bus = I2C1,
        .timeout = 3,
        .address = SSD1306_I2C_ADDR,
        .tx_dma = &dma1stream6
    };
    PANIC(i2c_init(&oled_i2c_bus));
    oled_dev.i2c_bus = &oled_i2c_bus;
    PANIC(ssd1306_init(&oled_dev));
    sleep(20);
    w25qxx_read(&flash,0,0,rx_buf,sizeof(to_write));
    scheduler_add_task(oled_task,1000);
    scheduler_run();
}
