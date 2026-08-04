#include "w25qxx.h"
#include "spi.h"
#include "err.h"
#include "systime.h"

static void spi_cs_select(w25qxx_flash_t* flash)
{
    LL_GPIO_ResetOutputPin(flash->cs_port,flash->cs_pinmask);
}
static void spi_cs_deselect(w25qxx_flash_t* flash)
{
    LL_GPIO_SetOutputPin(flash->cs_port,flash->cs_pinmask);
}
int w25qxx_init(GPIO_TypeDef* cs_port,uint32_t cs_pinmask,spi_bus_t* spi_bus,w25qxx_flash_t* flash)
{
    if(!(flash && spi_bus && cs_port && cs_pinmask)) return ERR_INV_ARG;

    *flash = (w25qxx_flash_t){
        .cs_port = cs_port,
        .cs_pinmask = cs_pinmask,
        .spi_bus = spi_bus,
    };
    RET_ERR(w25qxx_read_jedec_id(flash,&flash->manufacturer_id,&flash->memory_type,&flash->capacity));
    uint64_t total_bytes = 1ULL << flash->capacity;
    flash->pages   = total_bytes / 256;
    flash->sectors = total_bytes / 4096;
    flash->blocks  = total_bytes / 32768;

    return ERR_OK;
}
int w25qxx_read_jedec_id(w25qxx_flash_t* flash,uint8_t* manufacturer_id,uint8_t* memory_type,uint8_t* capacity)
{
    if(!flash) return ERR_INV_ARG;
    uint8_t tx_buf[4] = {0x9F,0x00,0x00,0x00};
    uint8_t rx_buf[4] = {0};
    spi_cs_select(flash);
    int status = spi_transfer(flash->spi_bus,tx_buf,rx_buf,4);
    spi_cs_deselect(flash);
    if (status == ERR_OK)
    {
        if (manufacturer_id) *manufacturer_id = rx_buf[1];
        if (memory_type)     *memory_type     = rx_buf[2];
        if (capacity)        *capacity        = rx_buf[3];
    }
    return status;
}
int w25qxx_write_enable(w25qxx_flash_t* flash)
{
    uint8_t cmds[1] = {0x06};
    spi_cs_select(flash);
    int status = spi_write(flash->spi_bus,cmds,1);
    spi_cs_deselect(flash);
    return status;
}
int w25qxx_write_disable(w25qxx_flash_t* flash)
{
    uint8_t cmds[1] = {0x04};
    spi_cs_select(flash);
    int status = spi_write(flash->spi_bus,cmds,1);
    spi_cs_deselect(flash);
    return status;
}
int w25qxx_read_streg1(w25qxx_flash_t* flash,uint8_t* s1)
{
    uint8_t tx_buf[2] = {0x05,0x00};
    uint8_t rx_buf[2] = {0};
    spi_cs_select(flash);
    int status = spi_transfer(flash->spi_bus,tx_buf,rx_buf,2);
    if(s1) *s1 = rx_buf[1];
    spi_cs_deselect(flash);
    return status;
}
int w25qxx_is_busy(w25qxx_flash_t* flash)
{
    uint8_t flag = 0;
    w25qxx_read_streg1(flash,&flag);
    return flag & W25QXX_ST1_BUSY;
}
int w25qxx_is_wel(w25qxx_flash_t* flash)
{
    uint8_t flag = 0;
    w25qxx_read_streg1(flash,&flag);
    return flag & W25QXX_ST1_WEL;
}
int w25qxx_erase_sector(w25qxx_flash_t* flash,uint16_t sector)
{
    if(!flash) return ERR_INV_ARG;
    if(w25qxx_is_busy(flash)) return ERR_BUSY;
    if(flash->sectors <= sector) return ERR_INV_ARG;
    uint32_t addr = sector * SECTOR_SIZE * PAGE_SIZE;
    uint8_t tx_buf[4] = {
        0x20,
        (addr >> 16) & 0xFF,
        (addr >> 8)  & 0xFF,
        (addr >> 0)  & 0xFF,
    };
    RET_ERR(w25qxx_write_enable(flash));
    spi_cs_select(flash);
    int status = spi_write(flash->spi_bus,tx_buf,4);
    spi_cs_deselect(flash);
    WAIT_TIMEOUT(w25qxx_is_busy(flash),400);
    return status;
}
int w25qxx_write(w25qxx_flash_t* flash,uint32_t page,uint32_t offs,const uint8_t* data,uint32_t len)
{
    if(!(flash && data)) return ERR_INV_ARG;
    if(page >= flash->pages || offs >= PAGE_SIZE) return ERR_INV_ARG;
    if(len == 0) return ERR_OK;
    if ((page * PAGE_SIZE + offs + len) > (flash->pages * PAGE_SIZE)) return ERR_INV_ARG;
    if (w25qxx_is_busy(flash)) return ERR_BUSY;
    uint8_t tx_data[266];
    uint32_t start_page = page;
    uint32_t end_page = start_page + (len + offs - 1) / PAGE_SIZE;
    uint32_t num_pages = end_page - start_page + 1;
    uint32_t data_pos = 0;
    for (uint32_t i = 0; i < num_pages;i++)
	{
		uint32_t addr = (start_page * PAGE_SIZE) + offs;
		uint16_t remaining  = (len + offs) < PAGE_SIZE ? len : PAGE_SIZE - offs;
		uint32_t index = 4;
		RET_ERR(w25qxx_write_enable(flash));
		if(!w25qxx_is_wel(flash)) return ERR_IO;
		tx_data[0] = 0x02; // page program
		tx_data[1] = (addr >> 16) & 0xFF;
		tx_data[2] = (addr >> 8)  & 0xFF;
		tx_data[3] = (addr >> 0)  & 0xFF;
		uint16_t bytes_to_send  = remaining + index;

		for (uint16_t i = 0;i < remaining;i++)
		{
			tx_data[index++] = data[i + data_pos];
		}
		spi_cs_select(flash);
		int status = spi_write(flash->spi_bus,tx_data,bytes_to_send);
		spi_cs_deselect(flash);
		RET_ERR(status);

		start_page++;
		offs = 0;
		len -= remaining;
		data_pos += remaining;
		WAIT_TIMEOUT(w25qxx_is_busy(flash),5);
	}
    return ERR_OK;
}
int w25qxx_read(w25qxx_flash_t* flash,uint32_t page,uint32_t offs,uint8_t* data,uint32_t len)
{
    if(!flash) return ERR_INV_ARG;
    if(page >= flash->pages || offs >= PAGE_SIZE) return ERR_INV_ARG;
    if(len == 0) return ERR_OK;
    if (w25qxx_is_busy(flash)) return ERR_BUSY;

    uint32_t addr = page * PAGE_SIZE + offs;
    if (addr + len > flash->pages * PAGE_SIZE) return ERR_INV_ARG;
    uint8_t tx_data[4] =
    {
        0x03,
        (addr >> 16) & 0xFF,
        (addr >> 8)  & 0xFF,
        (addr >> 0)  & 0xFF,
    };
    spi_cs_select(flash);
    int status = spi_write(flash->spi_bus,tx_data,4);
    if(status != ERR_OK)
    {
        spi_cs_deselect(flash);
        return status;
    }
    for (uint32_t i = 0; i < len; i++)
    {
        spi_read(flash->spi_bus,&data[i],1);
    }
    spi_cs_deselect(flash);
    return ERR_OK;
}
