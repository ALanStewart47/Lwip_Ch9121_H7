/******************************************************************************
 * Copyright (C) 2025 CST, Inc.(Gmbh) or its affiliates.
 *
 * All Rights Reserved.
 *
 * @file bsp_aip650.c
 *
 * @par dependencies
 * - bsp_aip650.h
 *
 * @author Alan | R&D Dept. | CST
 *
 * @brief Provide the HAL APIs of AIP650.
 *
 * Processing flow:
 * call directly.
 *
 * @version    V1.0     2025-05-09      ALan 
 * @note    
 *              1 tab == 4 spaces!
 *
 *****************************************************************************/
#include "bsp_led_indicator.h"
#include "app.h"
							
												
void indicator_led_hard_init(void)
{
    rcu_periph_clock_enable(RCU_GPIOA);
    rcu_periph_clock_enable(RCU_GPIOB);
	
    gpio_init(  MODE_LED_PORT, 
                GPIO_MODE_OUT_PP, 
                GPIO_OSPEED_2MHZ, 
                MODE_LED_PIN);			

    gpio_init(  POWER_LED_PORT, 
                GPIO_MODE_OUT_PP, 
                GPIO_OSPEED_2MHZ, 
                POWER_LED_PIN);	

    gpio_init(  ERROR_LED_PORT, 
                GPIO_MODE_OUT_PP, 
                GPIO_OSPEED_2MHZ, 
                ERROR_LED_PIN);	

    //close led
    gpio_bit_set(MODE_LED_PORT, MODE_LED_PIN);      
    gpio_bit_reset(POWER_LED_PORT, POWER_LED_PIN);    
    gpio_bit_set(ERROR_LED_PORT, ERROR_LED_PIN);
}


void open_led_indicator(uint8_t led_type)
{
    switch(led_type)
    {
        case LED_MODE:
            gpio_bit_reset(MODE_LED_PORT, MODE_LED_PIN);
            break;
        case LED_POWER:
            gpio_bit_reset(POWER_LED_PORT, POWER_LED_PIN);
            break;
        case LED_ERROR:
            gpio_bit_reset(ERROR_LED_PORT, ERROR_LED_PIN);
            break;
        default:
            break;
    }
}


void close_led_indicator(uint8_t led_type)
{
    switch(led_type)
    {
        case LED_MODE:
            gpio_bit_set(MODE_LED_PORT, MODE_LED_PIN);
            break;
        case LED_POWER:
            gpio_bit_set(POWER_LED_PORT, POWER_LED_PIN);
            break;
        case LED_ERROR:
            gpio_bit_set(ERROR_LED_PORT, ERROR_LED_PIN);
            break;
        default:
            break;
    }
}

				
void mode_led_indicator_handle(void)
{
    static uint8_t s_timer = 0;
    uint8_t page = GetMainPage();

    switch (page)
    {
        case PAGE_BRIGHTNESS_NORMAL:
            close_led_indicator(LED_MODE);  
            break;

        case PAGE_STROBE_WIDTH:
            if (s_timer < 5)               
                open_led_indicator(LED_MODE);
            else                           
                close_led_indicator(LED_MODE);
            if (++s_timer > 9)
                s_timer = 0;
            break;

        case PAGE_WORK_MODE:
                open_led_indicator(LED_MODE);
            break;

        default:
            //close_led_indicator(LED_MODE);
            break;
    }
}



void error_led_indicator_handle(void)
{
    uint8_t alarm = get_light_alarm();

    if(alarm != STATUS_NORMAL)
    {
        open_led_indicator(LED_ERROR);
    }
    else
    {
        close_led_indicator(LED_ERROR);
    }
}

