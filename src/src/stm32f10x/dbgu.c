/* Includes ------------------------------------------------------------------*/
#include <stm32f103xb.h>
#include <stdio.h>
#include "stm32f1xx_hal.h"

uint8_t DBGU_RxByte;
uint8_t DBGU_RxReady;

void SWO_PrintChar(char c, uint8_t portNo) {
  volatile int timeout;

    /* Check if Trace Control Register (ITM->TCR at 0xE0000E80) is set */
    if ((ITM->TCR&ITM_TCR_ITMENA_Msk) == 0) { /* check Trace Control Register if ITM trace is enabled*/
      return; /* not enabled? */
    }
    /* Check if the requested channel stimulus port (ITM->TER at 0xE0000E00) is enabled */
    if ((ITM->TER & (1ul<<portNo))==0) { /* check Trace Enable Register if requested port is enabled */
      return; /* requested port not enabled? */
    }
    timeout = 5000; /* arbitrary timeout value */
    while (ITM->PORT[portNo].u32 == 0) {
      /* Wait until STIMx is ready, then send data */
      timeout--;
      if (timeout==0) {
        return; /* not able to send */
      }
    }
    ITM->PORT[portNo].u8 = (uint8_t)c;
    //ITM->PORT[0].u16 = 0x08 | (c<<8);
}

int fputc(int ch, FILE *f) {
  SWO_PrintChar((uint8_t)ch,0);
  return ch;
}

signed int fputs(const char *pStr, FILE *pStream)
{
    signed int num = 0;

    while (*pStr != 0) {

        if (fputc(*pStr, pStream) == -1) {

            return -1;
        }
        num++;
        pStr++;
    }

    return num;
}


void _putchar(char character)
{
    SWO_PrintChar((uint8_t)character,0);
}

uint32_t _ITMPort = 0; // The stimulus port from which SWO data is received and displayed.


void SWO_Init() {

  uint32_t SWOPrescaler = 10 ; // baudrate in Hz, note that cpuCoreFreqHz is expected to match the CPU core clock
 
	CoreDebug->DEMCR = CoreDebug_DEMCR_TRCENA_Msk; 		// Debug Exception and Monitor Control Register (DEMCR): enable trace in core debug
	DBGMCU->CR	= 0x00000027u ;							// DBGMCU_CR : TRACE_IOEN DBG_STANDBY DBG_STOP 	DBG_SLEEP
	TPI->SPPR	= 0x00000002u ;							// Selected PIN Protocol Register: Select which protocol to use for trace output (2: SWO)
	TPI->ACPR	= SWOPrescaler ;						// Async Clock Prescaler Register: Scale the baud rate of the asynchronous output
	ITM->LAR	= 0xC5ACCE55u ;							// ITM Lock Access Register: C5ACCE55 enables more write access to Control Register 0xE00 :: 0xFFC
	ITM->TCR	= 0x0001000Du ;							// ITM Trace Control Register
	ITM->TPR	= ITM_TPR_PRIVMASK_Msk ;				// ITM Trace Privilege Register: All stimulus ports
	ITM->TER	= 0x01;							// ITM Trace Enable Register: Enabled tracing on stimulus ports. One bit per stimulus port.
	DWT->CTRL	= 0x400003FEu ;							// Data Watchpoint and Trace Register
	TPI->FFCR	= 0x00000100u ;

}

