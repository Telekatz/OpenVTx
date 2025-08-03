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


#define FIRST_LINE                  35
#define FIRST_COL                   5

#define OSD_ROWS                    15
#define OSD_ROWS_PAL                OSD_ROWS
#define OSD_ROWS_NTSC               13
#define OSD_COLUMNS                 36
#define LINEBUFFER_OUT_SIZE         64
#define LINEBUFFER_SIZE             75
#define FONT_HEIGHT                 18
#define VIDEO_BUFFER_CHARS          (OSD_ROWS * OSD_COLUMNS)

typedef enum {
  AUTOMATIC,
  INTERNAL,
  EXTERNAL,
  OFF
} syncMode_t;

typedef enum {
    PAL,
    NTSC
} videoMode_t;

typedef enum {
    INTERNAL_SYNC,
    EXTERNAL_SYNC_FOUND,
    EXTERNAL_SYNC,
    EXTERNAL_SYNC_LOST
} syncState_t;

typedef struct {
  uint32_t cr[256][18];
} font_t;

extern font_t *osdFont[];

extern uint8_t screenBuffer[2][VIDEO_BUFFER_CHARS];
extern uint8_t screenBufferActive;
extern uint16_t firstLine;
extern uint8_t firstCol;
extern volatile syncState_t syncState;
extern syncMode_t syncMode;
extern videoMode_t videoMode;
extern uint8_t videoModeLocked;

void OSD_init(void);
void OSD_heartbeat(void);
void OSD_clearScreen(void);
void OSD_print(uint8_t x, uint8_t y, const char *str);
void OSD_writeString(uint8_t *payload, uint8_t size);
void OSD_drawScreen(void);
void testScreen(void);
size_t printNumber(uint8_t x, uint8_t y, unsigned long long n, uint8_t base);
void OSD_update();





