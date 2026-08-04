#include <drivers/ssd1306.h>
#include <services/scheduler.h>
#include <drivers/i2c.h>
#include <bsp/STM32F411CEU6.h>
#include <stdio.h>
#include <drivers/w25qxx.h>
#include <drivers/mpu60x0.h>
#include <drivers/err.h>
#include "stm32f4xx_ll_gpio.h"

typedef struct
{
    ssd1306_device_t* oled;
    mpu60x0_t* mpu;
    int16_t gyro_x;
    int16_t gyro_y;
    int16_t gyro_z;
} global_context_t;

void oled_task(void* ctx)
{
    static int counter = 0;
    global_context_t* gctx = (global_context_t*)ctx;
    ssd1306_device_t* oled_dev = gctx->oled;
    ssd1306_clear(oled_dev,1);
    ssd1306_set_window(oled_dev,0,7,0,127);
    counter++;
    char buf[50];
    sprintf(buf,"GX:%2d\nGY:%2d\nGZ:%2d\nCNT:%i",gctx->gyro_x,gctx->gyro_y,gctx->gyro_z,counter);
    //sprintf(buf,"TEST682 %d",counter);
    /*if (mfr_id != 0)
    {

    }
    else
    {
        sprintf(buf,"FAILED TO READ JEDEC\n ID");
    }*/
    ssd1306_draw_text(oled_dev,buf,0,0);
    ssd1306_update(oled_dev);
}
static void mpu_task(void* ctx)
{
    global_context_t* global_context = (global_context_t*)ctx;
    mpu60x0_t* mpu = global_context->mpu;
    int ret = mpu60x0_read_gyroscope(mpu,&global_context->gyro_x,&global_context->gyro_y,&global_context->gyro_z);
}
static void init_sys(dma_channel_t* d1s6,i2c_bus_t* i2c1,spi_bus_t* spi2)
{
    board_init();
    board_init_i2c1_pins();
    board_init_spi2_pins();
    LL_GPIO_SetOutputPin(GPIOC,LL_GPIO_PIN_13);
    PANIC(systime_init(1000));

    dma_config_t cfg =
    {
        .dma = DMA1,
        .stream = LL_DMA_STREAM_6,
        .channel = LL_DMA_CHANNEL_1,
        .direction = LL_DMA_DIRECTION_MEMORY_TO_PERIPH,
        .priority = LL_DMA_PRIORITY_LOW,
    };
    d1s6->timeout = 1;
    PANIC(dma_init(d1s6,&cfg));

    *i2c1 = (i2c_bus_t){
        .instance = I2C1,
        .timeout = 100,
        .clock_speed = 100000U,
        .tx_dma = d1s6
    };
    *spi2 = (spi_bus_t){
        .bus = SPI2,
        .timeout = 50
    };
    PANIC(i2c_init(i2c1));
    PANIC(spi_init(spi2));
}
static void init_peripherals(i2c_bus_t* i2c1,spi_bus_t* spi2,ssd1306_device_t* oled_dev,w25qxx_flash_t* flash,mpu60x0_t* mpu)
{
    oled_dev->i2c_bus.bus = i2c1;
    oled_dev->i2c_bus.address = SSD1306_I2C_ADDR;
    mpu->i2c_bus.bus = i2c1;
    mpu->i2c_bus.address = MPU60X0_I2C_ADDR;
    //PANIC(w25qxx_init(GPIOB,LL_GPIO_PIN_12,spi2,flash));
    static volatile int num_devs = 0;
    /*for(uint8_t addr = 102; addr < 127; addr++)
    {

    }*/
    /*if(i2c_probe(i2c1, 0xD2) == ERR_OK)
    {
        num_devs++;
        //printf("Found device: 0x%02X\n", addr);
        }*/

    PANIC(ssd1306_init(oled_dev));
    sleep(10);
    PANIC(mpu60x0_init(mpu));
    PANIC(mpu60x0_set_sample_rate(mpu,1000));
    PANIC(mpu60x0_configure_sensors(mpu,FS_SEL_250,AFS_SEL_2));
}


int main()
{
    i2c_bus_t i2c1 = {0};
    spi_bus_t spi2 = {0};
    dma_channel_t dma1stream6 = {0};
    w25qxx_flash_t flash = {0};
    ssd1306_device_t oled = {0};
    mpu60x0_t gyro = {0};
    init_sys(&dma1stream6,&i2c1,&spi2);
    init_peripherals(&i2c1,&spi2,&oled,&flash,&gyro);
    sleep(20);
    //w25qxx_read(&flash,0,0,rx_buf,sizeof(to_write));
    global_context_t g_ctx =
    {
         .oled = &oled,
         .mpu = &gyro
    };
    scheduler_add_task(oled_task,1000,&g_ctx);
    scheduler_add_task(mpu_task,10,&g_ctx);
    scheduler_run();
}
