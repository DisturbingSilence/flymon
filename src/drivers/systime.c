#include "systime.h"
#include "stm32f4xx_ll_rcc.h"
#include "core_cm4.h"
#include "err.h"
static volatile systime_t systime = 0;
void SysTick_Handler()
{
    systime++;
}
systime_t systime_get()
{
    return systime;
}
int systime_init(systime_t ticks_per_second)
{
    LL_RCC_ClocksTypeDef clocks = {};
    LL_RCC_GetSystemClocksFreq(&clocks);
    if(SysTick_Config(clocks.SYSCLK_Frequency / ticks_per_second) != 0)
    {
        return ERR_INIT_FAILURE;
    }
    NVIC_EnableIRQ(SysTick_IRQn);
    return ERR_OK;
}
void sleep(systime_t ticks)
{
    systime_t start = systime;
    while(systime - start < ticks);
}
