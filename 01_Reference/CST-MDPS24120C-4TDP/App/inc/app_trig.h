/******************************************************************************
 * Copyright (C) 2025 CST, Inc.(Gmbh) or its affiliates.
 *
 * All Rights Reserved.
 *
 * @file bsp_trig.h
 *
 * @par dependencies
 * - bsp.h
 *
 * @author Alan | R&D Dept. | CST
 *
 * @brief Provide the HAL APIs of the trig.
 *
 * Processing flow:
 * call directly.
 *
 * @version 		V1.0 		2025-05-20 * 
 * @note saving data to flash
 *       1 tab == 4 spaces!
 *
 *****************************************************************************/
#ifndef	__APP_TRIG_H
#define	__APP_TRIG_H
/******************************** Includes *********************************/
#include "app.h"
/******************************** Includes *********************************/

//******************************** Defines **********************************//


#define STORBE_NUM_MAX			CHANNEL_NUM

typedef enum
{
	STROBE_UNIT_1MS = 0,
}strobe_writhunit_s;

typedef struct
{
	uint8_t timer_id;
	void (*timer_callback)(void);
}trig_timer_callback_t;
//******************************** Defines **********************************//

//******************************** Declaring ********************************//
void bsp_trig_fast_handle(uint8_t _channel);
void soft_trig_mode_handle(void);

void strobe_trig_callback(uint8_t channel);
void soft_trig_mode_handle(void);

void bsp_trig_soft_trig(uint8_t channel, uint16_t width);
//void trig_delay_tack(void);
//void trig_open_light_tack(void);
//void trig_config_preserve(void);
void fast_trig_isr(uint8_t ch);
void bsp_all_trig_soft_trig(uint16_t  width);
//******************************** Declaring ********************************//
#endif

