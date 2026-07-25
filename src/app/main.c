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
void oled_task()
{
    ssd1306_clear(1);
    ssd1306_set_window(0,7,0,127);
    counter++;
    char buf[50];
    char* pos = buf + sprintf(buf,"VALS:");
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
    ssd1306_draw_text(buf,0,0);
    ssd1306_update();
}

int main()
{
    board_init();
    board_init_i2c1_pins();
    board_init_spi2_pins();
    LL_GPIO_ResetOutputPin(GPIOC,LL_GPIO_PIN_13);
    systime_init(1000);
    w25qxx_flash_t flash;
    if(w25qxx_init(GPIOB,LL_GPIO_PIN_12,SPI2,&flash) != ERR_OK) { LL_GPIO_TogglePin(GPIOC, LL_GPIO_PIN_13); while(1); }
    if (ssd1306_init() != ERR_OK) { LL_GPIO_TogglePin(GPIOC, LL_GPIO_PIN_13); while(1); }

    w25qxx_write(&flash,0,0,to_write,sizeof(to_write));
    sleep(20);
    w25qxx_read(&flash,0,0,rx_buf,sizeof(to_write));
    ssd1306_init();
    scheduler_add_task(oled_task,1000);
    scheduler_run();
}
