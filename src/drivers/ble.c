#include <drivers/ble.h>
#include <drivers/err.h>
#include <drivers/systime.h>
#include <string.h>
static inline void pwrc_select(ble_device_t* dev)
{
    LL_GPIO_ResetOutputPin(dev->pwrc_port,dev->pwrc_pinmask);
}
static inline void pwrc_deselect(ble_device_t* dev)
{
    LL_GPIO_SetOutputPin(dev->pwrc_port,dev->pwrc_pinmask);
}
static int query_name(ble_device_t* dev,uint8_t* buf,uint32_t bufsize)
{
    const uint8_t at_name[] = "AT+NAME\r\n";
    pwrc_select(dev);
    int err = usart_write(dev->usart_bus,at_name,sizeof(at_name) - 1);
    pwrc_deselect(dev);
    if(err != ERR_OK) return err;
    WAIT_TIMEOUT(usart_has_full_command(dev->usart_bus) != ERR_OK,500);
    uint32_t avail = usart_available(dev->usart_bus);
    if(avail == 0) return ERR_EMPTY;
    memset(dev->buf,0,bufsize);
    if(avail > bufsize - 1) avail = bufsize - 1;
    err = usart_read(dev->usart_bus,buf,avail);
    return err;
}
static int query_version(ble_device_t* dev,uint8_t* buf,uint32_t bufsize)
{
    const uint8_t at_ver[] = "AT+VER\r\n";
    pwrc_select(dev);
    int err = usart_write(dev->usart_bus,at_ver,sizeof(at_ver) - 1);
    pwrc_deselect(dev);
    if(err != ERR_OK) return err;
    WAIT_TIMEOUT(usart_has_full_command(dev->usart_bus) != ERR_OK,500);
    uint32_t avail = usart_available(dev->usart_bus);
    if(avail == 0) return ERR_EMPTY;
    memset(dev->buf,0,bufsize);
    if(avail > bufsize - 1) avail = bufsize - 1;
    err = usart_read(dev->usart_bus,buf,avail);
    return err;
}
static int usart_has_full_command(usart_bus_t* bus)
{
    uint32_t avail = usart_available(bus);
    if(avail < 2) return ERR_INCOMPLETE;
    uint8_t prev = 0, cur = 0;
    for(uint32_t i = 0;i < avail;i++)
    {
        ringbuffer_peek_at(&bus->rx_rb,i,&cur);
        if(prev == '\r' && cur == '\n') return ERR_OK;
        prev = cur;
    }
    return ERR_INCOMPLETE;
}
int ble_init(ble_device_t* dev,const ble_config_t* cfg)
{
    if (!(dev && cfg)) return ERR_INV_ARG;
    dev->usart_bus = cfg->usart_bus;
    dev->pwrc_port = cfg->pwrc_port;
    dev->pwrc_pinmask = cfg->pwrc_pinmask;

    int err = query_version(dev,dev->version,sizeof(dev->version));
    if(err != ERR_OK) return err;
    return query_name(dev,dev->broadcast_name,sizeof(dev->broadcast_name));
}
int ble_read(ble_device_t* dev,uint8_t* data,uint16_t* size)
{
    if(!(dev && data && size)) return ERR_INV_ARG;

    uint32_t usart_size = usart_available(dev->usart_bus);
    if(*size == 0 || *size > usart_size) *size = usart_size;

    return usart_read(dev->usart_bus,data,*size);
}
int ble_write(ble_device_t* dev,const uint8_t* data,uint16_t size)
{
    if(!(dev && data)) return ERR_INV_ARG;

    return usart_write(dev->usart_bus,data,size);
}
uint32_t ble_available(ble_device_t* dev)
{
    if(!dev) return 0;
    return usart_available(dev->usart_bus);
}
int ble_disconnect(ble_device_t* dev)
{
    if(!dev) return ERR_INV_ARG;

    const uint8_t at_disc[] = "AT+DISC\r\n";
    pwrc_select(dev);
    int err = usart_write(dev->usart_bus,at_disc,sizeof(at_disc) - 1);
    pwrc_deselect(dev);

    return err;
}
