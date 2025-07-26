#include <stdint.h>
#include "targets.h"

__weak void target_setup(void)
{
  /* NOTE : This function should not be modified, when the callback is needed,
            the function could be implemented in target
   */
}

__weak void target_loop(void)
{
  /* NOTE : This function should not be modified, when the callback is needed,
            the function could be implemented in target
   */
}

__weak void target_set_power_dB(float __attribute__((unused)) power)
{
  /* NOTE : This function should not be modified, when the callback is needed,
            the function could be implemented in target
   */
}

__weak void target_mspProcessPacket(uint16_t __attribute__((unused)) in_Function, uint8_t* __attribute__((unused)) rxPacket)
{
  /* NOTE : This function should not be modified, when the callback is needed,
            the function could be implemented in target
   */
}

__weak void mcu_reboot(void)
{
  /* NOTE : This function should not be modified, when the callback is needed,
            the function could be implemented in target
   */
}