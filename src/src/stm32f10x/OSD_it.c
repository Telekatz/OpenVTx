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

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "atomic.h"

#include <stm32f1xx_hal.h>
#include <stm32f1xx_hal_dma.h>
#include <stm32f103xb.h>

#include "OSD.h"
#include "OSD_hal.h"
#include "OSD_it.h"
#include "common.h"

#include "gpio.h"

#define LOSTSYNC_COUNTER            20
#define SYNC_COUNTER                2
#define SYNC_QUALITY_THRESHOLD      80

//PAL: HSync_Pules:4,7us VSync_Pulse:27,3us Line:64us
#define PAL_HSYNC_PULSE         337
#define PAL_HSYNC_PULSE_HALF    168
#define PAL_VSYNC_PULSE         1965
#define PAL_LINE_DURATION       4607
#define PAL_LINE_DURATION_HALF  2303

//NTSC: HSync_Pules:4,7us VSync_Pulse:27,1us Line:63,5us
#define NTSC_HSYNC_PULSE        337
#define NTSC_HSYNC_PULSE_HALF   165
#define NTSC_VSYNC_PULSE        1950
#define NTSC_LINE_DURATION      4571
#define NTSC_LINE_DURATION_HALF 2285


volatile uint16_t line = 0;
volatile uint16_t lastLine = 240;
volatile uint16_t lineCounter = 0;
volatile uint16_t internalLine = 0;

volatile uint8_t ntscSync = 4;
volatile uint8_t palSync = 4;
volatile uint8_t lostSync = LOSTSYNC_COUNTER;

uint8_t linebufferA[2][LINEBUFFER_SIZE] = {0};
uint8_t linebufferB[2][LINEBUFFER_SIZE] = {0};

uint8_t syncQuality = 0;
uint16_t hSyncFail = 0;
videoMode_t videoMode = PAL;
uint8_t videoModeLocked = 0;

extern const uint32_t OSD_font_extra_large[];

volatile uint32_t ccrDebug = 0;
extern gpio_out_t debug_pin;

typedef struct _uint32u16u8_t {
    union {
    uint32_t dbword;
    uint16_t word[2];
    uint8_t byte[4];
    };

} uint32u16u8_t;

#if HAS_OSD == 1
void vsync_callback(void) {
  static uint32_t oldTick = 0;
  uint32_t actTick = HAL_GetTick();
  uint32_t difTick = actTick - oldTick;

  if(syncState == INTERNAL_SYNC) {
    if (difTick > 15 && difTick < 18) {
      if(ntscSync) {
        ntscSync--;
      }
    } else {
      if(ntscSync < SYNC_COUNTER) {
        ntscSync++;
      }
    }

    if (difTick > 18 && difTick < 22) {
      if(palSync) {
        palSync--;
      }
    } else {
      if(palSync < SYNC_COUNTER) {
        palSync++;
      }
    }
  }

  oldTick = actTick;

  if(hSyncFail < 100)
    syncQuality = 100 - hSyncFail;
  else
    syncQuality = 0;

  if(!ntscSync && (videoMode == NTSC || !videoModeLocked)) {
    if(difTick > 14) {
      line = 1;
      internalLine = 1;
      TIM1->CNT = 0;
      hSyncFail = 0;
      SPI1->CR1 |= SPI_CR1_SPE;
      if(lostSync < LOSTSYNC_COUNTER) {
        lostSync++;
      }
    }
    if(syncState == INTERNAL_SYNC) {
      videoMode = NTSC;
      videoModeLocked = 1;
      syncState = EXTERNAL_SYNC_FOUND;
    }
  }

  if(!palSync && (videoMode == PAL || !videoModeLocked)) {
    if(difTick > 18) {
      line = 1;
      internalLine = 1;
      TIM1->CNT = 0;
      hSyncFail = 0;
      SPI1->CR1 |= SPI_CR1_SPE;
      if(lostSync < LOSTSYNC_COUNTER) {
        lostSync++;
      }
    }
    if(syncState == INTERNAL_SYNC) {
      videoMode = PAL;
      videoModeLocked = 1;
      syncState = EXTERNAL_SYNC_FOUND;
    }
  }

  if(syncQuality < SYNC_QUALITY_THRESHOLD)
    SPI1->CR1 &= ~(SPI_CR1_SPE);

}

