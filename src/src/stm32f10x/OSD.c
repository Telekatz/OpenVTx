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
#include "common.h"

#include "OSD_font_default.h"
#include "OSD_font_betaflight.h"
#include "OSD_font_extra_large.h"
#include "OSD_font_impact.h"

#define ENABLE_VSYNC_IRQ()      HAL_NVIC_EnableIRQ(OSD_VSYNC_IRQ)
#define DISABLE_VSYNC_IRQ()     HAL_NVIC_DisableIRQ(OSD_VSYNC_IRQ)

uint8_t screenBuffer[2][VIDEO_BUFFER_CHARS];
uint8_t screenBufferActive = 0;
uint8_t screenBufferDraw = 0;
uint16_t firstLine = FIRST_LINE;
uint8_t firstCol = FIRST_COL;
volatile syncState_t syncState = INTERNAL_SYNC;
syncMode_t syncMode = AUTOMATIC;
uint8_t canvasSet = 0;
uint8_t heartbeat = 0;

font_t *osdFont[] = { (font_t*)OSD_font_impact,
                      (font_t*)OSD_font_extra_large};

void setSyncMode(syncMode_t mode) {

  switch(mode) {
    case AUTOMATIC:
      syncMode = AUTOMATIC;
      ENABLE_VSYNC_IRQ();
      syncState = INTERNAL_SYNC;
      break;
    case EXTERNAL:
      ENABLE_VSYNC_IRQ();
      TIM1->OSD_TIM_CCR = 0;
      syncState = EXTERNAL_SYNC;
      syncMode = EXTERNAL;
      break;
    case INTERNAL:
      DISABLE_VSYNC_IRQ();
      syncState = INTERNAL_SYNC;
      syncMode = INTERNAL;
      break;
    case OFF:
      DISABLE_VSYNC_IRQ();
      TIM1->OSD_TIM_CCR = 0;
      syncState = INTERNAL_SYNC;
      syncMode = OFF;
      break;
  }

}

void OSD_heartbeat(void) {
  heartbeat = 30;
}

void OSD_clearScreen(void) {
  screenBufferDraw = 1 - screenBufferActive;
  for(uint16_t x = 0; x < VIDEO_BUFFER_CHARS; x++ ) {
    screenBuffer[screenBufferDraw][x] = 0x20;
  }
}

void OSD_print(uint8_t x, uint8_t y, const char *str) {
  uint16_t pos = x + y * OSD_COLUMNS;

  while (*str) {
    screenBuffer[screenBufferDraw][pos++] = *str++;
  }
  OSD_clearScreen();
}

void OSD_writeString(uint8_t *payload, uint8_t size) {
  uint16_t pos = payload[2] + payload[1] * OSD_COLUMNS;

  for (uint8_t x=4; x< size; x++) {
    screenBuffer[screenBufferDraw][pos++] = payload[x];
  }
}

void OSD_drawScreen(void) {
  screenBufferActive = screenBufferDraw;
}

void print(uint8_t x, uint8_t y, uint8_t c) {
  uint16_t pos = x + y * OSD_COLUMNS;

  screenBuffer[1-screenBufferActive][pos++] = c;
}

void testScreen(void) {
  for(uint16_t x = 0; x < VIDEO_BUFFER_CHARS; x++ ) {
    screenBuffer[1-screenBufferActive][x] = x;//'0' + (x % 10);
  }
}


void OSD_setCanvas() {
    uint16_t payloadSize = 2;

    mspCreateHeader();

    txPacket[4] = MSP_SET_OSD_CANVAS & 0xFF;
    txPacket[5] = (MSP_SET_OSD_CANVAS >> 8) & 0xFF;
    
    txPacket[6] = payloadSize & 0xFF;
    txPacket[7] = (payloadSize >> 8) & 0xFF;
    
    txPacket[8] = OSD_COLUMNS; // col
    if (videoMode == PAL) 
      txPacket[9] = OSD_ROWS_PAL; // col
    else
      txPacket[9] = OSD_ROWS_NTSC; // col
    
    uint8_t crc = 0;
    for(int i = 3; i < MSP_HEADER_SIZE+payloadSize; i++)
    {
        crc = mspCalcCrc(crc, txPacket[i]);
    }

    txPacket[MSP_HEADER_SIZE+payloadSize] = crc;
    
    mspSendPacket(MSP_HEADER_SIZE+payloadSize+1);
}

void OSD_update() {
  static uint32_t lastTick=0;
  if (mspState == MONITORING) {
    if (videoModeLocked && !canvasSet) {
      OSD_setCanvas();
      canvasSet = 1;
      
      TRACE_INFO("MSP send canvas size\r");
    }
    if (syncMode == OFF)
      setSyncMode(AUTOMATIC);
  }
  

  if((HAL_GetTick() - lastTick) > 100) {
    lastTick = HAL_GetTick();
    if (heartbeat == 1) {
      OSD_clearScreen();
      OSD_drawScreen();
      OSD_clearScreen();
    }else if (heartbeat > 1) {
      heartbeat--;
    }
  }
}


void OSD_init(void) {

  OSD_hal_init();

  setSyncMode(OFF);

  OSD_clearScreen();
  //#ifdef TRACE_LEVEL
  TRACE_CMD(testScreen(););
  //#endif
  OSD_drawScreen();

}


