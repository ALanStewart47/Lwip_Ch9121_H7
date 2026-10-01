/******************************************************************************
 * Copyright (C) 2025 CST, Inc.(Gmbh) or its affiliates.
 *
 * All Rights Reserved.
 *
 * @file bsp_trig.c
 *
 * @par dependencies
 * - bsp_trig.h
 *
 * @author Alan | R&D Dept. | CST
 *
 * @brief Provide the HAL APIs of the strobe trig.
 *
 * Processing flow:
 * call directly.
 *
 * @version 		V1.0 		2025-07-18   	ALan 
 * @note saving data to flash
 *       1 tab == 4 spaces!
 *
 *****************************************************************************/

//******************************** Includes *********************************//
#include "app_trig.h"
#include "app_prog_trig.h"
#include "bsp_trig_gpio.h"
#include "app_light.h"
//******************************** Includes *********************************//

//******************************** Defines **********************************//
#define LIGHT_CHANNEL_MAX		CHANNEL_NUM
#if GD32
	extern uint32_t g_trig_pin[];
	extern uint32_t g_trig_port[];
#endif 
#if STM32
	extern uint16_t g_trig_pin[];
	extern GPIO_TypeDef * g_trig_port[];
#endif
extern system_param_t g_light_system; 

void bsp_trig_fast_handle(uint8_t _channel);
trig_status_t start_cam_out_timer(uint8_t _channel, uint8_t _uint, uint32_t _time);
//******************************** Defines **********************************//


//trig strobe inerrupt callback function
void strobe1_CallBack(void )
{
	bsp_light_off(CH1);
	g_light_system.channel[CH1].strobe_busy = STROBE_TRIG_STATE_IDLE;
}

void strobe2_CallBack(void )
{
	bsp_light_off(CH2);
	g_light_system.channel[CH2].strobe_busy = STROBE_TRIG_STATE_IDLE;
}

void strobe3_CallBack(void )
{
	bsp_light_off(CH3);
	g_light_system.channel[CH3].strobe_busy = STROBE_TRIG_STATE_IDLE;
}

void strobe4_CallBack(void )
{
	bsp_light_off(CH4);
	g_light_system.channel[CH4].strobe_busy = STROBE_TRIG_STATE_IDLE;
}


void camout1_CallBack(void )
{
	bsp_cam_output_close_in_strobe(CH1);
}

void camout2_CallBack(void )
{
	bsp_cam_output_close_in_strobe(CH2);
}

void camout3_CallBack(void )
{
	bsp_cam_output_close_in_strobe(CH3);
}

void camout4_CallBack(void )
{
	bsp_cam_output_close_in_strobe(CH4);
}


/*********************************************************************** */

/**
 * @brief	start strobe timer
 * @note    This function starts a strobe timer for a specific channel.
 * @param   _channel: The channel number (0-7) for which the strobe timer is started.
 * @param   _uint: The unit of time for the strobe (e.g., STROBE_UNIT_1MS).
 * @param   _time: The duration of the strobe in the specified unit. 
 * @retval  TRIG_OK on success, TRIG_ERROR on failure.
 */
trig_status_t start_strobe_timer(uint8_t _channel, uint8_t _uint, uint32_t _time)
{
	//uint32_t _trig_open_time[8] = {0};
	uint32_t temp_time = 0;
	if(_channel >= STORBE_NUM_MAX)
		return TRIG_ERROR;

	if(_time > 999 || _time == 0)
		return TRIG_ERROR;

	if(_uint > STROBE_UNIT_1MS)
		return TRIG_ERROR;

	//temp_time = _time * 10; // the timer unit is 100us 
	temp_time = _time * 10000;	  // the timer unit is 0.1us 

	switch(_channel)
	{
		case CH1:
			bsp_tim2_start_cc(1, temp_time, strobe1_CallBack);
			//bsp_tim2_start_cc(1, temp_time, strobe1_CallBack);
		break;

		case CH2:
			bsp_tim2_start_cc(2, temp_time, strobe2_CallBack);
			//bsp_tim2_start_cc(2, temp_time, strobe2_CallBack);
		break;

		case CH3:
			bsp_tim2_start_cc(3, temp_time, strobe3_CallBack);
			//bsp_tim2_start_cc(3, temp_time, strobe3_CallBack);
		break;

		case CH4:
			bsp_tim2_start_cc(4, temp_time, strobe4_CallBack);
			//bsp_tim2_start_cc(4, temp_time, strobe4_CallBack);
		break;

		default:
			break;
	}
	return TRIG_OK;
}

