#include "mspVtx.h"
#include "common.h"
#include "openVTxEEPROM.h"
#include "rtc6705.h"
#include <string.h>
#include "helpers.h"
#include "mspMenu.h"


#if HAS_OSD == 1

#include "OSD.h"

#define OSD_MENU_TOP          2
#define OSD_MENU_TEXT_LEFT    6
#define OSD_MENU_VALUE_LEFT   20

uint8_t tempChannel;
uint8_t tempDisplayport;

void printMenuValue(uint8_t x, uint8_t y, uint8_t idx);
void changeChannel(ButtonEvent_e btn, uint8_t idx);
void changePower(ButtonEvent_e btn, uint8_t idx);
void exitVtxMenu(ButtonEvent_e btn, uint8_t idx);
void exitVtxMenu(ButtonEvent_e btn, uint8_t idx);
void changePit(ButtonEvent_e btn, uint8_t idx);
void changeDisplayport(ButtonEvent_e btn, uint8_t idx);

osdEntry_t osdMenue[] = { {"BAND",        (osdPrintFuncPtr)printMenuValue,    (osdKeyFuncPtr)changeChannel},
                          {"CHANNEL",     (osdPrintFuncPtr)printMenuValue,    (osdKeyFuncPtr)changeChannel},
                          {"FREQUENCY",   (osdPrintFuncPtr)printMenuValue,    NULL},
                          {"POWER",       (osdPrintFuncPtr)printMenuValue,    (osdKeyFuncPtr)changePower},
                          {"PIT MODE",    (osdPrintFuncPtr)printMenuValue,    (osdKeyFuncPtr)changePit},
                          {"DISPLAYPORT", (osdPrintFuncPtr)printMenuValue,    (osdKeyFuncPtr)changeDisplayport},
                          {"EXIT",        NULL,                               (osdKeyFuncPtr)exitVtxMenu},
                          {"SAVE+EXIT",   NULL,                               (osdKeyFuncPtr)exitVtxMenu}};


void printMenuValue(uint8_t x, uint8_t y, uint8_t idx) {
  char buffer[20] = {0};
  uint8_t band = tempChannel / 8;
  uint8_t channel = tempChannel % 8;
  uint8_t db_idx = 0;

  switch (idx) {
    case 0:
      memcpy(buffer, &channelFreqLabel[band * 8], 8);
      break;
    case 1:
      sprintf(buffer, "%1i",channel + 1 );
      break;
    case 2:
      sprintf(buffer, "%i    ", getFreqByIdx(tempChannel));
      break;
    case 3:
      for (db_idx = 0; db_idx < SA_NUM_POWER_LEVELS; db_idx++) {
        if (myEEPROM.currPowerdB == saPowerLevelsLut[db_idx]) break;
      }
      sprintf(buffer, "    MW  ");
      memcpy(buffer, &saPowerLevelsLabel[db_idx * POWER_LEVEL_LABEL_LENGTH], 3);
      break;
    case 4:
      if (pitMode)
        sprintf(buffer, "ON ");
      else
        sprintf(buffer, "OFF");
      break;
    case 5:
      if (tempDisplayport)
        sprintf(buffer, "ON ");
      else
        sprintf(buffer, "OFF");
      break;
    default:
      break;
  }
  OSD_print(x, y, buffer);
}

void changeChannel(ButtonEvent_e btn, uint8_t idx) {
  uint8_t band = tempChannel / 8;
  uint8_t channel = tempChannel % 8;


  switch (idx) {
    case 0:
       if (btn == BTN_RIGHT)
        band = (band + 1) % getFreqTableBands();
      else
        band = (getFreqTableBands() + band - 1) % getFreqTableBands();
      break;
    case 1:
      if (btn == BTN_RIGHT)
        channel = (channel + 1) % 8;
      else
        channel = (8 + channel - 1) % 8;
      break;
    default:
      break;
  }
  tempChannel = band * 8 + channel;
}

void changePower(ButtonEvent_e btn, uint8_t idx) {
  uint8_t db_idx = 0;

  for (db_idx = 0; db_idx < SA_NUM_POWER_LEVELS; db_idx++) {
    if (myEEPROM.currPowerdB == saPowerLevelsLut[db_idx]) break;
  }
  if (btn == BTN_RIGHT)
    db_idx = (db_idx + 1) % SA_NUM_POWER_LEVELS;
  else
    db_idx = (SA_NUM_POWER_LEVELS + db_idx - 1) % SA_NUM_POWER_LEVELS;
  
  setPowerdB(saPowerLevelsLut[db_idx]);
}

void changePit(ButtonEvent_e btn, uint8_t idx) {
  if (pitMode)
    pitMode = 0;
  else
    pitMode = 1;
  TRACE_INFO("pitmode %i\r", pitMode);
  setPowerdB(myEEPROM.currPowerdB);
}

void changeDisplayport(ButtonEvent_e btn, uint8_t idx) {
  if (tempDisplayport) {
    tempDisplayport = 0;
    setSyncMode(INTERNAL);
  } else {
    tempDisplayport = 1;
    setSyncMode(AUTOMATIC);
  }
}

void exitVtxMenu(ButtonEvent_e btn, uint8_t idx) {
  if (btn == BTN_RIGHT) {
    osdState = OSD_EXIT_VTX;
    if (idx == ARRAY_SIZE(osdMenue) - 1) {
      myEEPROM.channel = tempChannel;
      rtc6705WriteFrequency(getFreqByIdx(tempChannel));
      myEEPROM.freqMode = 0;
      updateEEPROMtime = millis();
      if (tempDisplayport && (myEEPROM.displayport != tempDisplayport)) {
        OSD_setCanvas();
      }
      myEEPROM.displayport = tempDisplayport;
    }
  }
}
#endif

