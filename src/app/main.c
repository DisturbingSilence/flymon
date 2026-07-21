#include <drivers/ssd1306.h>
#include <services/scheduler.h>
#include <drivers/i2c.h>
#include <bsp/STM32F411CEU6.h>
#include <stdio.h>
static int counter = 0;
void oled_task()
{
    ssd1306_clear(1);
    ssd1306_set_window(0,127,0,127);
    counter++;
    char buf[50];
    sprintf(buf,"TEST #%d",counter);
    ssd1306_draw_text(buf,0,0);
    ssd1306_update();
}

int main()
{
    board_init();
    board_init_i2c1_pins();
    systime_init();
    i2c_init(I2C1);
    ssd1306_init();
    scheduler_add_task(oled_task,1000);
    scheduler_run();
}
