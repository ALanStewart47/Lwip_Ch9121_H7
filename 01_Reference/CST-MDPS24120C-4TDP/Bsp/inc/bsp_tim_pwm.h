/******************************************************************************
 * Copyright (C) 2025 CST, Inc.(Gmbh) or its affiliates.
 *
 * All Rights Reserved.
 *
 * @file bsp_tim_pwm.c
 *
 * @par dependencies
 * - bsp_tim_pwm.h
 *
 * @author Alan | R&D Dept. | CST
 *
 * @brief Provide the HAL APIs of the pwm by timer
 *
 * Processing flow:
 * call directly.
 *
 * @version    V1.0     2025-04-28      ALan 
 * @note    
 *              1 tab == 4 spaces!
 *
 *****************************************************************************/
#ifndef __BSP_TIM_PWM_H__
#define __BSP_TIM_PWM_H__

//******************************** Includes *********************************//
#include "main.h"
//#include "gd32f30x.h"
#include "app.h"
//******************************** Includes *********************************//

//******************************** Defines **********************************//

/*  º¯Êý·µ»Ø×´Ì¬Ã¶¾Ù                    */
typedef enum
{
  TIM_OK                = 0,           /* Operation completed successfully.  */
  TIM_ERROR             = 1,           /* Run-time error without case matched*/
  TIM_ERRORTIMEOUT      = 2,           /* Operation failed with timeout      */
  TIM_ERRORRESOURCE     = 3,           /* Resource not available.            */
  TIM_ERRORPARAMETER    = 4,           /* Parameter error.                   */
  TIM_ERRORNOMEMORY     = 5,           /* Out of memory.                     */
  TIM_ERRORISR          = 6,           /* Not allowed in ISR context         */
  TIM_RESERVED          = 0x7FFFFFFF   /* Reserved                           */
}tim_status_t;

typedef enum
{
    PWM_1   = 0,
    PWM_2   = 1,
    PWM_3      ,
    PWM_4      ,
    PWM_5      ,
    PWM_6      ,
    PWM_7      ,
    PWM_8      
}pwm_channel_t;


typedef enum
{
  PWM_VALUE_999 = 0,
  PWM_VALUE_255 = 1,
}pwm_output_value_t;

//typedef tim_status_t (*bsp_dps_light_value_config)(pwm_channel_t pwm_num , uint8_t mode ,uint32_t value);
//******************************** Defines ********************************//

//******************************** Declaring ********************************//
#if GD32
tim_status_t bsp_pwm_pulse_value_config(uint8_t pwm_num , uint32_t value);
#endif
tim_status_t bsp_tim_pwm_init(uint32_t timer_periph);
tim_status_t bsp_dps_light_value_config(uint8_t pwm_num , uint8_t mode ,uint32_t value);

tim_status_t bsp_pwm_enable(uint8_t pwm_num);
tim_status_t bsp_pwm_disable(uint8_t pwm_num);
void bsp_all_pwm_disable(void);
//******************************** Declaring ********************************//

#endif


