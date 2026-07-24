#include <drivers/ssd1306.h>
#include <services/scheduler.h>
#include <drivers/i2c.h>
#include <bsp/STM32F411CEU6.h>
#include <stdio.h>
#include <drivers/w25qxx.h>
#include <drivers/err.h>
#include "stm32f4xx_ll_gpio.h"
static int counter = 0;
void oled_task()
{
    ssd1306_clear(1);
    ssd1306_set_window(0,7,0,127);
    counter++;
    char buf[50];
    sprintf(buf,"TEST %d",counter);
    /*uint8_t mfr_id = 0,mem_type = 0,capacity = 0;
    if (w25qxx_read_jedec_id(&mfr_id,&mem_type,&capacity) == ERR_OK)
    {
        sprintf(buf,"TEST:#%d\nMF=0x%02X\nMTP=0x%02X,CAP=0x%02X\n%s",counter,
            mfr_id,mem_type,capacity,(mfr_id == 0xEF && capacity == 0x17) ? "W25Q64JV DETECTED" : "FAILED");
    }
    else
    {
        sprintf(buf,"FAILED TO READ JEDEC\n ID");
        }
    uint8_t cmds[] =
    {
        0x00,                       // control byte
        0x21,0,127,     // reset column start/end addr
        0x22,0,7    // reset page start/end addr
    };
    int err = ssd1306_write_cmds(cmds, sizeof(cmds));
    if (err != ERR_OK)
    {
        LL_GPIO_TogglePin(GPIOC,LL_GPIO_PIN_13);
        }*/
        //sprintf(buf,"TEST %d",counter);
    ssd1306_draw_text(buf,0,0);
    ssd1306_update();
}

int main()
{
    board_init();
    board_init_i2c1_pins();
    //board_init_spi2_pins();
    LL_GPIO_ResetOutputPin(GPIOC,LL_GPIO_PIN_13);
    systime_init(1000);
    //w25qxx_init();
    //w25qxx_read_jedec_id(&mf,&mem,&cap);
    if (ssd1306_init() != ERR_OK) { LL_GPIO_TogglePin(GPIOC, LL_GPIO_PIN_13); while(1); }
    ssd1306_init();
    scheduler_add_task(oled_task,1000);
    scheduler_run();
}
