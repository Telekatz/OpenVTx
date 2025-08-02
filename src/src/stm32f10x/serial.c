#include "serial.h"
#include "stm32f1xx_hal.h"
#include "atomic.h"
#include "targets.h"
#include "helpers.h"
#include "common.h"
#include "gpio.h"
#include "platform.h"

#define RX_BUFFER_SIZE 128

UART_HandleTypeDef huart;
DMA_HandleTypeDef hdma_usart_rx;

uint8_t inbyte;

uint8_t rx_buffer[RX_BUFFER_SIZE];
static volatile uint8_t rx_head, rx_tail;

struct usartx {
  USART_TypeDef *usart;
  uint32_t pin_rx, pin_tx, rm;
  DMA_Channel_TypeDef *dma_channel;
};

struct usartx usart_config[] = {
  {USART1, PA10, PA9,  0, DMA1_Channel5},
  {USART1, PB7,  PB6,  1, DMA1_Channel5},
  {USART2, PA3,  PA2,  0, DMA1_Channel6},
  {USART3, PB11, PB10, 0, DMA1_Channel3}
};

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *UartHandle) {
  uint8_t next = rx_head;
  if (((next + 1) % sizeof(rx_buffer)) != rx_tail) {
      rx_buffer[next] = inbyte;
      rx_head = (next + 1) % sizeof(rx_buffer);
  }

  ATOMIC_BLOCK() {
      HAL_UART_Receive_IT(&huart, &inbyte, 1);
  }

  return;
}

void HAL_UART_MspInit(UART_HandleTypeDef* uartHandle) {
  if(uartHandle->Instance==USART1)
  {
    __HAL_RCC_USART1_CLK_ENABLE();
    HAL_NVIC_SetPriority(USART1_IRQn, 10, 0);
    HAL_NVIC_EnableIRQ(USART1_IRQn);
  }
  else if(uartHandle->Instance==USART2)
  {
    __HAL_RCC_USART2_CLK_ENABLE();
    HAL_NVIC_SetPriority(USART2_IRQn, 10, 0);
    HAL_NVIC_EnableIRQ(USART2_IRQn);
  }
  else if(uartHandle->Instance==USART3)
  {
    __HAL_RCC_USART3_CLK_ENABLE();
    HAL_NVIC_SetPriority(USART3_IRQn, 10, 0);
    HAL_NVIC_EnableIRQ(USART3_IRQn);
  }
}


void config_uart_GPIO(struct usartx * usart_cfg, uint32_t tx_pin, uint32_t rx_pin) {
  uint8_t halfduplex = (tx_pin == rx_pin);
  uint32_t gpio_periph;
  uint32_t gpio_pin;
  GPIO_InitTypeDef GPIO_InitStruct;

  if(halfduplex) {
    gpio_periph = GPIOA_BASE + 0x400 * GPIO2PORT(tx_pin);
    gpio_pin = GPIO2BIT(tx_pin);

    GPIO_InitStruct.Pin = gpio_pin;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_OD;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init((GPIO_TypeDef *)gpio_periph, &GPIO_InitStruct);
    TRACE_DEBUG("serial_half %i \r",gpio_pin);

  } else {
    gpio_periph = GPIOA_BASE + 0x400 * GPIO2PORT(tx_pin);
    gpio_pin = GPIO2BIT(tx_pin);

    GPIO_InitStruct.Pin = gpio_pin;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init((GPIO_TypeDef *)gpio_periph, &GPIO_InitStruct);

    gpio_periph = GPIOA_BASE + 0x400 * GPIO2PORT(rx_pin);
    gpio_pin = GPIO2BIT(rx_pin);

    GPIO_InitStruct.Pin = gpio_pin;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    HAL_GPIO_Init((GPIO_TypeDef *)gpio_periph, &GPIO_InitStruct);
    TRACE_DEBUG("serial_full %i \r",gpio_pin);

  }

  if(usart_cfg->rm == 1)
    __HAL_AFIO_REMAP_USART1_ENABLE();

  #if USART_USE_DMA == 1
  // DMA USART1 RX Init
  hdma_usart_rx.Instance = usart_cfg->dma_channel;
  hdma_usart_rx.Init.Direction = DMA_PERIPH_TO_MEMORY;
  hdma_usart_rx.Init.PeriphInc = DMA_PINC_DISABLE;
  hdma_usart_rx.Init.MemInc = DMA_MINC_ENABLE;
  hdma_usart_rx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
  hdma_usart_rx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
  hdma_usart_rx.Init.Mode = DMA_CIRCULAR;
  hdma_usart_rx.Init.Priority = DMA_PRIORITY_LOW;
  HAL_DMA_Init(&hdma_usart_rx);

  __HAL_LINKDMA(&huart, hdmarx, hdma_usart_rx);
  #endif

}


static void config_uart(struct usartx * usart_cfg, uint32_t baud, uint8_t halfduplex, uint8_t stopbits) {
  USART_TypeDef *usart_periph = usart_cfg->usart;

  huart.Instance = usart_periph;
  huart.Init.BaudRate = baud;
  huart.Init.WordLength = UART_WORDLENGTH_8B;
  if(stopbits > 1)
      huart.Init.StopBits = UART_STOPBITS_2;
  else
      huart.Init.StopBits = UART_STOPBITS_1;
  huart.Init.Parity = UART_PARITY_NONE;
  huart.Init.Mode = UART_MODE_TX_RX;
  huart.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart.Init.OverSampling = UART_OVERSAMPLING_16;

  if(halfduplex)
    HAL_HalfDuplex_Init(&huart);
  else
    HAL_UART_Init(&huart);

}



void serial_begin(uint32_t baud, uint32_t tx_pin, uint32_t rx_pin, uint8_t stopbits) {
  uint8_t iter, halfduplex = (tx_pin == rx_pin);

  TRACE_DEBUG("serial_begin %i %i %i %i\r", baud, tx_pin, rx_pin, stopbits);

  for (iter = 0; iter < ARRAY_SIZE(usart_config); iter++) { 
    if (usart_config[iter].pin_tx == tx_pin && (halfduplex || usart_config[iter].pin_rx == rx_pin)) {
      config_uart(&usart_config[iter], baud, halfduplex, stopbits);
      config_uart_GPIO(&usart_config[iter], tx_pin, rx_pin);
      #if USART_USE_DMA == 1
      HAL_UART_Receive_DMA(&huart, rx_buffer, RX_BUFFER_SIZE);
      #else
      HAL_UART_Receive_IT(&huart, &inbyte, 1);
      #endif
      break;
    }
  }
}

uint8_t serial_available(void) {
  #if USART_USE_DMA == 1
    rx_head = RX_BUFFER_SIZE - __HAL_DMA_GET_COUNTER(&hdma_usart_rx);
  #endif
  return (uint32_t)(RX_BUFFER_SIZE + rx_head - rx_tail) % RX_BUFFER_SIZE;
}


uint8_t serial_read(void) {
  #if USART_USE_DMA == 1
    rx_head = RX_BUFFER_SIZE - __HAL_DMA_GET_COUNTER(&hdma_usart_rx);
  #endif
  uint8_t data = rx_buffer[rx_tail++];
  rx_tail %= RX_BUFFER_SIZE;
  return data;
}

void serial_write(uint8_t data)
{
  HAL_UART_Transmit(&huart, &data, 1, HAL_MAX_DELAY);
}

void Serial_write_len(uint8_t *data, uint32_t size)
{
  HAL_UART_Transmit(&huart, data, size, HAL_MAX_DELAY);
}

void serial_flush(void)
{
  // not needed...
}



