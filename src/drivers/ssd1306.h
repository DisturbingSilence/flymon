#pragma once
#include <stdbool.h>
#include <stdint.h>
#include <drivers/i2c.h>
#include <drivers/dma.h>
#define SSD1306_I2C_ADDR 0x78
#define SSD1306_WIDTH 128
#define SSD1306_HEIGHT 64
#define SSD1306_FRAMEBUFFER_SIZE ((SSD1306_WIDTH * SSD1306_HEIGHT) / 8)

typedef struct
{
    i2c_bus_t* i2c_bus;
    uint8_t framebuffer[SSD1306_FRAMEBUFFER_SIZE];
    bool is_busy;
} ssd1306_device_t;

int ssd1306_init(ssd1306_device_t* dev);
int ssd1306_write_cmds(ssd1306_device_t* dev,const uint8_t* cmds,uint32_t num_cmds);
void ssd1306_set_window(ssd1306_device_t* dev,uint8_t page_start,uint8_t page_end,uint8_t col_start,uint8_t col_end);
void ssd1306_update(ssd1306_device_t* dev);

void ssd1306_set_pixel(ssd1306_device_t* dev,unsigned x,unsigned y,bool value);
void ssd1306_draw_rect(ssd1306_device_t* dev,unsigned x1,unsigned y1,unsigned width,unsigned height);
void ssd1306_clear(ssd1306_device_t* dev,bool value);
void ssd1306_draw_text(ssd1306_device_t* dev,const char* txt,unsigned x1,unsigned y1);
void ssd1306_draw_bmp(ssd1306_device_t* dev,const uint8_t* pixels,unsigned width,unsigned height,unsigned x1,unsigned y1);
