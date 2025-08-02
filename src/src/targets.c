#include <stdint.h>
#include "targets.h"
#include "mspVtx.h"

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

__weak void target_set_power_dB(__attribute__((unused)) float power)
{
  /* NOTE : This function should not be modified, when the callback is needed,
            the function could be implemented in target
   */
}

__weak void target_mspProcessPacket(__attribute__((unused)) mspPacket_t *packet)
{
  /* NOTE : This function should not be modified, when the callback is needed,
            the function could be implemented in target
   */
}

__weak void target_mspUpdate(__attribute__((unused)) uint8_t mspState)
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