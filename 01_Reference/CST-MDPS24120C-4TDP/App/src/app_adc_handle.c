/******************************************************************************
 * Copyright (C) 2024 CST, Inc.(Gmbh) or its affiliates.
 * 
 * All Rights Reserved.
 * 
 * @file bsp_adc_handle.c
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
//******************************** Includes *********************************//
#include "app_adc_handle.h"
#include "app_uart.h"
#include "app.h"
//******************************** Includes *********************************//

//******************************** Defines **********************************//
float g_mcu_temperature;

//extern ADC_HandleTypeDef hadc1;
extern ADC_HandleTypeDef hadc2;

uint16_t g_adc_value[3] = {0};
uint16_t g_dac_output_adjustable_value = 20;
uint16_t g_dac_output_max = 0 ;

//uint16_t g_adc_1_values[ADC_1_NUMBER]	=	{0};
uint16_t g_adc_2_values[ADC_2_NUMBER]	=	{0};
//******************************** Defines **********************************//

//******************************** Declaring ********************************//
 void BubbleSort(uint16_t a[],uint16_t m)
{
    int i = 0,j = 0;
    int t = 0;
    for(i=0;i<m-1;i++)
    {
        for(j=0;j<m-i-1;j++)
        {
            if(a[j]>a[j+1])
            {
               t=a[j+1];
               a[j+1]=a[j];
               a[j]=t;
            }
        }
    }
    return;
}

uint32_t fliter_calucation( uint16_t a[],uint16_t m )
{
	uint16_t i = 0;
	uint32_t sum = 0;
	BubbleSort(a,m);
	i = m/2;

	sum = a[i] + a[i+1] + a[i+2] + a[i+3] + a[i+4] + a[i+5] + a[i+6] + a[i+7] + a[i+8] + a[i+9];

	sum = sum / 10;
	
	return sum;
}

/*
* @brief  adc init
* @note   adc init and start dma
* @param  void
* @retval void
*/
void bsp_adc_init(void)
{
	if (HAL_ADCEx_Calibration_Start(&hadc2, ADC_SINGLE_ENDED) != HAL_OK){
		Error_Handler();
	}
	HAL_ADC_Start_DMA(&hadc2 ,(uint32_t *)g_adc_2_values ,sizeof(g_adc_2_values)/sizeof(uint16_t));
}

/*
* @brief  adc handle
* @note   adc handle and filter the adc value
* @param  void
* @retval void
*/
void bsp_over_current_detect(void)
{
	static uint16_t err_cnt = 0;

	if(g_adc_2_values[0] > GB_OCP_VALUE_MAX)
	{
		if(++err_cnt > ADC_OVER_CURRENT_DETECT_TIMES)
		{
			set_light_alarm(STATUS_OVERCURRENT);
		}
	}
	else
	{
		err_cnt = 0;
	}
}


//******************************** Declaring ********************************//



