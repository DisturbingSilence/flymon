#include <services/buttons.h>
#include <drivers/systime.h>
#include <drivers/err.h>

#include "stm32f4xx_ll_adc.h"
#include "stm32f4xx_ll_bus.h"
#include "stm32f4xx_ll_system.h"
#include "core_cm4.h"
#include "stm32f4xx_ll_rcc.h"
#include "stm32f4xx_ll_tim.h"

static button_callback_t btn_callback = 0;
static void* btn_callback_context = 0;
static button_t last_stable_button = BUTTON_NONE;
static button_t current_candidate = BUTTON_NONE;
static uint8_t candidate_count = 0;
int buttons_init(button_callback_t clbck,void* ctx)
{
    if(!(clbck && ctx)) return ERR_INV_ARG;
    int err;
    btn_callback = clbck;
    btn_callback_context = ctx;

    LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_ADC1);
    LL_ADC_Disable(ADC1);
    LL_ADC_CommonInitTypeDef common_adc_cfg =
    {
        .CommonClock = LL_ADC_CLOCK_SYNC_PCLK_DIV2
    };
    LL_ADC_InitTypeDef adc_cfg =
    {
        .Resolution = LL_ADC_RESOLUTION_12B,
        .DataAlignment = LL_ADC_DATA_ALIGN_RIGHT,
        .SequencersScanMode = LL_ADC_SEQ_SCAN_DISABLE
    };
    LL_ADC_REG_InitTypeDef adc_reg_cfg =
    {
        .TriggerSource = LL_ADC_REG_TRIG_EXT_TIM2_TRGO,
        .SequencerLength = LL_ADC_REG_SEQ_SCAN_DISABLE,
        .SequencerDiscont = LL_ADC_REG_SEQ_DISCONT_DISABLE,
        .ContinuousMode = LL_ADC_REG_CONV_SINGLE,
        .DMATransfer = LL_ADC_REG_DMA_TRANSFER_NONE
    };
    err = LL_ADC_CommonInit(__LL_ADC_COMMON_INSTANCE(ADC),&common_adc_cfg);
    if(err != SUCCESS) return err;
    err = LL_ADC_Init(ADC1,&adc_cfg);
    if(err != SUCCESS) return err;
    err = LL_ADC_REG_Init(ADC1,&adc_reg_cfg);
    if(err != SUCCESS) return err;
    LL_ADC_EnableIT_EOCS(ADC1);
    NVIC_EnableIRQ(ADC_IRQn);
    LL_ADC_SetChannelSamplingTime(ADC1,LL_ADC_CHANNEL_8,LL_ADC_SAMPLINGTIME_56CYCLES);
    LL_ADC_REG_SetSequencerRanks(ADC1,LL_ADC_REG_RANK_1,LL_ADC_CHANNEL_8);

    LL_ADC_Enable(ADC1);
    sleep(1); // tSTAB
    LL_ADC_REG_StartConversionExtTrig(ADC1,LL_ADC_REG_TRIG_EXT_RISING);

    LL_RCC_ClocksTypeDef clocks = {};
    LL_RCC_GetSystemClocksFreq(&clocks);

    LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_TIM2);
    uint32_t tim2_freq = clocks.PCLK1_Frequency * (LL_RCC_GetAPB1Prescaler() == 1 ? 1 : 2);

    uint32_t prescaler = __LL_TIM_CALC_PSC(tim2_freq,10000);
    uint32_t arr = __LL_TIM_CALC_ARR(tim2_freq,prescaler,BTN_SAMPLING_FREQUENCY);
    LL_TIM_InitTypeDef tim2_cfg =
    {
        .Prescaler         = prescaler,
        .CounterMode       = LL_TIM_COUNTERMODE_UP,
        .Autoreload        = arr,
        .ClockDivision     = LL_TIM_CLOCKDIVISION_DIV1,
        .RepetitionCounter = 0,
    };
    if(!IS_TIM_MASTER_INSTANCE(TIM2)) return ERR_INIT_FAILURE;

    err = LL_TIM_Init(TIM2,&tim2_cfg);
    if(err != SUCCESS) return err;
    LL_TIM_SetTriggerOutput(TIM2,LL_TIM_TRGO_UPDATE);

    LL_TIM_EnableCounter(TIM2);
    return err == SUCCESS ? ERR_OK : ERR_INIT_FAILURE;
}
static button_t button_from_adc(uint16_t v)
{
    if (v < 200)
        return BUTTON_NONE;
    else if (v < 1200)
        return BUTTON_SELECT;
    else if (v < 2800)
        return BUTTON_RIGHT;
    else
        return BUTTON_LEFT;
}

void ADC_IRQHandler(void)
{
    if (!LL_ADC_IsActiveFlag_EOCS(ADC1))
        return;

    uint16_t value = LL_ADC_REG_ReadConversionData12(ADC1);
    button_t sampled_btn = button_from_adc(value);

    if (sampled_btn != current_candidate)
    {
        current_candidate = sampled_btn;
        candidate_count = 1;
        return;
    }
    if (candidate_count < DEBOUNCE_STABLE_COUNT)
    {
        candidate_count++;
    }
    if (candidate_count < DEBOUNCE_STABLE_COUNT) return;
    if (current_candidate != BUTTON_NONE)
    {
        last_stable_button = current_candidate;

        if (btn_callback)
        {
            btn_callback(btn_callback_context, current_candidate);
        }
    }
}
