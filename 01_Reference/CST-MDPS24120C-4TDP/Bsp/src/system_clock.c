/******************************************************************************
 * Copyright (C) 2025 CST, Inc.(Gmbh) or its affiliates.
 *
 * All Rights Reserved.
 *
 * @file system_clock.c
 *
 * @par dependencies
 * - system_clock.h
 *
 * @author Alan | R&D Dept. | CST
 *
 * @brief Provide the HAL APIs of clock handle
 *
 * Processing flow:
 * call directly.
 *
 * @version    V1.0     2025-04-29      ALan 
 * @note    
 *              1 tab == 4 spaces!
 *
 *****************************************************************************/
#include "system_clock.h"

#include "bsp_uart_dma.h"

char sys_txbuffer[20] = {0};


/**
 * @brief   peripheral clock init
 * @note    DMA0   CH3 CH4    
 * @param  
 * @retval  void
 */
void peripheral_clock_init(void)
{
    //alternate function clock
    rcu_periph_clock_enable(RCU_AF);

    //I/O compensation cell
    GPIO_COMPENSATION_ENABLE;
	
    //GPIO
	rcu_periph_clock_enable(RCU_GPIOA);
	rcu_periph_clock_enable(RCU_GPIOB);
	rcu_periph_clock_enable(RCU_GPIOC);
	rcu_periph_clock_enable(RCU_GPIOD);

    //USART
    rcu_periph_clock_enable(RCU_USART0);
    rcu_periph_clock_enable(RCU_USART1);

    //Timer
    rcu_periph_clock_enable(RCU_TIMER0);
    

    //Systick 
    systick_config();
}

/**
 * @brief   get reset flag
 * @note    
 * @param  
 * @retval  void
 */
uint8_t get_reset_flag(void )
{
	uint8_t reset_flag = 0;
    //reset PIN reset 
    if(RESET != rcu_flag_get(RCU_FLAG_EPRST))
    {
        reset_flag = EPRST;
        
    }
    //power reset
    if(RESET != rcu_flag_get(RCU_FLAG_PORRST))
    {
        reset_flag = PORRST;

    }
    //software reset 
    if(RESET != rcu_flag_get(RCU_FLAG_SWRST))
    {
        reset_flag = SWRST;

    }

    // FWDGT reset 
    if(RESET != rcu_flag_get(RCU_FLAG_FWDGTRST))
    {
        reset_flag = FWDGTRST;

    }
  
    // WWDGT reset 
    if(RESET != rcu_flag_get(RCU_FLAG_WWDGTRST))
    {
        reset_flag = WWDGTRST;

    }

    // low-power reset
    if(RESET != rcu_flag_get(RCU_FLAG_LPRST))
    {
        reset_flag = LPRST;

    }


	sprintf(sys_txbuffer,"reset in %d\r\n",reset_flag);

	GD_UART_Transmit_DMA(USART1,(uint8_t *)sys_txbuffer,strlen(sys_txbuffer));

    /* clear all reset flags */
    rcu_all_reset_flag_clear();

    return reset_flag;
}



