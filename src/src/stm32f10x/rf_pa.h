#pragma once

#include "common.h"

#define MSP_PACALTABLE        0x4800
#define MSP_SET_PACALTABLE    0x4801

typedef struct {
    uint8_t dBm;
    uint8_t flag;
    uint16_t value[7];
} paCalibration_t;

extern volatile uint32_t adc_value;
extern volatile uint16_t temperature;
extern ADC_HandleTypeDef hadc1;
extern paCalibration_t paCal[PA_CAL_TABLE_SIZE];

uint16_t bilinearInterpolation(float dB);
void sendPaCalibration(uint8_t idx);
void setPaCalibration(mspPacket_t *packet);
void initPaCalibration(void);
