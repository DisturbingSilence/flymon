#include <drivers/ssd1306.h>
#include <services/scheduler.h>
#include <drivers/i2c.h>
#include <bsp/STM32F411CEU6.h>
#include <stdio.h>
#include <drivers/w25qxx.h>
#include <drivers/err.h>
static int counter = 0;
void oled_task()
{
    ssd1306_clear(1);
    ssd1306_set_window(0,127,0,127);
    counter++;
    char buf[50];
    /*uint8_t mfr_id = 0,mem_type = 0,capacity = 0;
    if (w25qxx_read_jedec_id(&mfr_id,&mem_type,&capacity) == ERR_OK)
    {
        sprintf(buf,"TEST:#%d\nMF=0x%02X\nMTP=0x%02X,CAP=0x%02X\n%s",counter,
            mfr_id,mem_type,capacity,(mfr_id == 0xEF && capacity == 0x17) ? "W25Q64JV DETECTED" : "FAILED");
    }
    else
    {
        sprintf(buf,"FAILED TO READ JEDEC\n ID");
        }*/
    sprintf(buf,"TEST %d",counter);
    ssd1306_draw_text(buf,0,0);
    ssd1306_update();
}

int main()
{
    board_init();
    board_init_i2c1_pins();
    board_init_spi2_pins();
    systime_init(1000);
    w25qxx_init();
    //w25qxx_read_jedec_id(&mf,&mem,&cap);
    ssd1306_init();
    scheduler_add_task(oled_task,1000);
    scheduler_run();
}
