#include "targets.h"
#include "common.h"
#include "openVTxEEPROM.h"
//#include "rtc6705.h"
#include "gpio.h"
#include "trace.h"
#include "helpers.h"
#include <math.h>

#define OUTPUT_POWER_INTERVAL 5 // ms

#define MSP_DEBUG                       254  // out message: debug1,debug2,debug3,debug4

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


#define CAL_FREQ_SIZE 7
#define CAL_DBM_SIZE 6
uint8_t calDBm[CAL_DBM_SIZE] = {1, 10, 14, 17, 20, 21};
uint16_t calFreqs[CAL_FREQ_SIZE] =  { 5650, 5700, 5750, 5800, 5850, 5900, 5950};
uint16_t calVpd[CAL_DBM_SIZE][CAL_FREQ_SIZE] = {                                        
    { 1799, 1820, 1841, 1863, 1885, 1909, 1935 }, // 1mW
    { 1866, 1886, 1907, 1930, 1954, 1981, 2011 }, // 10 mW
    { 1909, 1929, 1952, 1975, 2002, 2033, 2070 }, // 25 mW
    { 1958, 1980, 2004, 2030, 2062, 2100, 2147 }, // 50 mW
    { 2031, 2056, 2085, 2121, 2165, 2222, 2301 }, // 100 mW
    { 2031, 2056, 2085, 2121, 2165, 2222, 2301 }
};

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

uint16_t bilinearInterpolation(float dB)
{
  uint16_t tempFreq = myEEPROM.currFreq;
  uint8_t i;
  uint8_t calFreqsIndex = 0;
  uint8_t calDBmIndex = 0;

  dB = dB + OFFSET;

  if (tempFreq < 5650) tempFreq = 5650;
  if (tempFreq > 5950) tempFreq = 5950;

  for (i = 0; i < (ARRAY_SIZE(calFreqs) - 1); i++)
  {
    if (tempFreq < calFreqs[i + 1])
    {
      calFreqsIndex = i;
      break;
    }
  }

  for (i = 0; i < (ARRAY_SIZE(calDBm) - 1); i++)
  {
    if (dB < calDBm[i + 1])
    {
      calDBmIndex = i;
      break;
    }
  }

  float x = dB;
  float x1 = calDBm[calDBmIndex];
  float x2 = calDBm[calDBmIndex + 1];

  float y = tempFreq;
  float y1 = calFreqs[calFreqsIndex];
  float y2 = calFreqs[calFreqsIndex + 1];

  float Q11 = calVpd[calDBmIndex][calFreqsIndex];
  float Q12 = calVpd[calDBmIndex][calFreqsIndex + 1];
  float Q21 = calVpd[calDBmIndex + 1][calFreqsIndex];
  float Q22 = calVpd[calDBmIndex + 1][calFreqsIndex + 1];

  float fxy1 = Q11 * (x2 - x) / (x2 - x1) + Q21 * (x - x1) / (x2 - x1);
  float fxy2 = Q12 * (x2 - x) / (x2 - x1) + Q22 * (x - x1) / (x2 - x1);

  uint16_t fxy = fxy1 * (y2 - y) / (y2 - y1) + fxy2 * (y - y1) / (y2 - y1);

  return fxy;
}

void target_rfPowerAmpPinSetup(void)
{
  TRACE_INFO("target_rfPowerAmpPinSetup\r");
  rtcen_pin = gpio_out_setup(RTC_ENABLE, 0);
  outputPower_pin = pwm_init(RTC_BIAS);
}

void target_set_power_dB(float dB)
{
  int8_t __attribute__((unused)) dBint = (int)(dB + 0.5);
  TRACE_INFO("target_set_power_dB %i (%i) \r", dBint, bilinearInterpolation(dB));

  pwm_out_write(outputPower_pin,bilinearInterpolation(dB));
}

void target_mspProcessPacket(uint16_t __attribute__((unused)) in_Function, uint8_t* __attribute__((unused)) rxPacket)
{
  uint16_t debug0;
  uint16_t debug1;

  TRACE_DEBUG("MSP receive %i\r", in_Function);

  switch (in_Function)
    {
    case MSP_DEBUG:
      debug0 = ((uint16_t)rxPacket[9] << 8) + rxPacket[8];
      debug1 = ((uint16_t)rxPacket[11] << 8) + rxPacket[10];
      TRACE_INFO("target_debug %04x %04x\r", debug0, debug1);
      switch (debug1)
        {
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
    
    default:
      break;
    }

}

void target_setup(void)
{
  target_rfPowerAmpPinSetup();
  TRACE_INFO("target_setup\r");
}

void target_loop(void)
{
  //TRACE_INFO("target_loop\r");
}

