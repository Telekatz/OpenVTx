#include "targets.h"
#include "common.h"
#include "rf_pa.h"
#include "openVTxEEPROM.h"
#include "helpers.h"
#include <string.h>
#include "EEPROM.h"

volatile uint32_t adc_value = 0;
volatile uint16_t temperature = 0;
ADC_HandleTypeDef hadc1;


void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* hadc)
{
    if (hadc->Instance == ADC1)
    {
        adc_value = HAL_ADC_GetValue(hadc);
        uint32_t Vsense_mV = (adc_value * 3300) / 4095;
        temperature = (((1430 - (int32_t)Vsense_mV) * 10) / 43) + 250;
    }
}


void MX_ADC1_Init(void)
{
  ADC_ChannelConfTypeDef sConfig = {0};

  hadc1.Instance = ADC1;
  hadc1.Init.ScanConvMode = ADC_SCAN_DISABLE;
  hadc1.Init.ContinuousConvMode = DISABLE;
  hadc1.Init.DiscontinuousConvMode = DISABLE;
  hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
  hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
  hadc1.Init.NbrOfConversion = 1;
  if (HAL_ADC_Init(&hadc1) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }

  sConfig.Channel = ADC_CHANNEL_TEMPSENSOR;
  sConfig.Rank = ADC_REGULAR_RANK_1;
  sConfig.SamplingTime = ADC_SAMPLETIME_1CYCLE_5;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    _Error_Handler(__FILE__, __LINE__);
  }

}

void HAL_ADC_MspInit(ADC_HandleTypeDef* hadc)
{
  if(hadc->Instance==ADC1)
  {
    __HAL_RCC_ADC1_CLK_ENABLE();
    HAL_NVIC_SetPriority(ADC1_2_IRQn, 10, 0);
    HAL_NVIC_EnableIRQ(ADC1_2_IRQn);
  }
}

uint16_t bilinearInterpolation(float dB)
{
  uint16_t tempFreq = myEEPROM.currFreq;
  uint8_t i;
  uint8_t calFreqsIndex = ARRAY_SIZE(paCal[0].value) - 2;
  uint8_t calDBmIndex = ARRAY_SIZE(paCal) - 2;;


  if (tempFreq < 5650) tempFreq = 5650;
  if (tempFreq > 5950) tempFreq = 5950;

  for (i = 0; i < (ARRAY_SIZE(paCal[0].value) - 1); i++)
  {
      if (tempFreq < paCal[0].value[i + 1])
      {
          calFreqsIndex = i;
          break;
      }
  }

  for (i = 1; i < (ARRAY_SIZE(paCal) - 1); i++)
  {
      if (dB < paCal[i + 1].dBm)
      {
          calDBmIndex = i;
          break;
      }
  }

  float x = dB;
  float x1 = paCal[calDBmIndex].dBm;
  float x2 = paCal[calDBmIndex + 1].dBm;

  float y = tempFreq;
  float y1 = paCal[0].value[calFreqsIndex];
  float y2 = paCal[0].value[calFreqsIndex + 1];

  float Q11 = paCal[calDBmIndex].value[calFreqsIndex];
  float Q12 = paCal[calDBmIndex].value[calFreqsIndex + 1];
  float Q21 = paCal[calDBmIndex + 1].value[calFreqsIndex];
  float Q22 = paCal[calDBmIndex + 1].value[calFreqsIndex + 1];

  float fxy1 = Q11 * (x2 - x) / (x2 - x1) + Q21 * (x - x1) / (x2 - x1);
  float fxy2 = Q12 * (x2 - x) / (x2 - x1) + Q22 * (x - x1) / (x2 - x1);

  uint16_t fxy = fxy1 * (y2 - y) / (y2 - y1) + fxy2 * (y - y1) / (y2 - y1);

  return fxy;
}

void sendPaCalibration(uint8_t idx) {
  mspPacket_t *packet = (mspPacket_t*)txPacket;    
  uint16_t payloadSize = sizeof(paCalibration_t);
  int i;

  mspCreateHeader();

  packet->v2.cmd = MSP_SET_PACALTABLE;
  packet->v2.size = payloadSize;
  packet->v2.payload[0] = idx;
  packet->v2.payload[1] = paCal[idx].dBm;
  memcpy(&packet->v2.payload[2] ,paCal[idx].value, sizeof(paCalibration_t) - 2);
  
  uint8_t crc = 0;
  for(i = 3; i < MSP_HEADER_SIZE+payloadSize; i++) {
      crc = mspCalcCrc(crc, txPacket[i]);
  }

  packet->v2.payload[payloadSize] = crc;

  mspSendPacket(MSP_HEADER_SIZE+payloadSize+1);

}

void setPaCalibration(mspPacket_t *packet) {
  uint8_t idx = packet->v2.payload[0] & 0x7f;

  if (idx > 0 && idx <= paCal[0].dBm) {
    paCal[idx].dBm = packet->v2.payload[1];
    memcpy(paCal[idx].value, &packet->v2.payload[2], sizeof(paCalibration_t) - 2);
  }
  
  if (packet->v2.payload[0] & 0x80) {
    eeprom_write_paCal(&paCal[idx], idx);
  }

}

void initPaCalibration(void) {
  MX_ADC1_Init();
  for (uint8_t x = 1; x < ARRAY_SIZE(paCal); x++) {
    eeprom_read_paCal(&paCal[x], x);
  }

}
