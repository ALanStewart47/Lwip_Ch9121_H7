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
 * 			V1.2	2025-10-15		Alan
 * 				-add function about uart baud config
 * 				-
 * @note saving data to flash
 *       1 tab == 4 spaces!
 *
 *****************************************************************************/
#include "app_uart.h"
#include "protocol_public.h"
//#include "bsp_uart_dma.h"
#include "main.h"
#include "app_light.h"
#include "bsp_timer.h"
//#include "app_spi_bridge.h"
//#include "app_spi_bridge_config.h"
#include "app_router.h"

#include <stdint.h>     
#include <stdio.h>

UART_rx_buf uart_data[UART_SUM]; 
unsigned char *urat_tx_buffer1;
unsigned int urat_tx_length = 0;
//uint8_t g_rs232_baud_gear = BAUD_19200;	//0-8 波特率档位 deault 19200
uint32_t g_rs232_baud = 19200;

uint32_t g_rs232_baud_buf[BAUD_MAX-1] = {
	4800,
	9600,
	14400,
	19200,
	28800,
	38400,
	57600,
	115200
};
 
//volatile uint8_t bridge_mode = 0;  //for uart1 <===> uart3
unsigned char slave_rx_buffer[BUFFER_SIZE];

void init_Uart_data(void)
{
	unsigned char i = 0;
	
	for(i = 0; i < UART_SUM; i++)
	{
		uart_data[i].enable = 0;
	}
}

#if GD32
void init_and_set_uart(unsigned char uart_number,uint32_t  huart, dma_channel_enum  hdma_usart_rx)
#elif STM32
void init_and_set_uart(unsigned char uart_number,UART_HandleTypeDef *huart, DMA_HandleTypeDef *hdma_usart_rx)
#endif
{
	unsigned char i = 0;

	for(i = 0; i < UART_SUM; i++)
	{
		if(uart_data[i].enable == 0)
		{
			uart_data[i].enable 		= 1;
			uart_data[i].uart_number 	= uart_number;						
			uart_data[i].huart 			= huart;
			uart_data[i].hdma_usart_rx 	= hdma_usart_rx;
			uart_data[i].rx_endFlag 	= 0;
			uart_data[i].rx_length 		= 0;
			my_memset(uart_data[i].rx_buffer,0,BUFFER_SIZE);
		#if GD32
			usart_interrupt_enable(uart_data[i].huart, USART_INT_IDLE);
		#elif STM32
			__HAL_UART_ENABLE_IT(uart_data[i].huart, UART_IT_IDLE);
		#endif
			UART_Receive_DMA(uart_data[i].uart_number);
			break;
			
		}
	}
}

/**
 *  increase rs232 baud gear, 
 *  @note : the gear can display in the led tube , so modify gear,and no to edit baud,
 * 			the baud will be update in get_rs232_baud(); 
 */
void inc_rs232_baud_gear(void )
{
	if(g_light_system.uart_baud_gear < BAUD_115200){
		g_light_system.uart_baud_gear++;
	}
	else{
		g_light_system.uart_baud_gear = BAUD_4800;
	}
}

/**
 *  decrease rs232 baud gear, 
 *  @note : the gear will display in the led tube , so modify gear,and no to edit baud,
 * 			the baud will be update in get_rs232_baud(); 
 */
void dec_rs232_baud_gear(void )
{
	if(g_light_system.uart_baud_gear > BAUD_4800){
		g_light_system.uart_baud_gear--;
	}
	else{
		g_light_system.uart_baud_gear = BAUD_115200;
	}
}

uint8_t get_232_baud_is_change(void)
{
	static uint8_t s_rs232_gear = 0;
	if(s_rs232_gear != g_light_system.uart_baud_gear)
	{
		s_rs232_gear = g_light_system.uart_baud_gear;
		return 1 ;
	}
	return 0;
}

static void get_rs232_baud(void)
{
	if(g_light_system.uart_baud_gear < BAUD_MAX)
	{
		if(g_light_system.uart_baud_gear == 0)
		{
			g_light_system.uart_baud_gear = 4;
		}
		g_rs232_baud = g_rs232_baud_buf[g_light_system.uart_baud_gear - 1];
	}
}

#if STM32
void bsp_rs232_reinit(void)
{
	get_rs232_baud();
	huart2.Init.BaudRate = g_rs232_baud;
  	if (HAL_UART_Init(&huart2) != HAL_OK)
  	{
  		Error_Handler();
  	}
}
#endif

#if GD32
void bsp_rs232_reinit(void)
{
	get_rs232_baud();
	uart_rs232_init(g_rs232_baud);
}
#endif

void UART_Receive_DMA(unsigned char uart_number)
{
	UART_rx_buf *uart_p = NULL;
	unsigned char i = 0;

	for(i = 0; i < UART_SUM; i++)
	{
		if(uart_data[i].uart_number == uart_number)
		{
			uart_p = &uart_data[i];
		}
	}
#if GD32
	GD_UART_Receive_DMA(uart_p->huart , uart_p->rx_buffer, BUFFER_SIZE);
#elif STM32
	HAL_UART_Receive_DMA(uart_p->huart, uart_p->rx_buffer, BUFFER_SIZE);
#endif

}