void  __attribute__((optimize("Ofast"))) csync_callback(void) {
  static uint8_t cBuffer = 0;
  static uint32_t syncTick = 0;
  static uint32_t lastSyncTick = 0;
  static uint32_t syncDiff;

  syncTick = internalLine * (TIM1->ARR + 1) + TIM1->CNT;
  syncDiff = syncTick - lastSyncTick;

  if( line > 10 && line < lastLine && syncDiff < 4000) {
    hSyncFail++;
    return;
  }

  if((line >= firstLine ) && (line <= lastLine) && syncQuality >= SYNC_QUALITY_THRESHOLD) {

    DMA_Channel_TypeDef* dmaSpiA = OSD_DMA_SPI_A;
    DMA_Channel_TypeDef* dmaSpiB = OSD_DMA_SPI_B;
    uint32_t ccr;

    ATOMIC_BLOCK() { 
      dmaSpiB->CCR = 0;
      dmaSpiA->CCR = 0;
      
      dmaSpiB->CMAR = (uint32_t)linebufferB[cBuffer];
      dmaSpiA->CMAR = (uint32_t)linebufferA[cBuffer];
      
      dmaSpiB->CNDTR = LINEBUFFER_OUT_SIZE;
      dmaSpiA->CNDTR = LINEBUFFER_OUT_SIZE;
      
      ccr = DMA_CCR_EN | DMA_CCR_DIR | DMA_CCR_MINC | DMA_CCR_PL;

      dmaSpiA->CCR = ccr;
      dmaSpiB->CCR = ccr;
      ccrDebug = ccr;
    }
    
    cBuffer = 1 - cBuffer;

    if( line < lastLine) {
      uint16_t fontRow = ((line - firstLine) % FONT_HEIGHT);
      uint32u16u8_t ch1 = {0};
      uint32u16u8_t ch2 = {0};
      uint16_t row = (line - firstLine) / FONT_HEIGHT;
      uint16_t buf = firstCol;
      uint16_t screenBufferCounter = row * OSD_COLUMNS;
      uint8_t char1 = 0;
      uint8_t char2 = 0;

      if(syncState == EXTERNAL_SYNC) {
        for(unsigned col=0; col<OSD_COLUMNS; col +=2) {
          char1 = screenBuffer[screenBufferActive][screenBufferCounter++];
          char2 = screenBuffer[screenBufferActive][screenBufferCounter++];

          ch1.dbword = osdFont[0]->cr[char1][fontRow]; 

          if(col < OSD_COLUMNS-1) {
            ch2.dbword = (osdFont[0]->cr[char2][fontRow])>>4;
          } else {
            ch2.dbword = 0x0000;
          }

          linebufferA[cBuffer][buf]   = ch1.byte[3];
          linebufferA[cBuffer][buf+1] = ch1.byte[2] | ch2.byte[3];
          linebufferA[cBuffer][buf+2] = ch2.byte[2];

          linebufferB[cBuffer][buf]   = ch1.byte[1];
          linebufferB[cBuffer][buf+1] = ch1.byte[0] | ch2.byte[1];
          linebufferB[cBuffer][buf+2] = ch2.byte[0];
          
          buf +=3;
        }
      } else {
        buf += 4;
        for(unsigned col=0; col<OSD_COLUMNS; col +=2) {
          char1 = screenBuffer[screenBufferActive][screenBufferCounter++];
          char2 = screenBuffer[screenBufferActive][screenBufferCounter++];

          //ch1.dbword = fontdata[char1 * (FONT_HEIGHT) + fontRow];
          ch1.dbword = osdFont[0]->cr[char1][fontRow];

          //ch2.dbword = (fontdata[char2 * (FONT_HEIGHT) + fontRow])>>4;
          ch2.dbword = (osdFont[0]->cr[char2][fontRow])>>4;

          linebufferB[cBuffer][buf]   = ch1.byte[1];
          linebufferB[cBuffer][buf+1] = ch1.byte[0] | ch2.byte[1];
          linebufferB[cBuffer][buf+2] = ch2.byte[0];
          buf +=3;
        }
      }

      lineCounter++;

    } else if (line == lastLine) {
      uint8_t buf = LINEBUFFER_SIZE;
      uint8_t alpha = 0xff;

      if(syncState == EXTERNAL_SYNC ) {
        alpha=0x00;
      }
      while(buf--) {
        linebufferA[0][buf] = alpha;
        linebufferB[0][buf] = 0;
        linebufferA[1][buf] = alpha;
        linebufferB[1][buf] = 0;
      }

      lineCounter = 0;
    }
  }

  line += 1;

  lastSyncTick = syncTick;

  if(line == 320) {
    SPI1->CR1 &= ~(SPI_CR1_SPE);
  }

  if(line == 322) {
    SPI1->CR1 |= SPI_CR1_SPE;
  }

}


