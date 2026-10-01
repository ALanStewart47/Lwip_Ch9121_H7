/******************************************************************************
 * Copyright (C) 2025 CST, Inc.(Gmbh) or its affiliates.
 *
 * All Rights Reserved.
 *
 * @file bsp_uart_dma.c
 *
 * @par dependencies
 * - bsp_uart_dma.h
 *
 * @author Alan | R&D Dept. | CST
 *
 * @brief Provide the HAL APIs of the uart by dma
 *
 * Processing flow:
 * call directly.
 *
 * @version    V1.0     2025-04-28      ALan 
 *             V1.1     2025-07-14		ALan
 * @note    
 *              1 tab == 4 spaces!
 *
 *****************************************************************************/
#include "bsp_uart_dma.h"
#include "app_uart.h"
#include "app_flash.h"
/*
uint8_t rs232_txbuffer[UART_SEND_BUF_LENGTH] = {0};
uint8_t rs232_rxbuffer[UART_RECEIVE_BUF_LENGTH] = {0};

uint8_t net_txbuffer[UART_SEND_BUF_LENGTH] = {0};
uint8_t net_rxbuffer[UART_RECEIVE_BUF_LENGTH] = {0};
*/


/**
 * @brief   RS232 uart init 
 * @note    uart_1    PA2 PA3    
 * @param  
 * @retval  void
 */
void uart_rs232_init(uint32_t baudval)
{
    /* enable GPIO clock */
    rcu_periph_clock_enable(RS232_GPIO_CLK);

    /* enable USART clock */
    rcu_periph_clock_enable(RS232_UART_CLK);

    /* connect port to USARTx_Tx */
    gpio_init(RS232_GPIO_PORT, GPIO_MODE_AF_PP, GPIO_OSPEED_50MHZ, RS232_TX_PIN);

    /* connect port to USARTx_Rx */
    gpio_init(RS232_GPIO_PORT, GPIO_MODE_IN_FLOATING, GPIO_OSPEED_50MHZ, RS232_RX_PIN);

    /* USART configure */
    usart_deinit(RS232_UART);
    usart_baudrate_set(RS232_UART, baudval);
    usart_receive_config(RS232_UART, USART_RECEIVE_ENABLE);
    usart_transmit_config(RS232_UART, USART_TRANSMIT_ENABLE);
    usart_dma_receive_config(RS232_UART, USART_RECEIVE_DMA_ENABLE);
    usart_dma_transmit_config(RS232_UART, USART_TRANSMIT_DMA_ENABLE);
    usart_enable(RS232_UART);

    nvic_irq_enable(USART1_IRQn, 2, 0);
    //nvic_irq_enable(USART1_IRQn, 0, 0);
    usart_interrupt_enable(RS232_UART, USART_INT_IDLE);
}




/**
 * @brief   net gpio init 
 * @note    reset,cfg  gpio
 * @param  
 * @retval  void
 */
static void net_ch9121_gpio_init(void)
{
    rcu_periph_clock_enable(NET_RESET_CLK);
    rcu_periph_clock_enable(NET_CFG0_CLK);

    gpio_bit_write(NET_RESET_PORT,NET_RESET_PIN, SET);
    gpio_bit_write(NET_CFG0_PORT,NET_CFG0_PIN,   SET);

    gpio_init(NET_RESET_PORT, GPIO_MODE_OUT_PP, GPIO_OSPEED_2MHZ, NET_RESET_PIN);
    gpio_init(NET_CFG0_PORT, GPIO_MODE_OUT_PP, GPIO_OSPEED_2MHZ, NET_CFG0_PIN);
}


/**
 * @brief   CH9121 NET uart init 
 * @note    using uart0 include reset and cfg0 gpio init.
 * @param  
 * @retval  void
 */
void uart_net_init(uint32_t com,uint32_t baudval)
{   
    /* reset cfg0 gpio init */
    net_ch9121_gpio_init();
    /* enable GPIO clock */
    rcu_periph_clock_enable(RCU_AF);

    rcu_periph_clock_enable(NET_GPIO_CLK);
    /* enable USART clock */
    rcu_periph_clock_enable(NET_UART_CLK);

    gpio_pin_remap_config(GPIO_USART0_REMAP,ENABLE);

    /* connect port to USARTx_Tx */
    gpio_init(NET_GPIO_PORT, GPIO_MODE_AF_PP, GPIO_OSPEED_50MHZ, NET_TX_PIN);

    /* connect port to USARTx_Rx */
    gpio_init(NET_GPIO_PORT, GPIO_MODE_IN_FLOATING, GPIO_OSPEED_50MHZ, NET_RX_PIN);

    /* USART configure */
    usart_deinit(USART0);
    usart_baudrate_set(USART0, baudval);
    usart_receive_config(USART0, USART_RECEIVE_ENABLE);
    usart_transmit_config(USART0, USART_TRANSMIT_ENABLE);
    usart_dma_receive_config(USART0, USART_RECEIVE_DMA_ENABLE);
    usart_dma_transmit_config(USART0, USART_TRANSMIT_DMA_ENABLE);
    usart_enable(USART0);

    nvic_irq_enable(USART0_IRQn, 2, 1);
    //nvic_irq_enable(USART0_IRQn, 0, 0);
    usart_interrupt_enable(USART0, USART_INT_IDLE);
}

