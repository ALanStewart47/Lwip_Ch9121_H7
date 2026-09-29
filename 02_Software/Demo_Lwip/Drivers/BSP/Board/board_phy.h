#ifndef BOARD_PHY_H
#define BOARD_PHY_H

#include "stm32h7xx_hal.h"

typedef enum
{
  BOARD_PHY_STATUS_OK = 0,
  BOARD_PHY_STATUS_ADDR_NACK,
  BOARD_PHY_STATUS_DATA_NACK
} BoardPhyStatus;

extern volatile BoardPhyStatus g_board_phy_status;

HAL_StatusTypeDef Board_PHY_Reset(void);

#endif
