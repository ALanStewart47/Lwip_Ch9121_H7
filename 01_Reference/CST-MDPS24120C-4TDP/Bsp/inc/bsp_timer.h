/******************************************************************************
 * Copyright (C) 2025 CST, Inc.(Gmbh) or its affiliates.
 *
 * All Rights Reserved.
 *
 * @file bsp_timer.h
 *
 * @par dependencies
 * 
 *
 * @author Alan | R&D Dept. | CST
 *
 * @brief Provide the HAL APIs of the timer
 *
 *	Implemented multiple software timers for the main program (accuracy: 1ms), 
 *	the number of timers can be increased or decreased by modifying TMR_COUNT.
 *	Implemented millisecond-level delay functions (accuracy: 1ms) and microsecond-level delay functions.
 *	Implemented system runtime function (unit: 1ms).
 *	Implemented TIM (optional) hardware timer interrupt for microsecond-level timing.
 *
 * @version    V1.0     2025-09-08      ALan 
 * @note    
 *              1 tab == 4 spaces!
 *
 *****************************************************************************/
#ifndef __BSP_TIMER_H
#define __BSP_TIMER_H

//******************************** Includes *********************************//
#include "main.h"
//#include "gd32f30x.h"
#include "app.h"
//******************************** Includes *********************************//

//******************************** Defines **********************************//

// Define several global software timer variables here.
// Note: mark them with __IO (volatile), because these variables are accessed from both interrupts and the main program; 
// otherwise compiler optimizations may cause incorrect
#define TMR_COUNT	6		// soft timer number£¨timer ID range : 0-5)

// TIM3  TIM4 is used to strobe the light
#if GD32
#define PROG_TRIG_TIMER				TIMER1
#define TIM_STROBE_1_4				TIMER3
#define TIM_STROBE_5_8				TIMER4
#endif


#define BSP_SINGLE_HW_TIMER     1

#if STM32
#if BSP_SINGLE_HW_TIMER
#define PROG_TRIG_TIMER   			TIM2
#define TIM_STROBE_1_4    			TIM2
#define TIM_STROBE_5_8    			TIM2
#define TIM_CAM_1_4       			TIM2
#else
#define PROG_TRIG_TIMER				TIM2
#define TIM_STROBE_1_4				TIM1
#define TIM_STROBE_5_8				TIM4
#define TIM_CAM_1_4					TIM8
#endif
#endif

enum
{
	ADC_TMR 			= 	0,		
	IWDG_TMR			=	1,		
	FLASH_BACKUP_TMR	=	2,		
	CHECK_POWER_TMR		=	3,		
	MODE_LED_TMR		=	4
};

typedef enum
{
	TMR_ONCE_MODE 		= 0,		
	TMR_AUTO_MODE 		= 1		
}TMR_MODE_E;

// Timer struct: member variables must be declared volatile; 
// otherwise compiler optimizations may cause incorrect behavior 
typedef struct
{
	volatile uint8_t Mode;		
	volatile uint8_t Flag;		
	volatile uint32_t Count;	
	volatile uint32_t PreLoad;	// Preload value for the counter 
}SOFT_TMR;

typedef enum
{
	TRIG_DELAY_1US		=	0,
	TRIG_DELAY_10US		=	1,
	TRIG_DELAY_100US	=	2,
	TRIG_DELAY_1MS		=	3,
	TRIG_DELAY_100NS	=	4,
}trig_delay_status_t;
//******************************** Defines **********************************//


//******************************** Declaring ********************************//
void 	bsp_InitTimer(void);
void 	bsp_DelayMS(uint32_t n);
void 	bsp_DelayUS(uint32_t n);
void 	bsp_StartTimer(uint8_t _id, uint32_t _period);
void 	bsp_StartAutoTimer(uint8_t _id, uint32_t _period);
void 	bsp_StopTimer(uint8_t _id);
void 	bsp_systick_handler(void);
uint8_t bsp_CheckTimer(uint8_t _id);
int32_t bsp_GetRunTime(void);
int32_t bsp_CheckRunTime(int32_t _LastTime);
uint8_t bsp_Check1msTask(void);
uint8_t bsp_Check10msTask(void);

//void bsp_InitHardTimer(void);
//void bsp_StartHardTimer(uint8_t _CC, uint32_t _uiTimeOut, void * _pCallBack);
//0.2
#if STM32
void bsp_InitTrigDelayTimer(TIM_TypeDef *TIMx, IRQn_Type s_TIM_IRQ , uint8_t t_delay);
#else
void bsp_InitTrigDelayTimer(uint32_t timer_periph, IRQn_Type s_TIM_IRQ , uint8_t t_delay);
#endif
void bsp_tim2_start_cc(uint8_t _CC, uint32_t _uiTimeOut, void * _pCallBack);
void bsp_start_strobe5_8_timer(uint8_t _CC, uint32_t _uiTimeOut, void * _pCallBack);
void bsp_tim4_start(uint8_t cc, uint32_t delta_us, void *cb);
//void tim_prog_callback(void);
void bsp_init_multi_timer(void);
void bsp_tim1_start(uint8_t cc, uint32_t delta_us, void *cb);

//******************************** Declaring ********************************//
#endif //__BSP_TIMER_H


