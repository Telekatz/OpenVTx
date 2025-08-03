/*
Copyright (c) 2025 Telekatz

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

- Redistributions of source code must retain the above copyright notice,
  this list of conditions and the following disclaimer.
- Redistributions in binary form must reproduce the above copyright notice,
  this list of conditions and the following disclaimer in the documentation
  and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
POSSIBILITY OF SUCH DAMAGE.
*/

#pragma once

#define OSD_VSYNC_PORT                      GPIOB
#define OSD_VSYNC_PIN                       GPIO_PIN_14
#define OSD_VSYNC_IRQ                       EXTI15_10_IRQn

#define OSD_CSYNC_PORT                      GPIOA
#define OSD_CSYNC_PIN                       GPIO_PIN_8
#define OSD_CSYNC_IRQ                       EXTI9_5_IRQn

#define OSD_SPI_A                           SPI2
#define OSD_DMA_SPI_A                       DMA1_Channel5
#define OSD_DMA_SPI_A_IRQ                   DMA1_Channel5_IRQn

#define OSD_SPI_B                           SPI1
#define OSD_DMA_SPI_B                       DMA1_Channel3
#define OSD_DMA_SPI_B_IRQ                   DMA1_Channel3_IRQn

#define OSD_SYNC_OUT_PORT                   GPIOB
#define OSD_SYNC_OUT_PIN                    GPIO_PIN_1
#define OSD_TIM_CCR                         CCR3
#define OSD_TIM_CHANNEL                     3
#define OSD_TIM_IRQ                         TIM1_UP_IRQn

extern TIM_HandleTypeDef htim1;

void OSD_hal_init(void);
uint8_t spi_send_DMA(SPI_TypeDef* Instance, uint8_t *pData, uint16_t Size);

