/******************************************************************************
 * Copyright (C) 2025 CST, Inc.(Gmbh) or its affiliates.
 *
 * All Rights Reserved.
 *
 * @file bsp_trig_gpio.h
 *
 * @par dependencies
 * -
 *
 * @author Alan | R&D Dept. | CST
 *
 * @brief Provide the HAL APIs of the trig
 *
 * Processing flow:
 * call directly.
 *
 * @version    V1.0     2025-05-09      ALan 
 * @note    
 *              1 tab == 4 spaces!
 *
 *****************************************************************************/
#ifndef __BSP_TRIG_GPIO_H__
#define __BSP_TRIG_GPIO_H__

//******************************** Includes *********************************//
#include "main.h"
//#include "gd32f30x.h"
#include "app.h"
//******************************** Includes *********************************//

//******************************** Defines **********************************//
#define USER_TRIG_NUM       4

//#define TRIG_1_GPIO_PORT            GPIOA
//#define TRIG_1_GPIO_PIN             GPIO_PIN_4

//#define TRIG_2_GPIO_PORT            GPIOA
//#define TRIG_2_GPIO_PIN             GPIO_PIN_5

//#define TRIG_3_GPIO_PORT            GPIOB
//#define TRIG_3_GPIO_PIN             GPIO_PIN_15

#define CAM_OUT_GPIO_PORT            GPIOB
#define CAM_OUT_GPIO_PIN             GPIO_PIN_11

typedef void (*trig_callback_t)(uint8_t channel);
//extern exti_line_enum user_trig_exti_line[USER_TRIG_NUM] ;

/*  º¯Êý·µ»Ø×´Ì¬Ã¶¾Ù                    */
typedef enum
{
  TRIG_OK                = 0,           /* Operation completed successfully.  */
  TRIG_ERROR             = 1,           /* Run-time error without case matched*/
  TRIG_ERRORTIMEOUT      = 2,           /* Operation failed with timeout      */
  TRIG_ERRORRESOURCE     = 3,           /* Resource not available.            */
  TRIG_ERRORPARAMETER    = 4,           /* Parameter error.                   */
  TRIG_ERRORNOMEMORY     = 5,           /* Out of memory.                     */
  TRIG_ERRORISR          = 6,           /* Not allowed in ISR context         */
  TRIG_RESERVED          = 0x7FFFFFFF   /* Reserved                           */
}trig_status_t;

typedef enum
{
    TRIG_1   = 0,
    TRIG_2   = 1,
    TRIG_3      ,
    TRIG_4      ,
    TRIG_5      ,
    TRIG_6      ,
    TRIG_7      ,
    TRIG_8      
}trig_num_t;

typedef enum
{
  RISING_FALLING_MODE = 0 ,
  RISING_MODE             ,
  FALLING_MODE
}exti_mode_t;

//******************************** Defines ********************************//

//******************************** Declaring ********************************//
trig_status_t bsp_trig_gpio_init(uint8_t trig_num);
trig_status_t bsp_trig_exti_init(uint8_t trig_num, exti_mode_t exti_mode);
trig_status_t bsp_trig_init(exti_mode_t exti_mode  );
void trig_interrupt_enable(void);
trig_status_t bsp_trig_mode_config(trig_num_t trig_num, exti_mode_t exti_mode);
trig_status_t bsp_trig_register_callback(uint8_t trig_num, trig_callback_t callback);
trig_status_t bsp_trig_callback(trig_num_t num);

trig_status_t bsp_trig_init_in_falling(void );

trig_status_t bsp_cam_out_init(void);
trig_status_t bsp_cam_output_enable(void);
trig_status_t bsp_cam_output_disable(void);

trig_status_t bsp_cam_output_open_in_strobe(uint8_t channel);
trig_status_t bsp_cam_output_close_in_strobe(uint8_t channel);

uint32_t bsp_trig_get_count(uint8_t num);
void bsp_trig_add_count(uint8_t num);
void bsp_trig_clear_all(void);

uint32_t bsp_cam_get_count(uint8_t num);
void bsp_cam_add_count(uint8_t num);
void bsp_cam_clear_all(void);

uint32_t bsp_light_output_get_count(uint8_t num);
void bsp_light_out_add_count(uint8_t num);
void bsp_light_output_clear_all(void);
//******************************** Declaring ********************************//

#endif


