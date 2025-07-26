#include "gpio.h"
#include "helpers.h"
#include "pwm.h"
#include "stm32f1xx_hal.h"
#include "pins.h"

#define PWM_PERIOD 3000 // Results in ~12kHz PWM

struct pwms {
    TIM_TypeDef *tim;
    uint16_t pin;
    uint16_t af;
    uint32_t ch;
    
};
struct pwms pwm_config[] = {
    {TIM1,  PA8,  0,  TIM_CHANNEL_1},
    {TIM1,  PA9,  0,  TIM_CHANNEL_2},
    {TIM1,  PA10, 0,  TIM_CHANNEL_3},
    {TIM1,  PA11, 0,  TIM_CHANNEL_4},
    {TIM2,  PA0,  0,  TIM_CHANNEL_1},
    {TIM2,  PA1,  0,  TIM_CHANNEL_2},
    {TIM2,  PA2,  0,  TIM_CHANNEL_3},
    {TIM2,  PA3,  0,  TIM_CHANNEL_4},
    {TIM3,  PA6,  0,  TIM_CHANNEL_1},
    {TIM3,  PA7,  0,  TIM_CHANNEL_2},
    {TIM3,  PB5,  1,  TIM_CHANNEL_2},
    {TIM3,  PB0,  0,  TIM_CHANNEL_3},
    {TIM3,  PB1,  0,  TIM_CHANNEL_4},
    {TIM4,  PB6,  0,  TIM_CHANNEL_1},
    {TIM4,  PB7,  0,  TIM_CHANNEL_2},
    {TIM4,  PB8,  0,  TIM_CHANNEL_3},
    {TIM4,  PB9,  0,  TIM_CHANNEL_4}
};

void timer_power_on(uint32_t timer_periph)
{
    
}

gpio_pwm_t pwm_init(uint32_t pin)
{
    TIM_HandleTypeDef htim;

    uint8_t index = 0;
    for (index = 0; index < ARRAY_SIZE(pwm_config); index++) {
        if (pin == pwm_config[index].pin)
            break;
    }
    if (ARRAY_SIZE(pwm_config) <= index)
        return (gpio_pwm_t){.tim = 0, .ch = 0};

    TIM_TypeDef *tim = pwm_config[index].tim;
    uint32_t channel = pwm_config[index].ch;
    uint16_t rempap  = pwm_config[index].af;

    TIM_ClockConfigTypeDef sClockSourceConfig = {0};
    TIM_MasterConfigTypeDef sMasterConfig = {0};
    TIM_OC_InitTypeDef sConfigOC = {0};


    htim.Instance = tim;
    htim.Init.Prescaler = 0;
    htim.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim.Init.Period = PWM_PERIOD;
    htim.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    htim.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
    HAL_TIM_Base_Init(&htim);

    sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
    HAL_TIM_ConfigClockSource(&htim, &sClockSourceConfig);
    HAL_TIM_PWM_Init(&htim);
    
    
    sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
    sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
    HAL_TIMEx_MasterConfigSynchronization(&htim, &sMasterConfig);

    sConfigOC.OCMode = TIM_OCMODE_PWM1;
    sConfigOC.Pulse = 0;
    sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
    sConfigOC.OCFastMode = TIM_OCFAST_ENABLE;
    HAL_TIM_PWM_ConfigChannel(&htim, &sConfigOC, channel);

    switch(rempap){
    case 1:
        __HAL_AFIO_REMAP_TIM3_PARTIAL();
        break;
    default:
        break;
    }
    
    uint32_t gpio_periph = GPIOA_BASE + 0x400 * GPIO2PORT(pin);
    uint32_t gpio_pin = GPIO2BIT(pin);

    GPIO_InitTypeDef GPIO_InitStruct = {0};
  
    GPIO_InitStruct.Pin = gpio_pin;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init((GPIO_TypeDef *)gpio_periph, &GPIO_InitStruct);

    HAL_TIM_PWM_Start(&htim, channel);
    

    return (gpio_pwm_t){.tim = tim, .ch = channel};
}

void pwm_out_write(gpio_pwm_t pwm, uint16_t val)
{
    
    if ((uint32_t)pwm.tim) {
        val = (val <= 0) ? 0 : (PWM_PERIOD < val) ? PWM_PERIOD : (val);
        switch(pwm.ch){
        case TIM_CHANNEL_1:
            pwm.tim->CCR1 = val;
            break;
        case TIM_CHANNEL_2:
            pwm.tim->CCR2 = val;
            break;
        case TIM_CHANNEL_3:
            pwm.tim->CCR3 = val;
            break;
        case TIM_CHANNEL_4:
            pwm.tim->CCR4 = val;
            break;
        default:
            break;
        }
    }
}