/**
 * @brief   dma init of RS232
 * @note    DMA0   CH5 CH6    
 * @param  
 * @retval  void
 */
void dma_rs232_init(void)
{
    dma_parameter_struct dma_init_struct;
    rcu_periph_clock_enable(RCU_DMA0);

    
    //dma_deinit(DMA0, DMA_CH6);

    /* deinitialize DMA channel6(USART1 tx) */
    dma_deinit(DMA0, DMA_CH6);
    dma_struct_para_init(&dma_init_struct);

    dma_init_struct.direction       = DMA_MEMORY_TO_PERIPHERAL;
    dma_init_struct.memory_inc      = DMA_MEMORY_INCREASE_ENABLE;
    dma_init_struct.memory_width    = DMA_MEMORY_WIDTH_8BIT;
    dma_init_struct.periph_addr     = (uint32_t)&USART_DATA(RS232_UART);
    dma_init_struct.periph_inc      = DMA_PERIPH_INCREASE_DISABLE;
    dma_init_struct.periph_width    = DMA_PERIPHERAL_WIDTH_8BIT;
    dma_init_struct.priority        = DMA_PRIORITY_ULTRA_HIGH;
    dma_init(DMA0, DMA_CH6, &dma_init_struct);

    /* initialize DMA0 channel2(Usart1_RX) */
    dma_deinit(DMA0, DMA_CH5);
    dma_struct_para_init(&dma_init_struct);
    dma_init_struct.direction       = DMA_PERIPHERAL_TO_MEMORY;
	dma_init_struct.memory_inc      = DMA_MEMORY_INCREASE_ENABLE;
	dma_init_struct.memory_width    = DMA_MEMORY_WIDTH_8BIT;
	dma_init_struct.periph_addr     = (uint32_t)(&USART_DATA(RS232_UART));
	dma_init_struct.periph_inc      = DMA_PERIPH_INCREASE_DISABLE;
	dma_init_struct.periph_width    = DMA_PERIPHERAL_WIDTH_8BIT;
	dma_init_struct.priority        = DMA_PRIORITY_ULTRA_HIGH;
	dma_init(DMA0, DMA_CH5, &dma_init_struct);	


    /* configure DMA mode */
    dma_circulation_disable     (DMA0, DMA_CH5);
    dma_memory_to_memory_disable(DMA0, DMA_CH5);

    dma_circulation_disable     (DMA0, DMA_CH6);
    dma_memory_to_memory_disable(DMA0, DMA_CH6);

    dma_channel_disable(DMA0,DMA_CH6);      //  disable tx dma
    dma_channel_disable(DMA0,DMA_CH5);       //  enable  rx dma
}


/**
 * @brief   dma init of NET
 * @note    DMA0   CH3 CH4    
 * @param  
 * @retval  void
 */
void dma_net_init(void)
{
    dma_parameter_struct dma_init_struct;

    rcu_periph_clock_enable(RCU_DMA0);
    //dma_deinit(DMA0, DMA_CH6);

    /* deinitialize DMA channel3(USART0 tx) */
    dma_deinit(DMA0, DMA_CH3);
    dma_struct_para_init(&dma_init_struct);

    dma_init_struct.direction       = DMA_MEMORY_TO_PERIPHERAL;
    dma_init_struct.memory_inc      = DMA_MEMORY_INCREASE_ENABLE;
    dma_init_struct.memory_width    = DMA_MEMORY_WIDTH_8BIT;
    dma_init_struct.periph_addr     = (uint32_t)&USART_DATA(NET_UART);
    dma_init_struct.periph_inc      = DMA_PERIPH_INCREASE_DISABLE;
    dma_init_struct.periph_width    = DMA_PERIPHERAL_WIDTH_8BIT;
    dma_init_struct.priority        = DMA_PRIORITY_ULTRA_HIGH;
    dma_init(DMA0, DMA_CH3, &dma_init_struct);

    /* initialize DMA0 channel4(Usart0_RX) */
    dma_deinit(DMA0, DMA_CH4);
    dma_struct_para_init(&dma_init_struct);
    dma_init_struct.direction       = DMA_PERIPHERAL_TO_MEMORY;
	dma_init_struct.memory_inc      = DMA_MEMORY_INCREASE_ENABLE;
	dma_init_struct.memory_width    = DMA_MEMORY_WIDTH_8BIT;
	dma_init_struct.periph_addr     = (uint32_t)(&USART_DATA(NET_UART));
	dma_init_struct.periph_inc      = DMA_PERIPH_INCREASE_DISABLE;
	dma_init_struct.periph_width    = DMA_PERIPHERAL_WIDTH_8BIT;
	dma_init_struct.priority        = DMA_PRIORITY_ULTRA_HIGH;
	dma_init(DMA0, DMA_CH4, &dma_init_struct);	


    /* configure DMA mode */
    dma_circulation_disable     (DMA0, DMA_CH3);
    dma_memory_to_memory_disable(DMA0, DMA_CH3);

    dma_circulation_disable     (DMA0, DMA_CH4);
    dma_memory_to_memory_disable(DMA0, DMA_CH4);

    dma_channel_disable (DMA0,DMA_CH3);      //  disable tx dma
    dma_channel_enable  (DMA0,DMA_CH4);       //  enable  rx dma
}


