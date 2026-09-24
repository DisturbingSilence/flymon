#include <drivers/devices/ble.h>
#include <drivers/systime.h>
#include <drivers/err.h>

#include <config.h>
#include <string.h>
static inline void pwrc_select(ble_device_t* dev)
{
    LL_GPIO_ResetOutputPin(dev->pwrc_port,dev->pwrc_pinmask);
}
static inline void pwrc_deselect(ble_device_t* dev)
{
    LL_GPIO_SetOutputPin(dev->pwrc_port,dev->pwrc_pinmask);
}
int ble_is_connected(ble_device_t* dev)
{
    return LL_GPIO_IsInputPinSet(dev->stat_port,dev->stat_pinmask);
}
static void ble_flush_rx(ble_device_t* dev)
{
    uint8_t buffer[64];
    while(usart_available(dev->usart_bus) > 0)
    {
        uint16_t size = sizeof(buffer);
        if(ble_read(dev,buffer,&size) != ERR_OK) break;
    }
}
static void ble_wait_tx_complete(ble_device_t* dev)
{
    while (!LL_USART_IsActiveFlag_TC(dev->usart_bus->instance));
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
static inline int initial_configuration(ble_device_t* dev)
{
    const char* prefix = "+NAME:";
    char* name_start = strstr((char*)dev->broadcast_name, prefix);
    if(name_start) name_start += strlen(prefix);
    else name_start = (char*)dev->broadcast_name;

    char clean_name[32] = {0};
    strncpy(clean_name, name_start, sizeof(clean_name) - 1);
    char* cr = strpbrk(clean_name, "\r\n");
    if(cr) *cr = '\0';

    if(strcmp(clean_name, BLE_BROADCAST_NAME) == 0) return ERR_OK;

    const uint8_t set_name[] = "AT+NAME" BLE_BROADCAST_NAME "\r\n";
    const uint8_t drop_settings[] = "AT+DEFAULT\r\n";
    int err = ERR_OK;
    pwrc_select(dev);
    sleep(10);
    err = usart_write(dev->usart_bus, drop_settings, sizeof(drop_settings) - 1);
    ble_wait_tx_complete(dev);
    pwrc_deselect(dev);
    sleep(300);
    ble_flush_rx(dev);

    // Send AT+NAME
    pwrc_select(dev);
    sleep(10);
    if(err == ERR_OK)
    {
        err = usart_write(dev->usart_bus, set_name, sizeof(set_name) - 1);
        ble_wait_tx_complete(dev);
    }
    pwrc_deselect(dev);

    sleep(50);
    ble_flush_rx(dev);

    return err;
}

static int query_name(ble_device_t* dev, uint8_t* buf, uint32_t bufsize)
{
    const uint8_t at_name[] = "AT+NAME\r\n";
    pwrc_select(dev);
    sleep(10);
    int err = usart_write(dev->usart_bus, at_name, sizeof(at_name) - 1);
    ble_wait_tx_complete(dev);
    pwrc_deselect(dev);

    if(err != ERR_OK) return err;

    WAIT_TIMEOUT(usart_has_full_command(dev->usart_bus) != ERR_OK, 500);
    uint32_t avail = usart_available(dev->usart_bus);
    if(avail == 0) return ERR_EMPTY;

    memset(buf, 0, bufsize);
    if(avail > bufsize - 1) avail = bufsize - 1;
    err = usart_read(dev->usart_bus, buf, avail);
    return err;
}

static int query_version(ble_device_t* dev, uint8_t* buf, uint32_t bufsize)
{
    const uint8_t at_ver[] = "AT+VER\r\n";
    pwrc_select(dev);
    sleep(10);
    int err = usart_write(dev->usart_bus, at_ver, sizeof(at_ver) - 1);
    ble_wait_tx_complete(dev);
    pwrc_deselect(dev);

    if(err != ERR_OK) return err;

    WAIT_TIMEOUT(usart_has_full_command(dev->usart_bus) != ERR_OK, 500);
    uint32_t avail = usart_available(dev->usart_bus);
    if(avail == 0) return ERR_EMPTY;

    memset(buf, 0, bufsize);
    if(avail > bufsize - 1) avail = bufsize - 1;
    err = usart_read(dev->usart_bus, buf, avail);
    return err;
}
int ble_init(ble_device_t* dev,const ble_config_t* cfg)
{
    if(!(dev && cfg)) return ERR_INV_ARG;

    dev->usart_bus = cfg->usart_bus;
    dev->pwrc_port = cfg->pwrc_port;
    dev->pwrc_pinmask = cfg->pwrc_pinmask;
    dev->stat_port = cfg->stat_port;
    dev->stat_pinmask = cfg->stat_pinmask;

    int err = query_version(dev,dev->version,sizeof(dev->version));
    if(err != ERR_OK)return err;
    err = query_name(dev,dev->broadcast_name,sizeof(dev->broadcast_name));
    if(err != ERR_OK) return err;
    err = initial_configuration(dev);
    if(err != ERR_OK) return err;
    pwrc_deselect(dev);
    ble_flush_rx(dev);
    return ERR_OK;
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
    if(!ble_is_connected(dev)) return ERR_OK;
    static const uint8_t cmd[] = "AT+DISC\r\n";
    pwrc_select(dev);
    sleep(10);
    int err = usart_write(dev->usart_bus,cmd,sizeof(cmd) - 1);
    pwrc_deselect(dev);
    return err;
}
