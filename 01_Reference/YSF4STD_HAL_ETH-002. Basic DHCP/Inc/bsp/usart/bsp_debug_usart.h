/**
  ******************************************************************************
  * 文件名程: bsp_debug_usart.h
  * 作    者: 硬石嵌入式开发团队
  * 版    本: V1.0
  * 编写日期: 2022-11-20
  * 功    能: bsp_debug_usart头文件
  ******************************************************************************
  * 说明：
  * 本例程配套硬石stm32开发板YS-F4STD使用。
  * 
  * 淘宝：
  * 论坛：http://www.ing10bbs.com
  * 版权归硬石嵌入式开发团队所有，请勿商用。
  ******************************************************************************
  */
#ifndef __BSP_DEBUG_USART_H__
#define __BSP_DEBUG_USART_H__

/* 包含头文件 ----------------------------------------------------------------*/
#include "stm32f4xx_hal.h"
#include <stdio.h>

/* 类型定义 ------------------------------------------------------------------*/

/* 宏定义 --------------------------------------------------------------------*/
#define DEBUG_USARTx                                 UART4
#define DEBUG_USARTx_BAUDRATE                        115200
#define DEBUG_USART_RCC_CLK_ENABLE()                 __HAL_RCC_UART4_CLK_ENABLE()
#define DEBUG_USART_RCC_CLK_DISABLE()                __HAL_RCC_UART4_CLK_DISABLE()

#define DEBUG_USARTx_GPIO_ClK_ENABLE()               __HAL_RCC_GPIOC_CLK_ENABLE()
#define DEBUG_USARTx_Tx_GPIO_PIN                     GPIO_PIN_10
#define DEBUG_USARTx_Tx_GPIO                         GPIOC
#define DEBUG_USARTx_Rx_GPIO_PIN                     GPIO_PIN_11
#define DEBUG_USARTx_Rx_GPIO                         GPIOC

#define DEBUG_USARTx_AFx                             GPIO_AF8_UART4

#define DEBUG_USART_IRQn                             UART4_IRQn

/* 扩展变量 ------------------------------------------------------------------*/
extern UART_HandleTypeDef husart_debug;

/* 函数声明 ------------------------------------------------------------------*/
void MX_DEBUG_USART_Init(void);


#endif  /* __BSP_DEBUG_USART_H__ */

/******************* (C) COPYRIGHT 2020-2030 硬石嵌入式开发团队 *****END OF FILE****/
