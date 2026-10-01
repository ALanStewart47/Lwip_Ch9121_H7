/******************************************************************************
 * Copyright (C) 2024 CST, Inc.(Gmbh) or its affiliates.
 *
 * All Rights Reserved.
 *
 * @file app_uart.h
 *
 * @par dependencies
 * - protocol_public.h
 *
 * @author imphx  | R&D Dept. | CST
 *
 * @brief 
 *
 * Processing flow:
 * call directly.
 *
 * @version V1.0	20xx-xx-xx * 	imphx
 * 			V1.1	2025-03-14		Alan
 * 				-add function about uart baud config
 * 				-
 * @note saving data to flash
 *       1 tab == 4 spaces!
 *
 *****************************************************************************/
#ifndef __APP_UART_H__
#define __APP_UART_H__

#ifdef __cplusplus
extern "C" {
#endif
//******************************** Includes *********************************//
#include "main.h"
#include "app.h"
#include "bsp_spi_bus_dma.h"
//#include "usart.h"
//******************************** Includes *********************************//

//******************************** Defines **********************************//
#define    BUFFER_SIZE		300
#define    UART_SUM			1
#if GD32
	#define		RS232_UART		USART1
	#define		NET_UART		USART0
#elif STM32
	#define	RS232_UART			USART2
#endif
#define		UART_MODE		1
#define 	NET_MODE		0

#if STM32
	extern UART_HandleTypeDef huart2;
	extern DMA_HandleTypeDef hdma_usart2_rx;
	extern DMA_HandleTypeDef hdma_usart2_tx;
#endif

typedef enum
{
  UART_0 = 0,
  UART_1,
  UART_2,
  UART_3,
  UART_4
}UART_NUMBER;

typedef enum
{
	BAUD_DETECT 	= 0,
	BAUD_4800 		= 1,
	BAUD_9600 		= 2,
	BAUD_14400 		= 3,
	BAUD_19200 		= 4, 
	BAUD_28800 		= 5,
	BAUD_38400 		= 6, 
	BAUD_57600 		= 7, 
	BAUD_115200 	= 8,
    BAUD_MAX
}uart_bauds_t;

#if GD32
typedef struct __UART_RX_BUF{
	unsigned char enable;
	unsigned char uart_number;
	unsigned int rx_length;
	unsigned char rx_buffer[BUFFER_SIZE];
	unsigned char rx_endFlag;
	uint32_t huart;
	dma_channel_enum  hdma_usart_rx;
}UART_rx_buf;
#elif STM32
typedef struct __UART_RX_BUF{
	unsigned char enable;
	unsigned char uart_number;
	unsigned int rx_length;
	unsigned char rx_buffer[BUFFER_SIZE];
	unsigned char rx_endFlag;
	UART_HandleTypeDef *huart;
	DMA_HandleTypeDef *hdma_usart_rx;
}UART_rx_buf;
#endif

extern uint8_t g_rs232_baud_gear;	//0-8 ²¨ÌØÂÊµµÎ» deault 19200
extern uint32_t g_rs232_baud;
extern uint32_t g_rs232_baud_buf[BAUD_MAX-1];
//******************************** Defines **********************************//

//******************************** Declaring ********************************//
void UART_Receive_DMA(unsigned char uart_number);
void my_memset(unsigned char *dest, unsigned char set, unsigned int len);
void init_Uart_data(void);
#if GD32
void init_and_set_uart(unsigned char uart_number,uint32_t  huart, dma_channel_enum  hdma_usart_rx);
#elif STM32
void init_and_set_uart(unsigned char uart_number,UART_HandleTypeDef *huart, DMA_HandleTypeDef *hdma_usart_rx);
#endif
void uart_receive(unsigned char uart_number);
void uart_task(void);
void uart_ota_task(void);
void uart_tmc_task(void);
void uart_task_init(void);
void bsp_rs232_reinit(void);
void dec_rs232_baud_gear(void );
void inc_rs232_baud_gear(void );
uint8_t get_232_baud_is_change(void);
void DMA_Usart_Send(unsigned char uart_number,uint8_t *buf,uint16_t len);
//******************************** Declaring ********************************//	
#ifdef __cplusplus
}
#endif


#endif 