void DMA_Usart_Send(unsigned char uart_number,uint8_t *buf,uint16_t len)
{
	UART_rx_buf *uart_p = 0;
	unsigned char i = 0;

	for(i = 0; i < UART_SUM; i++)
	{
		if(uart_data[i].uart_number == uart_number)
		{
			uart_p = &uart_data[i];
			break;
		}
	}
#if GD32
	GD_UART_Transmit_DMA(uart_p->huart , buf,len);
#elif STM32
	if(HAL_UART_Transmit_DMA(uart_p->huart, buf, len) != HAL_OK) 
	{
		//Error_Handler();
	}
#endif
}


void uart_receive(unsigned char uart_number)
{
	unsigned char recv_flag = 0;
	unsigned int numb = 0;
	unsigned char i = 0;
	UART_rx_buf *uart_p = 0;

	for(i = 0; i < UART_SUM; i++)
	{
		if(uart_data[i].uart_number == uart_number)
		{
			uart_p = &uart_data[i];
		}
	}
#if GD32
	numb = dma_transfer_number_get(DMA0,uart_p->hdma_usart_rx);
	uart_p->rx_length = BUFFER_SIZE - numb;
	uart_p->rx_endFlag =1;
#elif STM32
	recv_flag =__HAL_UART_GET_FLAG(uart_p->huart,UART_FLAG_IDLE);
	if((recv_flag != 0))
	{
		__HAL_UART_CLEAR_IDLEFLAG(uart_p->huart);
		HAL_UART_DMAStop(uart_p->huart);
		numb = __HAL_DMA_GET_COUNTER(uart_p->hdma_usart_rx);	// get dma remaining data number of channel 
		uart_p->rx_length = BUFFER_SIZE - numb;
		uart_p->rx_endFlag =1;
	}
#endif

}


#if GD32
void USART1_IRQHandler(void)
{

	if(RESET != usart_interrupt_flag_get(USART1, USART_INT_FLAG_IDLE))
	{
		//Module.USART1_RX_OK=1;		
		/* clear USART_INT_FLAG_IDLE */
		usart_data_receive(USART1);
		/* disable USART2_RX DMA_Channel */
		dma_channel_disable(DMA0, DMA_CH5);  
		/* user function */
		uart_receive(1);
		/* reset DMA_Channel CNT */
		dma_transfer_number_config(DMA0, DMA_CH5, BUFFER_SIZE);
		/* enable USART2_RX DMA_Channel */
		dma_channel_enable(DMA0, DMA_CH5);
	}
}
#endif
/**
 *  NET IRQ Handler
 */
#if GD32
void USART0_IRQHandler(void)
{
	if(RESET != usart_interrupt_flag_get(USART0, USART_INT_FLAG_IDLE))
	{	
		/* clear USART_INT_FLAG_IDLE */
		usart_data_receive(USART0);
		/* disable USART2_RX DMA_Channel */
		dma_channel_disable(DMA0, DMA_CH4);   
		/* user function */
		uart_receive(0);
		/* reset DMA_Channel CNT */
		dma_transfer_number_config(DMA0, DMA_CH4, BUFFER_SIZE);
		/* enable USART2_RX DMA_Channel */
		dma_channel_enable(DMA0, DMA_CH4);
	}
}
#endif


void my_memset(unsigned char *dest, unsigned char set, unsigned int len)
{
	unsigned char *pdest = (unsigned char *)dest;
	
	while (len != 0)
	{
		*pdest++ = set;
		len--;
	}
	
}

void uart_spi_comm_complete(uint8_t slave_id, spi_status_t status, uint8_t *response_data, uint16_t response_len) 
{
    if (status == SPI_STATUS_OK) 
	{
        DMA_Usart_Send(uart_data->uart_number, response_data, response_len);  // 转发给上位机
    }
}


void uart_data_handle(UART_rx_buf *urat_data)
{
	if( urat_data->rx_endFlag != 0 )
	{
	#if APP_ROUTER_ENABLE
		if (urat_data->rx_endFlag != 0) {
			router_process_uart(urat_data->uart_number, urat_data->rx_buffer, urat_data->rx_length);
			memset(urat_data->rx_buffer, 0, urat_data->rx_length);
    		urat_data->rx_length = 0;
    		urat_data->rx_endFlag = 0;
    		UART_Receive_DMA(urat_data->uart_number);
		}
	#else
		// 原有的协议处理（如果需要保留）
		input_data(urat_data->rx_buffer,urat_data->rx_length);
		urat_tx_length = 0;
		analysis_command();
		urat_tx_buffer1 = get_prepare_tx_buffer(&urat_tx_length);

		DMA_Usart_Send(urat_data->uart_number, urat_tx_buffer1, urat_tx_length);

		//my_memset(urat_data->rx_buffer,0,urat_data->rx_length);
		memset(urat_data->rx_buffer,0,urat_data->rx_length);
		urat_data->rx_length = 0;
		urat_data->rx_endFlag = 0;

		UART_Receive_DMA(urat_data->uart_number);
	#endif
	}
}

 /**
 * @brief	uart task handle
 * @note	
 * @param	void
 * @retval	void
 */
void uart_task(void)
{
	// 处理串口数据
	uart_data_handle(&uart_data[0]);
#if GD32
	uart_data_handle(&uart_data[1]);
#endif
}



 /**
 * @brief	uart task init
 * @note	
 * @param	void
 * @retval	void
 */
void uart_task_init(void)
{
	init_Uart_data();
	init_protocol_para();
	
	init_and_set_uart(0,&huart2, &hdma_usart2_rx);
}
