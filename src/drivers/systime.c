#include "systime.h"
#include "stm32f4xx_ll_rcc.h"
#include "core_cm4.h"
static volatile systime_t systime = 0;
void SysTick_Handler()
{
    systime++;
}
systime_t systime_get()
{
    return systime;
}
int systime_init()
{
    LL_RCC_ClocksTypeDef clocks = {};
    LL_RCC_GetSystemClocksFreq(&clocks);
    if(SysTick_Config(clocks.SYSCLK_Frequency / 1000) != 0) // interrupt every millisecond
    {
        return 1;
    }
    NVIC_SetPriority(SysTick_IRQn,0);
    NVIC_EnableIRQ(SysTick_IRQn);
    return 0;
}