trig_status_t start_cam_out_timer(uint8_t _channel, uint8_t _uint, uint32_t _time)
{
	//uint32_t _trig_open_time[8] = {0};
	uint32_t temp_time = 0;
	if(_channel >= STORBE_NUM_MAX)
		return TRIG_ERROR;

	if(_time > 999 || _time == 0)
		return TRIG_ERROR;

	if(_uint > STROBE_UNIT_1MS)
		return TRIG_ERROR;

	//temp_time = _time * 10; // the timer unit is 100us 
	//temp_time = _time * 10000;	  // the timer unit is 0.1us 
	temp_time = _time * 1000;	  // the timer unit is 1us

	switch(_channel)
	{
		case CH1:
			bsp_tim1_start(1, temp_time, camout1_CallBack);
		break;

		case CH2:
			bsp_tim1_start(2, temp_time, camout2_CallBack);
		break;

		case CH3:
			bsp_tim1_start(3, temp_time, camout3_CallBack);
		break;

		case CH4:
			bsp_tim1_start(4, temp_time, camout4_CallBack);
		break;
		default:
			break;
	}
	return TRIG_OK;
}


bool get_tirg_gpio_state(uint8_t _channel)
{
	if(_channel >= LIGHT_CHANNEL_MAX)
		return false;
#if GD32
	if(gpio_input_bit_get(g_trig_port[_channel], g_trig_pin[_channel])  ==  SET)
#elif STM32
	if(HAL_GPIO_ReadPin(g_trig_port[_channel], g_trig_pin[_channel]) == GPIO_PIN_SET)
#endif
	{
		return false;
	}
	else
	{
		return true;
	}
}

void bsp_trig_fast_handle(uint8_t _channel)
{
	bsp_light_on				 (_channel);
	bsp_light_out_add_count		 (_channel);
	bsp_cam_output_open_in_strobe(_channel);
	start_strobe_timer			 (_channel, STROBE_UNIT_1MS , g_light_system.channel[_channel].strobe_width);
	start_cam_out_timer			 (_channel, STROBE_UNIT_1MS, g_light_system.channel[_channel].camout_width);

}

void strobe_trig_callback(uint8_t channel)
{
	if( g_light_system.work_mode != WORK_MODE_STROBE )return;
	if(channel >= LIGHT_CHANNEL_MAX )return;
	if(get_light_value(channel) == 0)return;
	if(get_light_strobe_width(channel) == 0)return;

	if(get_tirg_gpio_state(channel) == true) 			
	{
		if(g_light_system.channel[channel].strobe_busy == STROBE_TRIG_STATE_IDLE)
		{
			g_light_system.channel[channel].strobe_busy = STROBE_TRIG_STATE_WORK;
			bsp_trig_fast_handle(channel);
		}
	}
}


void soft_trig_mode_handle(void)
{
    if( get_light_mode() != WORK_MODE_STROBE )return;

    for(uint8_t i = 0; i < LIGHT_CHANNEL_MAX; i++)
    {
		if(g_light_system.channel[i].soft_trig == SOFT_TRIGGER_REQUEST)
		{
			g_light_system.channel[i].soft_trig = NO_SOFT_TRIGGER_REQUEST;

			if(get_light_value(i) == 0)continue;

            uint16_t width = 0;
            if (g_light_system.channel[i].soft_trig_width > 0) 
			{
                width = g_light_system.channel[i].soft_trig_width;
                g_light_system.channel[i].soft_trig_width = 0; 
            } 
			else 
			{
                width = get_light_strobe_width(i);
            }
			if(width == 0)continue;

			if(g_light_system.channel[i].strobe_busy == STROBE_TRIG_STATE_IDLE)
			{
				g_light_system.channel[i].strobe_busy = STROBE_TRIG_STATE_WORK;
				bsp_trig_fast_handle(i);
			}
		}
    }
}

void bsp_trig_soft_trig(uint8_t channel, uint16_t width)
{
    if (g_light_system.work_mode != WORK_MODE_STROBE) return;
    if (channel >= LIGHT_CHANNEL_MAX) return;
    if (g_light_system.channel[channel].strobe_busy == STROBE_TRIG_STATE_WORK) return;
	
	bsp_trig_add_count(channel);
    if (get_light_value(channel) == 0) return;
    if (get_light_strobe_width(channel) == 0 && width == 0) return;

    if (width > 0) {
        g_light_system.channel[channel].soft_trig_width = width;
    }

    g_light_system.channel[channel].soft_trig = SOFT_TRIGGER_REQUEST;
}


void bsp_all_trig_soft_trig(uint16_t  width)
{
	if (g_light_system.work_mode != WORK_MODE_STROBE) return;

	for (uint8_t i = 0; i < LIGHT_CHANNEL_MAX; i++) {
		bsp_trig_soft_trig(i, width);
	}
}


#if STM32
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
/*
	if(GPIO_Pin == g_trig_pin[i])
	{
		bsp_trig_callback(i);
	}
*/	
}
#endif

//temp code 
void fast_trig_isr(uint8_t ch)
{
    // 假设只在 STROBE 模式且参数已预先验证，直接执行
    if(g_light_system.work_mode == WORK_MODE_STROBE &&
       g_light_system.channel[ch].strobe_busy == STROBE_TRIG_STATE_IDLE &&
       g_light_system.channel[ch].brightness &&
       g_light_system.channel[ch].strobe_width)
    {
        g_light_system.channel[ch].strobe_busy = STROBE_TRIG_STATE_WORK;
        bsp_light_on(ch);
        start_strobe_timer(ch, STROBE_UNIT_1MS, g_light_system.channel[ch].strobe_width);
    }
}


