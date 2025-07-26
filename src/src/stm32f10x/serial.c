#include "serial.h"
#include "stm32f1xx_hal.h"
#include "atomic.h"
#include "targets.h"


UART_HandleTypeDef huart;
uint8_t inbyte;


static volatile uint8_t rx_buffer[64];
static volatile uint8_t rx_head, rx_tail;


void HAL_UART_MspInit(UART_HandleTypeDef* uartHandle)
{

  GPIO_InitTypeDef GPIO_InitStruct;
  if(uartHandle->Instance==USART1)
  {
    /* Peripheral clock enable */
    __HAL_RCC_USART1_CLK_ENABLE();
  
    /**USART1 GPIO Configuration    
    PA9     ------> USART1_TX
    PA10     ------> USART1_RX 
    */
    GPIO_InitStruct.Pin = GPIO_PIN_9;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_10;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    /* Peripheral interrupt init */
    HAL_NVIC_SetPriority(USART1_IRQn, 10, 0);
    HAL_NVIC_EnableIRQ(USART1_IRQn);
  /* USER CODE BEGIN USART1_MspInit 1 */

  }
  else if(uartHandle->Instance==USART2)
  {
    /* Peripheral clock enable */
    __HAL_RCC_USART2_CLK_ENABLE();
  
    /**USART2 GPIO Configuration    
    PA2     ------> USART2_TX
    PA3     ------> USART2_RX 
    */
    GPIO_InitStruct.Pin = GPIO_PIN_2;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_3;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    /* Peripheral interrupt init */
    HAL_NVIC_SetPriority(USART2_IRQn, 1, 0);
    HAL_NVIC_EnableIRQ(USART2_IRQn);
  }
  else if(uartHandle->Instance==USART3)
  {
    /* Peripheral clock enable */
    __HAL_RCC_USART3_CLK_ENABLE();
  
    /**USART3 GPIO Configuration    
    PB10     ------> USART3_TX
    PB11     ------> USART3_RX 
    */
    GPIO_InitStruct.Pin = GPIO_PIN_10;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_11;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    /* Peripheral interrupt init */
    HAL_NVIC_SetPriority(USART3_IRQn, 1, 0);
    HAL_NVIC_EnableIRQ(USART3_IRQn);
  }
}


void HAL_UART_RxCpltCallback(UART_HandleTypeDef *UartHandle)
{

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


void serial_begin(uint32_t baud, uint32_t tx_pin, uint32_t rx_pin, uint8_t stopbits)
{
    huart.Instance = USART1;
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

    HAL_UART_Init(&huart);

    HAL_UART_Receive_IT(&huart, &inbyte, 1);
}

uint8_t serial_available(void)
{
    return (uint32_t)(sizeof(rx_buffer) + rx_head - rx_tail) % sizeof(rx_buffer);
}

uint8_t serial_read(void)
{
    uint8_t data = rx_buffer[rx_tail++];
    rx_tail %= sizeof(rx_buffer);
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



