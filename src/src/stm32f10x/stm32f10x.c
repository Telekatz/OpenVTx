#include "stm32f1xx_hal.h"

#include "targets.h"
#include "dbgu.h"
#include "common.h"
#include "EEPROM.h"
uint32_t _bootloader_data;

void mcu_reboot(void)
{

}

void SystemClock_Config(void)
{
  RCC_ClkInitTypeDef RCC_ClkInitStruct;
  RCC_OscInitTypeDef RCC_OscInitStruct;

  /* Enable HSE Oscillator and activate PLL with HSE as source */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct)!= HAL_OK)
  {
    /* Initialization Error */
    while(1); 
  }

  /* Select PLL as system clock source and configure the HCLK, PCLK1 and PCLK2
     clocks dividers */
  RCC_ClkInitStruct.ClockType = (RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2);
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2)!= HAL_OK)
  {
    /* Initialization Error */
    while(1); 
  }


  HAL_SYSTICK_Config(HAL_RCC_GetHCLKFreq()/1000);

  HAL_SYSTICK_CLKSourceConfig(SYSTICK_CLKSOURCE_HCLK);

  /* SysTick_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(SysTick_IRQn, 2, 0);

  SystemCoreClockUpdate();

}

void HAL_MspInit(void)
{
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_AFIO_CLK_ENABLE();
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_RCC_TIM1_CLK_ENABLE();
  __HAL_RCC_TIM2_CLK_ENABLE();
  __HAL_RCC_TIM3_CLK_ENABLE();
  __HAL_RCC_TIM4_CLK_ENABLE();
  __HAL_RCC_DMA1_CLK_ENABLE();

}

uint32_t millis(void)
{
  return HAL_GetTick();
}

void delay(uint32_t const ms)
{
  uint32_t const start = millis();
  while((millis() - start) < ms);
}

void delayMicroseconds(uint32_t us)
{
  CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
  DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;

  uint32_t start = DWT->CYCCNT;
  uint32_t ticks = us * (SystemCoreClock / 1000000);

  while ((DWT->CYCCNT - start) < ticks);

}

void _Error_Handler(const char * file, int line) {
  TRACE_ERROR("Error Handler %s %i \r", file, line);
  while (1) {}
}


gpio_out_t led1_pin;
gpio_in_t buttonPin;
gpio_pwm_t pwm_pin;

int main(void)
{
    
  HAL_Init();
  SystemClock_Config();
  
  SWO_Init();

  TRACE_INFO_WP("\n");
  TRACE_INFO("Getting new Started Project --\r");
  TRACE_INFO("Compiled: %s %s --\r", __DATE__, __TIME__);

  flash_init();

  /*
  led1_pin = gpio_out_setup(LED1, 1);
  pwm_pin = pwm_init(PB5);

  pwm_out_write(pwm_pin,1500);

  serial_begin(9600,UART_TX,UART_RX,1);


  while (1)
  {
    gpio_out_toggle(led1_pin);
      delay(1000);
      
    if(serial_available()) {
      uint8_t inbyte = serial_read();

      pwm_out_write(pwm_pin,(inbyte - 'a') * 100);
    }

    //TRACE_INFO_WP("Tick\r");
  }
*/

    extern void setup(void);
    extern void loop(void);
    setup();
    while(1) loop();

    return 0;
}

