#include "targets.h"
#include "common.h"
#include "openVTxEEPROM.h"
#include "gpio.h"
#include "helpers.h"
#include <math.h>
#include "mspVtx.h"
#include "mspMenu.h"
#include "rtc6705.h"
#include "rf_pa.h"
#if HAS_OSD == 1
#include "OSD.h"
#endif
#include <string.h>

#define OUTPUT_POWER_INTERVAL   5 // ms
#define MSP_VTXSTATUS           0x4802

/* SA2.1 powerlevels in dBm.
 *
 * INav:
 *    Max of 5 [https://github.com/iNavFlight/inav/blob/a8016edd0d6f05bb12a75b0ea75a3483772baaeb/src/main/io/vtx_smartaudio.h#L36]
 *    Index 0 is ignored [https://github.com/iNavFlight/inav/blob/a8016edd0d6f05bb12a75b0ea75a3483772baaeb/src/main/io/vtx_smartaudio.c#L334]
 *
 */
uint8_t saPowerLevelsLut[SA_NUM_POWER_LEVELS] = {1, 14, 17, 20};

uint8_t saPowerLevelsLabel[SA_NUM_POWER_LEVELS * POWER_LEVEL_LABEL_LENGTH] = {'1', ' ', ' ',
                                                                              '2', '5', ' ',
                                                                              '5', '0', ' ',
                                                                              '1', '0', '0'};

gpio_out_t rtcen_pin;
gpio_out_t bias2_pin;
gpio_pwm_t outputPower_pin;

paCalibration_t paCal[PA_CAL_TABLE_SIZE] = {{5,  0, { 5650, 5700, 5750, 5800, 5850, 5900, 5950 }},    //frequency
                                            {1,  0, { 1799, 1820, 1841, 1863, 1885, 1909, 1935 }},    // 1mW
                                            {10, 0, { 1866, 1886, 1907, 1930, 1954, 1981, 2011 }},    // 10 mW
                                            {14, 0, { 1909, 1929, 1952, 1975, 2002, 2033, 2070 }},    // 25 mW
                                            {17, 0, { 1958, 1980, 2004, 2030, 2062, 2100, 2147 }},    // 50 mW
                                            {20, 0, { 2031, 2056, 2085, 2121, 2165, 2222, 2301 }} };  // 100 mW

#if USE_CUSTOM_FREQ_TABLE == 1
const uint8_t channelFreqLabel[32] = {
    'B', 'A', 'N', 'D', '_', 'A', ' ', ' ', // A
    'B', 'A', 'N', 'D', '_', 'B', ' ', ' ', // B
    'F', 'A', 'T', 'S', 'H', 'A', 'R', 'K', // F
    'R', 'A', 'C', 'E', ' ', ' ', ' ', ' '  // R
};

const uint8_t bandLetter[4] = {'A', 'B', 'F', 'R'};

uint16_t channelFreqTable[FREQ_TABLE_SIZE] = {
    5865, 5845, 5825, 5805, 5785, 5765, 5745,    0, // A
    5733, 5752, 5771, 5790, 5809, 5828, 5847, 5866, // B
    5740, 5760, 5780, 5800, 5820, 5840, 5860,    0, // F
       0,    0, 5732, 5769, 5806, 5843,    0,    0, // R
};
#endif

void target_rfPowerAmpPinSetup(void)
{
  TRACE_INFO("target_rfPowerAmpPinSetup\r");
  outputPower_pin = pwm_init(RTC_BIAS);
  rtcen_pin = gpio_out_setup(RTC_ENABLE, 0);
  delay(500);
}

void target_set_power_dB(float dB)
{
  uint16_t paValue = bilinearInterpolation(dB);
  TRACE_INFO("target_set_power_dB %i (%i) \r", (int)(dB + 0.5), paValue);
  #ifdef PA_LIMIT
  if(paValue > PA_LIMIT) 
    paValue = PA_LIMIT;
  #endif
  pwm_out_write(outputPower_pin,paValue);
}

void sendVtxStatus() {
  mspPacket_t *packet = (mspPacket_t*)txPacket;    
  uint16_t payloadSize = 2;
  int i;

  mspCreateHeader();

  packet->v2.cmd = MSP_VTXSTATUS;
  packet->v2.size = payloadSize;
  packet->v2.payload[0] = temperature & 0xff;
  packet->v2.payload[1] = temperature >> 8;
  
  uint8_t crc = 0;
  for(i = 3; i < MSP_HEADER_SIZE+payloadSize; i++) {
      crc = mspCalcCrc(crc, txPacket[i]);
  }

  packet->v2.payload[payloadSize] = crc;

  mspSendPacket(MSP_HEADER_SIZE+payloadSize+1);

}

void target_mspProcessPacket(mspPacket_t __attribute__((unused)) *packet)
{
  uint16_t debug0;
  uint16_t debug1;

  switch (packet->v1.version) {

    case MSP_V1:
      break;

    case MSP_V2:
      switch (packet->v2.cmd) {
        case MSP_DEBUG:
          debug0 = ((uint16_t)packet->v2.payload[1] << 8) + packet->v2.payload[0];
          debug1 = ((uint16_t)packet->v2.payload[3] << 8) + packet->v2.payload[2];
          TRACE_INFO("target_debug %04x %04x\r", debug0, debug1);
          switch (debug1) {
            case 0:
              pwm_out_write(outputPower_pin,debug0);
              break;
            case 1:
              TRACE_INFO("myEEPROM.version %i\r", myEEPROM.version);
              TRACE_INFO("myEEPROM.vtxMode %i\r", myEEPROM.vtxMode);
              TRACE_INFO("myEEPROM.currFreq %i\r", myEEPROM.currFreq);
              TRACE_INFO("myEEPROM.channel %i\r", myEEPROM.channel);
              TRACE_INFO("myEEPROM.freqMode %i\r", myEEPROM.freqMode);
              TRACE_INFO("myEEPROM.pitmodeInRange %i\r", myEEPROM.pitmodeInRange);
              TRACE_INFO("myEEPROM.pitmodeOutRange %i\r", myEEPROM.pitmodeOutRange);
              TRACE_INFO("myEEPROM.currPowerdB %f\r", myEEPROM.currPowerdB);
              TRACE_INFO("myEEPROM.currPowermW %i\r", myEEPROM.currPowermW);
              TRACE_INFO("myEEPROM.unlocked %i\r", myEEPROM.unlocked);
              break;
            default:
              break;
          }
          break;
        case MSP_PACALTABLE:
          sendPaCalibration(packet->v2.payload[0]);
          break;
        case MSP_SET_PACALTABLE:
          setPaCalibration(packet);
          break;
        case MSP_VTXSTATUS:
          sendVtxStatus();
          break;
        default:
          TRACE_INFO("CMDV2 unkw %i\r", packet->v2.cmd);
          break;
        } //switch (packet->v2.cmd)
    default:
        break;
  } // switch (packet->v1.version)
}

void target_setup(void)
{
  TRACE_INFO("target_setup\r");
  initPaCalibration();
  target_rfPowerAmpPinSetup();
#if HAS_OSD == 1
  OSD_init();
#endif
}


void target_loop(void)
{
	static uint32_t lastTick=0;
  static uint32_t loops = 0;
  
  if ((HAL_GetTick() - lastTick) >= 1000) {
    lastTick = HAL_GetTick();
    measureTemperature();
    //TRACE_INFO("Loops %i; Temp: %i\r", loops, temperature );
    //TRACE_INFO("osdState %i\r", osdState);
    loops = 0;
  }

#if HAS_OSD == 1
  OSD_update();
#endif
  msp_menu();

  loops +=1;
}

