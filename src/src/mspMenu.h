#pragma once

#include <stdint.h>

typedef enum {
    BTN_UP,
    BTN_DOWN,
    BTN_LEFT,
    BTN_RIGHT,
    BTN_MID,
    BTN_ENTER,
    BTN_EXIT,
    BTN_ENTER_VTX,
    BTN_INVALID
} ButtonEvent_e;

typedef enum {
    MSP_DP_HEARTBEAT = 0,         // Release the display after clearing and updating
    MSP_DP_RELEASE = 1,         // Release the display after clearing and updating
    MSP_DP_CLEAR_SCREEN = 2,    // Clear the display
    MSP_DP_WRITE_STRING = 3,    // Write a string at given coordinates
    MSP_DP_DRAW_SCREEN = 4,     // Trigger a screen draw
    MSP_DP_OPTIONS = 5,         // Not used by Betaflight. Reserved by Ardupilot and INAV
    MSP_DP_SYS = 6,             // Display system element displayportSystemElement_e at given coordinates
    MSP_DP_COUNT,
} displayportMspCommand_e;

typedef const void *(*osdPrintFuncPtr)(uint8_t x, uint8_t y, uint8_t idx);
typedef const void *(*osdKeyFuncPtr)(ButtonEvent_e btn, uint8_t idx);

typedef struct {
    const char *text;
    osdPrintFuncPtr printFunc;
    osdKeyFuncPtr keyFunc;
} osdEntry_t;

void mspProcessPacketV1(mspPacket_t *packet);
void msp_menu(void);