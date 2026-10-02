#include "ch9121_board.h"

#include "stm32f4xx_hal.h"

const uint8_t *ch9121_board_mac(void)
{
  static uint8_t address[6];
  static uint8_t initialized;
  const uint32_t *uid = (const uint32_t *)UID_BASE;
  uint32_t mix;
  uint32_t i;

  if (initialized == 0U)
  {
    mix = uid[0] ^ (uid[1] << 11U) ^ (uid[1] >> 21U) ^
          (uid[2] << 21U) ^ (uid[2] >> 11U);
    address[0] = 0x02U;
    for (i = 1U; i < 6U; ++i)
    {
      mix = (mix * 1664525U) + 1013904223U;
      address[i] = (uint8_t)(mix >> 24U);
    }
    initialized = 1U;
  }

  return address;
}
