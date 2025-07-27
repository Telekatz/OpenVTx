#pragma once

#define MAX_POWER                   100 // mW

#define SA_NUM_POWER_LEVELS         4 // Max 5 for INAV.
#define POWER_LEVEL_LABEL_LENGTH    3
#define USE_CUSTOM_FREQ_TABLE       1

#if USE_CUSTOM_FREQ_TABLE == 1
#define BAND_COUNT                  4
#define DEFAULT_BAND                3
#define DEFAULT_CHANNEL             4
#define DEFAULT_POWER               2
#endif

extern uint8_t saPowerLevelsLut[SA_NUM_POWER_LEVELS];
extern uint8_t saPowerLevelsLabel[SA_NUM_POWER_LEVELS * POWER_LEVEL_LABEL_LENGTH];

#if 1
#define UART_RX       PA10
#define UART_TX       PA9
#else
#define UART_RX       PA2
#define UART_TX       PA2
#endif

#define SPI_SS        PC15
#define SPI_CLOCK     PC14
#define SPI_MOSI      PB7

#define LED1          PB8 // Red (power)
#define LED2          PB9 // Green (connected)
//#define LED3          PA2 // Blue (SA message)

#define RTC_BIAS      PB6
#define RTC_ENABLE    PB12

#define BUTTON        PC13

/******* Target specific declarations *******/
#include "gpio.h"
#include "serial.h"
#include "pwm.h"


uint32_t millis(void);
void delay(uint32_t ms);
void delayMicroseconds(uint32_t us);

