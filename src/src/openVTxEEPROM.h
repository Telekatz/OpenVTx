#pragma once

#include <stdint.h>

#define versionEEPROM 0x112

uint32_t updateEEPROMtime;

typedef enum
{
  TRAMP,
  SMARTAUDIO,
  MSP,
  //BOOTLOADER,
  VTX_MODE_MAX
} vtxMode_e;

typedef struct
{
    uint16_t version;
    uint16_t currFreq;
    uint8_t channel;
    uint8_t freqMode;
    uint8_t pitmodeInRange;
    uint8_t pitmodeOutRange;
    float currPowerdB;
    uint16_t currPowermW; // Required due to rounding errors when converting between dBm and mW
    uint8_t unlocked;
    vtxMode_e vtxMode;
    uint8_t displayport;
} openVTxEEPROM;

extern openVTxEEPROM myEEPROM;

void updateEEPROM(void);
void defaultEEPROM(void);
void readEEPROM(void);
void writeEEPROM(void);
