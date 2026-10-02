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
 * @brief Provide the HAL APIs of the pwm by timer
 *
 * Processing flow:
 * call directly.
 *
 * @version    V1.0     2025-05-07      ALan
 *             V1.1
 * @note    
 *              1 tab == 4 spaces!
 *
 *****************************************************************************/
#include "bsp_tim_pwm.h"
#include "app_light.h"

extern TIM_HandleTypeDef htim3;


#if STM32
TIM_TypeDef * user_pwm_timers[USER_TIM_NUM] = {TIM3};
uint32_t user_pwm_channel[USER_TIM_PWM_NUM] = {TIM_CHANNEL_1,TIM_CHANNEL_2,TIM_CHANNEL_3,TIM_CHANNEL_4};
#endif


 /**
 * @brief	timer pwm output init
 * @note	pwm frequency is 10KHz£¬ io no remap
 * @param	timer_periph £º TIMER0,TIMER1,TIMER2 
 * @retval	void
 */

tim_status_t bsp_tim_pwm_init(uint32_t timer_periph)
{
    LL_TIM_EnableAllOutputs(TIM3);
	LL_TIM_EnableCounter(TIM3);   
    return TIM_OK;
}


 /**
 * @brief	config pwm pulse value 
 * @note	
 * @param	pwm_num: PWM_1 ~ PWM_8 , value: 0~999
 * @retval	tim_status_t
 */
tim_status_t bsp_pwm_pulse_value_config(uint8_t pwm_num , uint32_t value)
{
    if(pwm_num >= USER_TIM_PWM_NUM)
        return TIM_ERROR;

    if(value > USER_PWM_FREQ)
        return TIM_ERROR;

#if GD32
    uint8_t s_timer_number = 0;

    if(pwm_num < PWM_5){ 
        s_timer_number = 0;
    }
    else if(pwm_num < USER_TIM_PWM_NUM){ 
        s_timer_number = 1;
    }

    timer_channel_output_pulse_value_config(user_pwm_timers[s_timer_number],
                                            user_pwm_channel[pwm_num],
                                            value);
#endif
#if STM32
    if(pwm_num >= USER_TIM_PWM_NUM)
        return TIM_ERROR;

   // __HAL_TIM_SetCompare(&htim3, user_pwm_channel[pwm_num], value);
	switch(pwm_num)
	{
		case PWM_1:
				LL_TIM_OC_SetCompareCH1(TIM3,value);
		break;
		
		case PWM_2:
				LL_TIM_OC_SetCompareCH2(TIM3,value);
		break;
		
		case PWM_3:
			LL_TIM_OC_SetCompareCH3(TIM3,value);
		break;
		
		case PWM_4:
				LL_TIM_OC_SetCompareCH4(TIM3,value);
		break;
		default:
			break;
	}
    return TIM_OK;
#endif	
}


 /**
 * @brief	config pwm pulse value 
 * @note	
 * @param	pwm_num: PWM_1 ~ PWM_8 , value: 0~999   mode: 0- 0-999 , 1- 0-255
 * @retval	tim_status_t
 */
tim_status_t bsp_dps_light_value_config(uint8_t pwm_num , uint8_t mode ,uint32_t value)
{

    if(pwm_num > PWM_8)
        return TIM_ERRORPARAMETER;

    if(mode > BRIGHTNESS_255)
        return TIM_ERRORPARAMETER;

    uint32_t temp_value = 0;
	uint32_t max_value = 0;

	if(mode == BRIGHTNESS_999){
		max_value = 999;
        if(value > LIGHT_VALUE_999_MAX){
            value = LIGHT_VALUE_999_MAX;
        }
    }
	if(mode == BRIGHTNESS_255){
		max_value = 255;
        if(value > LIGHT_VALUE_255_MAX){
            value = LIGHT_VALUE_255_MAX;
        }
    }
	
	temp_value = (value * USER_PWM_FREQ) / max_value; 
	if(bsp_pwm_pulse_value_config(pwm_num,temp_value) == TIM_ERROR)
    {
        return TIM_ERROR;
    }
	return TIM_OK;
}


tim_status_t bsp_pwm_enable(uint8_t pwm_num)
{
    if(pwm_num >= USER_TIM_PWM_NUM)return TIM_ERROR;
#if GD32
    uint8_t s_timer_number = 0;
    if(pwm_num  < PWM_5){
        s_timer_number = 0;
    }
    else if(pwm_num < USER_TIM_PWM_NUM){
        s_timer_number = 1;
    }                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                
    timer_channel_output_state_config(  user_pwm_timers[s_timer_number],
                                        user_pwm_channel[pwm_num],
                                        TIMER_CCX_ENABLE);
    return TIM_OK;
#endif
#if STM32
    //HAL_TIM_PWM_Start(&htim3, user_pwm_channel[pwm_num]); 
	switch(pwm_num)
	{
		case PWM_1:
			LL_TIM_CC_EnableChannel(TIM3,LL_TIM_CHANNEL_CH1);
		break;
		case PWM_2:
			LL_TIM_CC_EnableChannel(TIM3,LL_TIM_CHANNEL_CH2);
		break;
		case PWM_3:
			LL_TIM_CC_EnableChannel(TIM3,LL_TIM_CHANNEL_CH3);
		break;
		case PWM_4:
			LL_TIM_CC_EnableChannel(TIM3,LL_TIM_CHANNEL_CH4);
		break;
		default:
			break;
	}	
    return TIM_OK;
#endif
}
                                                                                                                            

tim_status_t bsp_pwm_disable(uint8_t pwm_num)
{
    if(pwm_num >= USER_TIM_PWM_NUM)return TIM_ERROR;
#if GD32
    uint8_t s_timer_number = 0;

    if(pwm_num < PWM_5){
        s_timer_number = 0;
    }
    else if(pwm_num < USER_TIM_PWM_NUM){
        s_timer_number = 1;
    }

    timer_channel_output_state_config(  user_pwm_timers[s_timer_number],
                                        user_pwm_channel[pwm_num],
                                      TIMER_CCX_DISABLE);
    return TIM_OK;
#endif
#if STM32
   // HAL_TIM_PWM_Stop(&htim3, user_pwm_channel[pwm_num]); 
	switch(pwm_num)
	{
		case PWM_1:
			LL_TIM_CC_DisableChannel(TIM3,LL_TIM_CHANNEL_CH1);
		break;
		
		case PWM_2:
			LL_TIM_CC_DisableChannel(TIM3,LL_TIM_CHANNEL_CH2);
		break;
		
		case PWM_3:
			 LL_TIM_CC_DisableChannel(TIM3,LL_TIM_CHANNEL_CH3);
		break;
		
		case PWM_4:
			 LL_TIM_CC_DisableChannel(TIM3,LL_TIM_CHANNEL_CH4);
		break;
		default:
			break;
	}	
    return TIM_OK;
#endif
}

void bsp_all_pwm_disable(void)
{
    for(uint8_t i = 0; i < USER_TIM_PWM_NUM; i++)
    {
        bsp_pwm_disable(i);
    }
}