/**
 * @brief   transmit data by dma in GD32
 * @note       
 * @param   usart_periph : USART1£¬USART0
 *          data_buffer  : data need to be transmit
 *          length       : transmit data length
 * @retval  void
 */
void GD_UART_Transmit_DMA(uint32_t usart_periph,uint8_t* data_buffer,uint32_t length)
{
    if(usart_periph == USART0)
	{
		/* Channel disable */
		dma_channel_disable(DMA0, DMA_CH3);
		
		dma_memory_address_config(DMA0, DMA_CH3,(uint32_t)data_buffer);
		dma_transfer_number_config(DMA0,DMA_CH3,length);
       
		/* enable DMA channel to start send */
		dma_channel_enable(DMA0, DMA_CH3);	
	}

	if(usart_periph == USART1)
	{
		/* Channel disable */
		dma_channel_disable(DMA0, DMA_CH6);
		
		dma_memory_address_config(DMA0, DMA_CH6,(uint32_t)data_buffer);
		dma_transfer_number_config(DMA0,DMA_CH6,length);
       		
		/* enable DMA channel to start send */
		dma_channel_enable(DMA0, DMA_CH6);	
	}
}


/**
 * @brief   receive data by dma in GD32
 * @note       
 * @param   usart_periph : USART1£¬USART0
 *          data_buffer  : data need to be receive
 *          length       : receive data length 
 * @retval  void
 */
void GD_UART_Receive_DMA(uint32_t usart_periph,uint8_t* data_buffer,uint32_t length)
{
    if(usart_periph == USART0)
	{
		/* Channel disable */
		dma_channel_disable(DMA0, DMA_CH4);
		
		dma_memory_address_config(DMA0, DMA_CH4,(uint32_t)data_buffer);
		dma_transfer_number_config(DMA0,DMA_CH4,length);
		
		/* enable DMA channel to start send */
		dma_channel_enable(DMA0, DMA_CH4);	
	}

    if(usart_periph == USART1)
	{
		/* Channel disable */
		dma_channel_disable(DMA0, DMA_CH5);
		
		dma_memory_address_config(DMA0, DMA_CH5,(uint32_t)data_buffer);
		dma_transfer_number_config(DMA0,DMA_CH5,length);
		
		/* enable DMA channel to start send */
		dma_channel_enable(DMA0, DMA_CH5);	
	}
}

/*
__attribute__((weak))	void USART1_IRQHandler(void)
{
	if(RESET != usart_interrupt_flag_get(USART1, USART_INT_FLAG_IDLE))
	{	
		// clear USART_INT_FLAG_IDLE 
        usart_interrupt_flag_clear(USART1,USART_INT_FLAG_IDLE);

		usart_data_receive(USART1);
        
		// disable USART2_RX DMA_Channel 
		//dma_channel_disable(DMA0, DMA_CH4); 
         dma_channel_disable(DMA0, DMA_CH5); 

        uart_receive(1);

		// reset DMA_Channel CNT 
        dma_memory_address_config(DMA0, DMA_CH5, UART_RECEIVE_BUF_LENGTH);
		
		
		// enable USART2_RX DMA_Channel 
		//dma_channel_enable(DMA0, DMA_CH4);
        dma_channel_enable(DMA0, DMA_CH5);
	}
}*/

/*
__attribute__((weak))	void USART0_IRQHandler(void)
{
	if(RESET != usart_interrupt_flag_get(USART0, USART_INT_FLAG_IDLE))
	{	
		// clear USART_INT_FLAG_IDLE 
        usart_interrupt_flag_clear(USART1,USART_INT_FLAG_IDLE);

		usart_data_receive(USART0);
        
		// disable USART2_RX DMA_Channel 
		dma_channel_disable(DMA0, DMA_CH4);  

        uart_receive(1);

		// reset DMA_Channel CNT 
        //dma_memory_address_config(DMA0, DMA_CH5, (uint32_t)rs232_rxbuffer);
		dma_transfer_number_config(DMA0, DMA_CH4, UART_RECEIVE_BUF_LENGTH);
		
		// enable USART2_RX DMA_Channel 
		dma_channel_enable(DMA0, DMA_CH4);
	}
}*/




void bsp_uart_hardware_init(void)
{
	bsp_CheckBaud();
	
	uint32_t rs232_baud = g_rs232_baud_buf[g_light_system.uart_baud_gear - 1];
	
    uart_rs232_init(rs232_baud); 
	dma_rs232_init();

    uart_net_init(USART0,19200);
    dma_net_init();
}



