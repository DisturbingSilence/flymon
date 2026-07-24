#pragma once
#include <stdbool.h>
#include <stdint.h>

#define SSD1306_ADDR 0x78
#define SSD1306_WIDTH 128
#define SSD1306_HEIGHT 64
#define SSD1306_FRAMEBUFFER_SIZE ((SSD1306_WIDTH * SSD1306_HEIGHT) / 8)

int ssd1306_init();
int ssd1306_write_cmds(const uint8_t* cmds,uint32_t num_cmds);
void ssd1306_set_window(uint8_t page_start,uint8_t page_end,uint8_t col_start,uint8_t col_end);
void ssd1306_update();

void ssd1306_set_pixel(unsigned x,unsigned y,bool value);
void ssd1306_draw_rect(unsigned x1,unsigned y1,unsigned width,unsigned height);
void ssd1306_clear(bool value);
void ssd1306_draw_text(const char* txt,unsigned x1,unsigned y1);
void ssd1306_draw_bmp(const uint8_t* pixels,unsigned width,unsigned height,unsigned x1,unsigned y1);
