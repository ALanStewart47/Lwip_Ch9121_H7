/******************************************************************************
 * Copyright (C) 2025 CST, Inc.(Gmbh) or its affiliates.
 *
 * All Rights Reserved.
 *
 * @file bsp_uart_dma.h
 *
 * @par dependencies
 * - 
 *
 * @author Alan | R&D Dept. | CST
 *
 * @brief Provide the HAL APIs of the uart by dma
 *
 * Processing flow:
 * call directly.
 *
 * @version     V1.0        2025-04-28          ALan
 * @note    
 *       1 tab == 4 spaces!
 *
 *****************************************************************************/

 #ifndef __BSP_UART_DMA_H__  //Avoid repeated including same files later
 #define __BSP_UART_DMA_H__
 
 //******************************** Includes *********************************//

 #include "main.h"
 #include <stdint.h>              
 #include <stdio.h>
 
//******************************** Includes *********************************//

//******************************** Defines **********************************//
 #define COMn                       2

 #define UART_SEND_BUF_LENGTH       100
 #define UART_RECEIVE_BUF_LENGTH    300

 #define NET_SEND_BUF_LENGTH        100
 #define NET_RECEIVE_BUF_LENGTH     300

//******************* RS232 *******************//
#define RS232_UART          USART1
#define RS232_IRQn          USART1_IRQn

#define RS232_GPIO_CLK      RCU_GPIOA
#define RS232_UART_CLK      RCU_USART1

#define RS232_GPIO_PORT     GPIOA
#define RS232_RX_PIN        GPIO_PIN_3
#define RS232_TX_PIN        GPIO_PIN_2

//******************* NET *******************//
#define NET_UART            USART0
#define NET_IRQn            USART0_IRQn

#define NET_GPIO_CLK        RCU_GPIOB
#define NET_UART_CLK        RCU_USART0

#define NET_GPIO_PORT       GPIOB
#define NET_RX_PIN          GPIO_PIN_7
#define NET_TX_PIN          GPIO_PIN_6

#define NET_RESET_CLK       RCU_GPIOB
#define NET_RESET_PORT      GPIOB
#define NET_RESET_PIN       GPIO_PIN_5

#define NET_CFG0_CLK        RCU_GPIOB
#define NET_CFG0_PORT       GPIOB
#define NET_CFG0_PIN        GPIO_PIN_4


//******************************** Defines **********************************//

//******************************** Declaring ********************************//

void uart_rs232_init(uint32_t baudval);
void dma_rs232_init(void);
void GD_UART_Transmit_DMA(uint32_t usart_periph,uint8_t* data_buffer,uint32_t length);
void GD_UART_Receive_DMA(uint32_t usart_periph,uint8_t* data_buffer,uint32_t length);
void bsp_uart_hardware_init(void);
//******************************** Declaring ********************************//

 #endif /* __UART_DMA_H__ */ 