void mspProcessPacketV1(mspPacket_t __attribute__((unused)) *packet) {
  
  if (packet->v1.version != MSP_V1) {
    return;
  } 

  switch (packet->v1.cmd) {
    case MSP_DISPLAYPORT:
      #if HAS_OSD == 1
      if (osdState == OSD_MSP && myEEPROM.displayport) {  
        switch (packet->v1.payload[0]) {
          case MSP_DP_HEARTBEAT:    OSD_heartbeat(); break;
          case MSP_DP_CLEAR_SCREEN: OSD_clearScreen(); break;
          case MSP_DP_WRITE_STRING: OSD_writeString(packet->v1.payload, packet->v1.size); break;
          case MSP_DP_DRAW_SCREEN:  OSD_drawScreen(); break;
          default: TRACE_INFO("MSP_DP unkw %02x\r", packet->v1.payload[0]); break;
        }
      }
      #endif
      break;

    default:
      TRACE_INFO("CMDV1 unkw %i\r", packet->v1.cmd);
      break;
  }
}

void msp_menu(void) {
  static ButtonEvent_e btn = BTN_INVALID;
  static ButtonEvent_e btnLast = BTN_INVALID;

  if      (stickPos == 0x20)            btn = BTN_ENTER;
  else if (stickPos == 0x10)            btn = BTN_EXIT;
  else if (stickPos == 0x65)            btn = BTN_ENTER_VTX;
  else if ((stickPos & 0x0f) == 0x00)   btn = BTN_MID;
  else if ((stickPos & 0x0f) == 0x01)   btn = BTN_LEFT;
  else if ((stickPos & 0x0f) == 0x02)   btn = BTN_RIGHT;
  else if ((stickPos & 0x0f) == 0x04)   btn = BTN_DOWN;
  else if ((stickPos & 0x0f) == 0x08)   btn = BTN_UP;
  else                                  btn = BTN_INVALID;

#if HAS_OSD == 1
  static uint8_t selectedEntry = 0;

  if ((osdState == OSD_MSP) && (btn == BTN_ENTER_VTX)) {
    osdState = OSD_VTX;
    selectedEntry = 0;
    btnLast = BTN_INVALID;
    tempChannel = myEEPROM.channel;
    tempDisplayport = myEEPROM.displayport;

    if (!myEEPROM.displayport)
      setSyncMode(INTERNAL);

    OSD_clearScreen();
    for (uint8_t i = 0; i < ARRAY_SIZE(osdMenue); i++) {
      OSD_print(OSD_MENU_TEXT_LEFT, OSD_MENU_TOP + i, osdMenue[i].text);
      if (i == selectedEntry)
        OSD_print(OSD_MENU_TEXT_LEFT - 1, OSD_MENU_TOP + i, ">");
      if (osdMenue[i].printFunc != NULL) {
        osdMenue[i].printFunc(OSD_MENU_VALUE_LEFT ,OSD_MENU_TOP + i, i);
        if (osdMenue[i].keyFunc != NULL) {
          OSD_print(OSD_MENU_VALUE_LEFT - 2, OSD_MENU_TOP + i, "<");
          OSD_print(OSD_MENU_VALUE_LEFT + 9, OSD_MENU_TOP + i, ">");
        }
      }
    }
    OSD_drawScreen();
    TRACE_INFO("osdState = OSD_VTX %i\r", osdState);
  }

  if ((osdState == OSD_VTX && fcArmed) || (osdState == OSD_EXIT_VTX)) {
    TRACE_INFO("osdState = OSD_MSP\r");
    OSD_clearScreen();
    OSD_drawScreen();
    osdState = OSD_MSP;
    if (myEEPROM.displayport)
      setSyncMode(AUTOMATIC);
    else
      setSyncMode(OFF);
  }

  if (osdState != OSD_VTX) {
    btnLast = btn;
    return;
  }

  if (btnLast == BTN_MID && (btn == BTN_LEFT || btn == BTN_RIGHT)) {
    if (osdMenue[selectedEntry].keyFunc != NULL) {
      osdMenue[selectedEntry].keyFunc(btn, selectedEntry);
      for (uint8_t i = 0; i < ARRAY_SIZE(osdMenue); i++) {
        if (osdState == OSD_VTX && osdMenue[i].printFunc != NULL)
          osdMenue[i].printFunc(OSD_MENU_VALUE_LEFT ,OSD_MENU_TOP + i, i);
      }
    }
  }

  if (btnLast == BTN_MID && (btn == BTN_DOWN || btn == BTN_UP)) {
    OSD_print(OSD_MENU_TEXT_LEFT - 1, OSD_MENU_TOP + selectedEntry, " ");
    if (btn == BTN_DOWN)
      selectedEntry = (selectedEntry + 1) % ARRAY_SIZE(osdMenue);
    else
      selectedEntry = (ARRAY_SIZE(osdMenue) + selectedEntry - 1) % ARRAY_SIZE(osdMenue);
    OSD_print(OSD_MENU_TEXT_LEFT - 1, OSD_MENU_TOP + selectedEntry, ">");
  }
#else
  static uint32_t btnPressed = 0;

  if (btn != btnLast) 
    btnPressed = millis();

  if (!fcArmed && (btn == BTN_ENTER_VTX) && (millis() - btnPressed > 2000)) {
    updateEEPROMtime = millis();
    TRACE_INFO("save EEPROM \r");
  }
#endif

  btnLast = btn;
}