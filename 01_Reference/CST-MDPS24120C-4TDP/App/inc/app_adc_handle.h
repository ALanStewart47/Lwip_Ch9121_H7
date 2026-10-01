/******************************************************************************
 * Copyright (C) 2024 CST, Inc.(Gmbh) or its affiliates.
 * 
 * All Rights Reserved.
 * 
 * @file bsp_adc_handle.h
 * 
 * @par dependencies 
 * - bsp_adc_handle.h
 * 
 * @author Alan | R&D Dept. | CST
 * 
 * @brief Provide the HAL APIs of Bsp.
 * 
 * Processing flow:
 * call directly.
 * 
 * @version V1.0 2025-02-05		
 *
 * @note 1 tab == 4 spaces!
 * 
 *****************************************************************************/

#ifndef __APP_ADC_HANDLE_H__
#define __APP_ADC_HANDLE_H__

#include "main.h"
#include "app.h"

#define NUM_CHANNELS                ADC_LIGHT_CHANNEL_NUM
#define PULL_TIMES                  ADC_PULL_DETECT_TIMES
#define ID_COLLECT_TIMES            ADC_R_COLLECT_NUM

#define ADC_1_NUMBER			1	
#define ADC_2_NUMBER			2


/*	ADC引脚的输入电压	*/
#define	ADC_Ref_Volt			3.3f 

#define OCP_Range				1.3f

#if CHANNEL_NUM == 8
	#define GB_OCP_VOLT_MAX			0.2f
#endif

#if CHANNEL_NUM == 4
	#define GB_OCP_VOLT_MAX			0.125f
#endif

#define	GB_OCP_VALUE_MAX		(uint16_t)(((GB_OCP_VOLT_MAX/ADC_Ref_Volt) * OCP_Range) * 4096.0f +0.5f )


#define ADC_OVER_CURRENT_DETECT_TIMES		150			//检测到过流的次数，到达次数才进入 检测过流状态的判断


#if CHANNEL_NUM == 8
	#define ADC_OVER_CURRENT_VALUE		370			//1.5*248   when light channel is 8, the over current is 0.2V
#endif


#if CHANNEL_NUM == 4
	#define ADC_OVER_CURRENT_VALUE		465			//1.5*310   when light channel is 4 ,the over current is 0.25V
#endif

typedef enum{
	OC_TRUE = 0	,
	OC_FALSE	,
}over_current_state_t;


void bsp_adc_init(void);
void bsp_over_current_detect(void);


#endif //__BSP_ADC_HANDLE_H__


