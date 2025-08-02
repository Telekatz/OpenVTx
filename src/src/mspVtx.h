#pragma once

#include <stdint.h>

#define MSP_V1          0x4D
#define MSP_V2          0x58
#define MSP_REQUEST     0x3C
#define MSP_RESPONSE    0x3E

#define MSP_DEBUG                       254  // out message: debug1,debug2,debug3,debug4
#define MSP_SET_OSD_CANVAS              188  // in message:  Set OSD canvas size COLSxROWS
#define MSP_DISPLAYPORT                 182  // out message: External OSD displayport mode

typedef enum
{
  GET_VTX_TABLE_SIZE = 0,
  CHECK_POWER_LEVELS,
  CHECK_BANDS,
  SET_DEFAULTS,
  SEND_EEPROM_WRITE,
  MONITORING,
  MSP_STATE_MAX
} mspState_e;

typedef struct mspPacketV1_s {
  uint8_t sync;
  uint8_t version;
  uint8_t dir;
  uint8_t size;
  uint8_t cmd;
  uint8_t payload[];
} mspPacketV1_t;

typedef struct mspPacketV2_s {
  uint8_t sync;
  uint8_t version;
  uint8_t dir;
  uint8_t flag;
  uint16_t cmd;
  uint16_t size;
  uint8_t payload[];
} mspPacketV2_t;

typedef struct mspPacket_s {
  union {
  mspPacketV1_t v1;
  mspPacketV2_t v2;
  };
}  mspPacket_t;

void mspQueryFlightController(uint32_t time_ms);
void mspBuildPacket(void);
void mspProcessSerial(void);
void mspUpdate(uint32_t now);
void mspReset();
