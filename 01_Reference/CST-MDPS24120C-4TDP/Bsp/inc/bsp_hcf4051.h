/******************************************************************************
 * Copyright (C) 2025 CST, Inc.(Gmbh) or its affiliates.
 *
 * All Rights Reserved.
 *
 * @file bsp_hcf4051.h
 *
 * @par dependencies
 * 
 *
 * @author Alan | R&D Dept. | CST
 *
 * @brief Provide the HAL APIs of the Single 8-channel analog multiplexer/demultiplexer.
 *
 * Processing flow:
 * call directly.
 *
 * @version         V1.0    2025-10-13   ALan
 *                 
 * @note saving data to flash
 *       1 tab == 4 spaces!
 *
 *****************************************************************************/
#ifndef __BSP_HCF4051_H__
#define __BSP_HCF4051_H__   

#include "stdint.h"

void bsp_hcf4051_init(void);
void bsp_hcf4051_set_channel(uint8_t channel);
void bsp_hcf4051_open_channel(void);
void bsp_hcf4051_close_channel(void);


#endif /* __BSP_HCF4051_H__ */
