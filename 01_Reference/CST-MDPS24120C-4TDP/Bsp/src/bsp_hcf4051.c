/******************************************************************************
 * Copyright (C) 2025 CST, Inc.(Gmbh) or its affiliates.
 *
 * All Rights Reserved.
 *
 * @file bsp_hcf4051.c
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
#include "bsp_hcf4051.h"
#include "main.h"

void bsp_hcf4051_init(void)
{
    HAL_GPIO_WritePin(MULTI_EN_GPIO_Port, MULTI_EN_Pin, GPIO_PIN_RESET);  //enable  channel input or output
}



void bsp_hcf4051_set_channel(uint8_t channel)
{
    if(channel > 7) return;

    bsp_hcf4051_open_channel();
    
    HAL_GPIO_WritePin(MULTI_A_GPIO_Port, MULTI_A_Pin, (GPIO_PinState)(channel & 0x01));
    HAL_GPIO_WritePin(MULTI_B_GPIO_Port, MULTI_B_Pin, (GPIO_PinState)((channel >> 1) & 0x01));
    HAL_GPIO_WritePin(MULTI_C_GPIO_Port, MULTI_C_Pin, (GPIO_PinState)((channel >> 2) & 0x01));
    
    // ? 关键修复: HCF4051 通道切换后需要稳定时间
    // 数据手册: tON/tOFF典型值30ns, 最大85ns
    // 考虑PCB走线延迟 + 从机响应时间, 添加1us延迟确保稳定
    for (volatile uint32_t i = 0; i < 170; i++) {  // 170MHz系统时钟: ~1us
        __NOP();
    }
}



void bsp_hcf4051_open_channel(void)
{
    HAL_GPIO_WritePin(MULTI_EN_GPIO_Port, MULTI_EN_Pin, GPIO_PIN_RESET);  //enable channel input or output
}

/**
 * @brief Close the HCF4051 multiplexer channel
 * 
 * This function disables the HCF4051 multiplexer by setting the enable pin high,
 * which disables the channel input or output functionality.
 * 
 * @param None
 * @return None
 */
void bsp_hcf4051_close_channel(void)
{
    HAL_GPIO_WritePin(MULTI_EN_GPIO_Port, MULTI_EN_Pin, GPIO_PIN_SET);  //disable channel input or output
}

