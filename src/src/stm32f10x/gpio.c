#include "gpio.h"
#include "stm32f1xx_hal.h"


gpio_out_t gpio_out_setup(uint32_t pin, uint32_t val)
{
    uint32_t gpio_periph = GPIOA_BASE + 0x400 * GPIO2PORT(pin);
    uint32_t gpio_pin = GPIO2BIT(pin);

    GPIO_InitTypeDef GPIO_InitStruct;
  
    GPIO_InitStruct.Pin = gpio_pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init((GPIO_TypeDef *)gpio_periph, &GPIO_InitStruct);

    gpio_out_t g = (gpio_out_t){.regs = gpio_periph, .bit = gpio_pin};
    gpio_out_write(g, val);
    return g;
}

void gpio_out_toggle(gpio_out_t g)
{
   HAL_GPIO_TogglePin((GPIO_TypeDef *)(g.regs), g.bit);
}

void gpio_out_write(gpio_out_t g, uint32_t val)
{
    if (val)
        HAL_GPIO_WritePin((GPIO_TypeDef *)(g.regs), g.bit, GPIO_PIN_SET);
    else
        HAL_GPIO_WritePin((GPIO_TypeDef *)(g.regs), g.bit, GPIO_PIN_RESET);
}


gpio_in_t gpio_in_setup(uint32_t pin, int32_t pull_up)
{
    uint32_t gpio_periph = GPIOA_BASE + 0x400 * GPIO2PORT(pin);
    uint32_t gpio_pin = GPIO2BIT(pin);
    /* enable the clock */
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();

    GPIO_InitTypeDef GPIO_InitStruct;
  
    GPIO_InitStruct.Pin = gpio_pin;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = pull_up;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init((GPIO_TypeDef *)gpio_periph, &GPIO_InitStruct);

    gpio_in_t g = (gpio_in_t){.regs = gpio_periph, .bit = gpio_pin};
    return g;
}

uint8_t gpio_in_read(gpio_in_t g)
{
    return !!(HAL_GPIO_ReadPin((GPIO_TypeDef *)(g.regs), g.bit));
}