void __attribute__((optimize("O3"))) TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) {
  DMA_Channel_TypeDef* dmaSpiA = OSD_DMA_SPI_A;
  uint8_t buf = LINEBUFFER_SIZE;

  switch(syncState){
    case INTERNAL_SYNC:
      syncQuality = 100;
      break;

    case EXTERNAL_SYNC_FOUND:
      HAL_NVIC_EnableIRQ(OSD_CSYNC_IRQ);

      TIM1->OSD_TIM_CCR = 0;
      if(videoMode == PAL) {
          TIM1->ARR = PAL_LINE_DURATION;
          lastLine = firstLine + (OSD_ROWS_PAL * FONT_HEIGHT) + 1;
      } else {
          TIM1->ARR = NTSC_LINE_DURATION;
          lastLine = firstLine + (OSD_ROWS_NTSC * FONT_HEIGHT) + 1;
      }
      internalLine = line;
      syncState = EXTERNAL_SYNC;
      lostSync = LOSTSYNC_COUNTER;
      return;

    case EXTERNAL_SYNC:
      internalLine++;
      if(internalLine == 7) {
          internalLine += 5 + (uint8_t)videoMode;
      } else if(internalLine == 50) {
          if((line < 47) || (line > 52)) {
              lostSync--;
          }
      } else if(internalLine == 100) {
          if((line < 97) || (line > 102)) {
              lostSync--;
          }
      } else if(internalLine == 150) {
          //debug4 = line;
          if((line < 147) || (line > 152)) {
              lostSync--;
          }
      } else if ((videoMode == PAL) && (internalLine > 323)){
          internalLine = 1;
      } else if ((videoMode == NTSC) && (internalLine > 272)){
          internalLine = 1;
      }
      if(!lostSync) {
          syncState = EXTERNAL_SYNC_LOST;
      }
      return;

    case EXTERNAL_SYNC_LOST:
      HAL_NVIC_DisableIRQ(OSD_CSYNC_IRQ);

      while(buf--) {
          linebufferA[0][buf] = 0x00;
      }
      dmaSpiA->CCR = 0;
      dmaSpiA->CNDTR = 1;
      dmaSpiA->CMAR = (uint32_t)linebufferA[0];
      dmaSpiA->CCR |= DMA_CCR_EN;
      
      syncState = INTERNAL_SYNC;
      palSync = SYNC_COUNTER;
      ntscSync = SYNC_COUNTER;
      return;
  }

  if(syncMode >= EXTERNAL) return;

  if(videoMode == PAL) {
    internalLine++;
    if(internalLine == 322) {
      internalLine = 1;
    }

    csync_callback();

    switch(internalLine) {
      case 316: TIM1->ARR = PAL_LINE_DURATION_HALF; TIM1->OSD_TIM_CCR = PAL_HSYNC_PULSE_HALF; break;
      case 320: TIM1->OSD_TIM_CCR = PAL_VSYNC_PULSE; break;
      case 1:   line = 1; break;
      case 5:   TIM1->OSD_TIM_CCR = PAL_HSYNC_PULSE_HALF; break;
      case 10:  TIM1->OSD_TIM_CCR = PAL_HSYNC_PULSE; break;
      case 11:  TIM1->ARR = PAL_LINE_DURATION; break;
    }
  } else {

    internalLine++;
    if(internalLine == 273) {
      internalLine = 1;
    }

    csync_callback();

    switch(internalLine) {
      case 265: TIM1->OSD_TIM_CCR = NTSC_HSYNC_PULSE_HALF; break;
      case 266: TIM1->ARR = NTSC_LINE_DURATION_HALF; break;
      case 271: TIM1->OSD_TIM_CCR = NTSC_VSYNC_PULSE; break;
      case 1:   line = 1; break;
      case 5:   TIM1->OSD_TIM_CCR = NTSC_HSYNC_PULSE_HALF; break;
      case 11:  TIM1->OSD_TIM_CCR = NTSC_HSYNC_PULSE; break;
      case 12:  TIM1->ARR = NTSC_LINE_DURATION; break;
    }
  }

}


void __attribute__((optimize("Ofast"))) TIM1_UP_IRQHandler(void)
{
  TIM_HandleTypeDef *htim = &htim1;
  /* TIM Update event */
  if(__HAL_TIM_GET_FLAG(htim, TIM_FLAG_UPDATE) != RESET)
  {
    if(__HAL_TIM_GET_IT_SOURCE(htim, TIM_IT_UPDATE) !=RESET)
    {
      TIM_PeriodElapsedCallback(htim);
      __HAL_TIM_CLEAR_IT(htim, TIM_IT_UPDATE);
    }
  } else {
      HAL_TIM_IRQHandler(&htim1);
  }
}

void EXTI4_IRQHandler(void)
{
    __HAL_GPIO_EXTI_CLEAR_IT(OSD_CSYNC_PIN);
    csync_callback();
}

void EXTI9_5_IRQHandler(void)
{
    __HAL_GPIO_EXTI_CLEAR_IT(OSD_CSYNC_PIN);
    csync_callback();
}

void EXTI15_10_IRQHandler(void)
{
    __HAL_GPIO_EXTI_CLEAR_IT(OSD_VSYNC_PIN);
    vsync_callback();
}

#endif

