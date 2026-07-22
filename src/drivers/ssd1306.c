#include "ssd1306.h"
#include "stm32f4xx_ll_i2c.h"
#include "stm32f4xx_ll_dma.h"
#include <services/font.h>
#include <drivers/i2c.h>
#include <drivers/dma.h>
#include <drivers/err.h>
uint8_t ssd1306_framebuffer[SSD1306_FRAMEBUFFER_SIZE] = {};
static volatile bool is_oled_busy = false;
void ssd1306_init()
{
    i2c_init(I2C1);
    dma_config_t dma_cfg =
    {
        .dma = DMA1,
        .stream = LL_DMA_STREAM_6,
        .channel = LL_DMA_CHANNEL_1,
        .direction = LL_DMA_DIRECTION_MEMORY_TO_PERIPH,
        .priority = LL_DMA_PRIORITY_LOW,
    };
    dma_init(&dma_cfg);
    uint8_t cmds[] =
    {
        0x00,               // Control byte Co = 0,D/C# = 0. All next bytes are commands
        0xAE,               // Display off
        0x20, 0x00,         // Memory addressing mode = horizontal
        0x21, 0x00, 0x7F,   // Column address [0;127]
        0x22, 0x00, 0x07,   // Set Page Address [0;7]
        0x8D, 0x14,         // Enable charge pump
        0xA8, 0x3F,         // Multiplex ratio (1/64)
        0xD3, 0x00,         // Display offset (0)
        0x40,               // Start line address (0)
        0xA1,               // Segment remap
        0xC8,               // COM Output scan direction
        0xDA, 0x12,         // COM pins hardware configuration
        0x81, 0x7F,         // Contrast (0-255)
        0xA4,               // Resume to RAM content display
        0xA6,               // Normal display
        0x2E,               // Deactivate scroll
        0xAF                // Display ON
    };
    ssd1306_write_cmds(cmds,sizeof(cmds));
}
void ssd1306_set_window(uint8_t page_start,uint8_t page_end,uint8_t col_start,uint8_t col_end)
{
    if(col_start > 127) col_start = 127;
    if(col_end > 127) col_end = 127;
    if(page_start > 7) page_start = 7;
    if(page_end > 7) page_end = 7;
    uint8_t cmds[] =
    {
        0x00,                       // control byte
        0x21,col_start,col_end,     // reset column start/end addr
        0x22,page_start,page_end    // reset page start/end addr
    };
    ssd1306_write_cmds(cmds,sizeof(cmds));
}
static void ssd1306_dma_on_complete()
{
    i2c_dma_finish(I2C1);
    is_oled_busy = false;
}

void ssd1306_update()
{
    if(is_oled_busy) return;
    is_oled_busy = true;
    static uint8_t buf[] = {0x40};

    dma_transfer_t tr_info =
    {
        .dma = DMA1,
        .stream = LL_DMA_STREAM_6,
        .prefix_data = buf,
        .prefix_len = 1,
        .src = (uint32_t)ssd1306_framebuffer,
        .dst = (uint32_t)&I2C1->DR,
        .len = SSD1306_FRAMEBUFFER_SIZE,
        .on_complete_callback = ssd1306_dma_on_complete
    };
    if(i2c_dma_write(I2C1,SSD1306_ADDR,&tr_info) != ERR_OK)
    {
        is_oled_busy = false;
    }
}
void ssd1306_set_pixel(unsigned x,unsigned y,bool value)
{
    if(x >= SSD1306_WIDTH || y >= SSD1306_HEIGHT) return;
    unsigned byte = x + (y >> 3) * SSD1306_WIDTH;
    uint8_t bit = 1 << (y & 7);

    if (value)
        ssd1306_framebuffer[byte] |= bit;
    else
        ssd1306_framebuffer[byte] &= ~bit;
}
void ssd1306_draw_rect(unsigned x1,unsigned y1,unsigned width,unsigned height)
{
    for(unsigned x = x1;x < x1 + width;x++)
    {
        for(unsigned y = y1;y < y1 + height;y++)
        {
            ssd1306_set_pixel(x,y,1);
        }
    }
}
void ssd1306_clear(bool value)
{
    uint8_t color = value ? UINT8_MAX : 0;
    for(unsigned i = 0;i < SSD1306_FRAMEBUFFER_SIZE;i++)
    {
        ssd1306_framebuffer[i] = color;
    }
}
void ssd1306_write_cmds(const uint8_t* cmds,uint32_t num_cmds)
{
    i2c_write(I2C1,SSD1306_ADDR,cmds,num_cmds);
}
void ssd1306_draw_bmp(const uint8_t* pixels,unsigned width,unsigned height,unsigned x1,unsigned y1)
{
    unsigned total_size = width * height / 8;
    uint8_t bitmask;
    unsigned curx = x1,cury = y1;
    for(unsigned i = 0;i < total_size;i++)
    {
        for(uint8_t bit = 0;bit < 8;bit++)
        {
            bitmask = 0b10000000 >> bit;
            ssd1306_set_pixel(curx,cury,pixels[i] & bitmask);
            curx++;
            if(curx - x1 == width)
            {
                cury++;
                curx = x1;
            }
            if(cury - y1 == height) return;
        }
    }
}
void ssd1306_draw_text(const char* txt,unsigned x1,unsigned y1)
{
    unsigned curx = x1,cury = y1;
    const unsigned fwidth = FONT_BWIDTH * 8;
    while(*txt)
    {
        if(curx >= SSD1306_WIDTH)
        {
            curx = x1;
            cury += FONT_HEIGHT;
        }
        if(cury >= SSD1306_HEIGHT) return;
        if (*txt == '\n')
        {
            curx = x1;
            cury += FONT_HEIGHT;
            ++txt;
            continue;
        }
        ssd1306_draw_bmp(FONT[*txt - ' '],fwidth,FONT_HEIGHT,curx,cury);
        curx += fwidth;
        ++txt;
    }
}
