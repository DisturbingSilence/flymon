#include <drivers/ssd1306.h>
#include <services/font.h>
#include <drivers/dma.h>
#include <drivers/err.h>
int ssd1306_init(ssd1306_device_t* dev)
{
    if(!dev) return ERR_INV_ARG;
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
    return ssd1306_write_cmds(dev,cmds,sizeof(cmds));
}
void ssd1306_set_window(ssd1306_device_t* dev,uint8_t page_start,uint8_t page_end,uint8_t col_start,uint8_t col_end)
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
    ssd1306_write_cmds(dev,cmds,sizeof(cmds));
}
static void ssd1306_dma_on_complete(void* ctx)
{
    ssd1306_device_t* dev = (ssd1306_device_t*)ctx;
    i2c_dma_finish(&dev->i2c_bus);
    dev->is_busy = false;
}

void ssd1306_update(ssd1306_device_t* dev)
{
    if(dev->is_busy) return;
    dev->is_busy = true;
    static uint8_t buf[] = {0x40};
    dma_transfer_t tr_info =
    {
        .prefix_data = buf,
        .prefix_len = 1,
        .src = (uint32_t)dev->framebuffer,
        .dst = (uint32_t)&dev->i2c_bus.bus->instance->DR,
        .len = SSD1306_FRAMEBUFFER_SIZE,
        .on_tx_complete_callback = ssd1306_dma_on_complete,
        .ctx = dev
    };
    if(i2c_dma_write(&dev->i2c_bus,&tr_info) != ERR_OK)
    {
        dev->is_busy = false;
    }
}
void ssd1306_set_pixel(ssd1306_device_t* dev,unsigned x,unsigned y,bool value)
{
    if(x >= SSD1306_WIDTH || y >= SSD1306_HEIGHT) return;
    unsigned byte = x + (y >> 3) * SSD1306_WIDTH;
    uint8_t bit = 1 << (y & 7);

    if (value)
        dev->framebuffer[byte] |= bit;
    else
        dev->framebuffer[byte] &= ~bit;
}
void ssd1306_draw_rect(ssd1306_device_t* dev,unsigned x1,unsigned y1,unsigned width,unsigned height,bool color)
{
    for(unsigned x = x1;x < x1 + width;x++)
    {
        for(unsigned y = y1;y < y1 + height;y++)
        {
            ssd1306_set_pixel(dev,x,y,color);
        }
    }
}
void ssd1306_clear(ssd1306_device_t* dev,bool value)
{
    uint8_t color = value ? UINT8_MAX : 0;
    for(unsigned i = 0;i < SSD1306_FRAMEBUFFER_SIZE;i++)
    {
        dev->framebuffer[i] = color;
    }
}
int ssd1306_write_cmds(ssd1306_device_t* dev,const uint8_t* cmds,uint32_t num_cmds)
{
    return i2c_write(&dev->i2c_bus,cmds,num_cmds);
}
void ssd1306_draw_bmp(ssd1306_device_t* dev,const uint8_t* pixels,unsigned width,unsigned height,unsigned x1,unsigned y1)
{
    unsigned total_size = width * height / 8;
    uint8_t bitmask;
    unsigned curx = x1,cury = y1;
    for(unsigned i = 0;i < total_size;i++)
    {
        for(uint8_t bit = 0;bit < 8;bit++)
        {
            bitmask = 0b10000000 >> bit;
            ssd1306_set_pixel(dev,curx,cury,pixels[i] & bitmask);
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
void ssd1306_draw_text(ssd1306_device_t* dev,const char* txt,unsigned x1,unsigned y1)
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
        ssd1306_draw_bmp(dev,FONT[*txt - ' '],fwidth,FONT_HEIGHT,curx,cury);
        curx += fwidth;
        ++txt;
    }
}
void ssd1306_draw_line(ssd1306_device_t* dev,unsigned x1,unsigned y1,unsigned x2,unsigned y2,bool color)
{
    unsigned m_new = 2 * (y2 - y1);
    unsigned slope_error_new = m_new - (x2 - x1);
    for (int x = x1, y = y1; x <= x2; x++)
    {
        ssd1306_set_pixel(dev,x,y,color);
        slope_error_new += m_new;
        if (slope_error_new >= 0)
        {
            y++;
            slope_error_new -= 2 * (x2 - x1);
        }
    }
}
