#include <bsp/STM32F411CEU6.h>
#include "stm32f4xx_ll_gpio.h"
#include "stm32f4xx_ll_bus.h"

int board_init()
{
    LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_GPIOB);
    LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_GPIOC);
    LL_GPIO_InitTypeDef gpio_cfg = {};
    gpio_cfg.Pin = LL_GPIO_PIN_13;
    gpio_cfg.Mode = LL_GPIO_MODE_OUTPUT;
    gpio_cfg.Pull = LL_GPIO_PULL_NO;
    gpio_cfg.Speed = LL_GPIO_SPEED_FREQ_LOW;
    gpio_cfg.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
    return LL_GPIO_Init(GPIOC,&gpio_cfg) == SUCCESS;
}
int board_init_i2c1_pins()
{
    LL_GPIO_InitTypeDef gpio_cfg = {};
    gpio_cfg.Pin = LL_GPIO_PIN_8 | LL_GPIO_PIN_9;
    gpio_cfg.Speed = LL_GPIO_SPEED_FREQ_LOW;
    gpio_cfg.Pull = LL_GPIO_PULL_NO;
    gpio_cfg.Mode = LL_GPIO_MODE_ALTERNATE;
    gpio_cfg.OutputType = LL_GPIO_OUTPUT_OPENDRAIN;
    gpio_cfg.Alternate = LL_GPIO_AF_4;
    return LL_GPIO_Init(GPIOB,&gpio_cfg) == SUCCESS;
}
int board_init_usart1_pins()
{
    LL_GPIO_InitTypeDef gpio_cfg = {};
    gpio_cfg.Pin = LL_GPIO_PIN_6 | LL_GPIO_PIN_7;
    gpio_cfg.Mode = LL_GPIO_MODE_ALTERNATE;
    gpio_cfg.Pull = LL_GPIO_PULL_NO;
    gpio_cfg.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
    gpio_cfg.Speed = LL_GPIO_SPEED_FREQ_VERY_HIGH;
    gpio_cfg.Alternate = LL_GPIO_AF_7;
    int err = LL_GPIO_Init(GPIOB,&gpio_cfg);
    if(err != SUCCESS) return err;
    gpio_cfg.Pin = LL_GPIO_PIN_5;
    gpio_cfg.Mode = LL_GPIO_MODE_OUTPUT;
    gpio_cfg.Pull = LL_GPIO_PULL_UP;
    gpio_cfg.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
    gpio_cfg.Speed = LL_GPIO_SPEED_FREQ_LOW;
    err = LL_GPIO_Init(GPIOB,&gpio_cfg);
    if(err != SUCCESS) return err;
    gpio_cfg.Pin = LL_GPIO_PIN_1;
    gpio_cfg.Mode = LL_GPIO_MODE_INPUT;
    gpio_cfg.Pull = LL_GPIO_PULL_NO;
    gpio_cfg.Speed = LL_GPIO_SPEED_FREQ_LOW;
    err = LL_GPIO_Init(GPIOB,&gpio_cfg);
    return err == SUCCESS;
}
int board_init_spi2_pins()
{
    LL_GPIO_InitTypeDef gpio_cfg = {};
    gpio_cfg.Pin = LL_GPIO_PIN_13 | LL_GPIO_PIN_14 | LL_GPIO_PIN_15;
    gpio_cfg.Mode = LL_GPIO_MODE_ALTERNATE;
    gpio_cfg.Pull = LL_GPIO_PULL_NO;
    gpio_cfg.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
    gpio_cfg.Speed = LL_GPIO_SPEED_FREQ_VERY_HIGH;
    gpio_cfg.Alternate = LL_GPIO_AF_5;
    int err = LL_GPIO_Init(GPIOB,&gpio_cfg);
    gpio_cfg.Pin = LL_GPIO_PIN_12;
    gpio_cfg.Mode = LL_GPIO_MODE_OUTPUT;
    gpio_cfg.Pull = LL_GPIO_PULL_UP;
    return LL_GPIO_Init(GPIOB,&gpio_cfg) == SUCCESS && err == SUCCESS;
}
int board_init_button_input_pins()
{
    LL_GPIO_InitTypeDef gpio_cfg = {};
    gpio_cfg.Pin = LL_GPIO_PIN_0;
    gpio_cfg.Mode = LL_GPIO_MODE_ANALOG;
    gpio_cfg.Pull = LL_GPIO_PULL_NO;
    gpio_cfg.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
    gpio_cfg.Speed = LL_GPIO_SPEED_FREQ_LOW;
    return LL_GPIO_Init(GPIOB,&gpio_cfg) == SUCCESS;
}
