#include <services/ble_protocol.h>
#include <drivers/err.h>

int ble_protocol_init(ble_protocol_t* protocol,const ble_protocol_cfg_t* cfg)
{
    if(!(protocol && cfg)) return ERR_INV_ARG;

    return ERR_OK;
}
