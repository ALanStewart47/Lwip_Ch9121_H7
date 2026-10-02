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
 * @brief Provide the HAL APIs of the program trig.
 *
 * Processing flow:
 * call directly.
 *
 * @version 		    V1.0 		    2025-06-11   ALan
 *                  V1.1        2025-06-12   ALan
 *                  V1.2        2025-07-23   ALan
 * @note saving data to flash
 *       1 tab == 4 spaces!
 *
 *****************************************************************************/
#ifndef	__BSP_PROG_TRIG_H__
#define	__BSP_PROG_TRIG_H__

//******************************** Includes *********************************//
#include "main.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "stdbool.h"

#include "app_light.h"
//******************************** Includes *********************************//

//******************************** Defines **********************************//
typedef enum
{
  PROG_OK                = 0,           /* Operation completed successfully.  */
  PROG_ERROR             = 1,           /* Run-time error without case matched*/
  PROG_ERRORTIMEOUT      = 2,           /* Operation failed with timeout      */
  PROG_ERRORRESOURCE     = 3,           /* Resource not available.            */
  PROG_ERRORPARAMETER    = 4,           /* Parameter error.                   */
  PROG_ERRORNOMEMORY     = 5,           /* Out of memory.                     */
  PROG_ERRORISR          = 6,           /* Not allowed in ISR context         */
  PROG_RESERVED          = 0x7FFFFFFF   /* Reserved                           */
}prog_status_t;

//******************************** Defines **********************************//

//******************************** Declaring ********************************//
void Prog_test_init(void);
void bsp_prog_trig_task_init(void);
prog_status_t bsp_prog_trig_init(void);
prog_status_t bsp_prog_trig_reset(void );
prog_status_t bsp_prog_trig_set_current_step( uint8_t step);
//prog_status_t bsp_prog_trig_enable( bool enable);
prog_status_t bsp_prog_trig_set_table(uint8_t idx, const prog_table_row_t* table);
prog_status_t bsp_prog_trig_insert_table(uint8_t idx, const prog_table_row_t* table);
prog_status_t bsp_prog_trig_delete_table( uint8_t idx);
prog_status_t bsp_prog_trig_set_start_step(uint8_t start);
prog_status_t bsp_prog_trig_set_end_step( uint8_t end);
//prog_status_t bsp_prog_trig_set_range_enable( bool enable);
prog_status_t bsp_prog_trig_exec(uint8_t trigger_src);
prog_status_t bsp_prog_trig_set_brightness(const uint8_t brightness[PROG_TRIG_CH_MAX]);
prog_status_t bsp_prog_trig_start_pulse(const uint16_t pulse_width);
void prog_trig_pulse_width_callback(void);
void prog_trig_continuous_next_step(void);
prog_status_t bsp_prog_trig_soft_trigger(void);
void bsp_output_camera_signal(void);
void prog_trig_callback(uint8_t channel);
//0.2.1
void prog_trig_handle(void);
prog_status_t bsp_prog_trig_set_interval(uint16_t interval);
uint16_t bsp_prog_trig_get_interval(void);
prog_status_t bsp_prog_trig_set_unit(uint8_t unit);
uint8_t bsp_prog_trig_get_unit(void);
void bsp_save_prog_table_data(void);
prog_status_t bsp_prog_trig_set_mode(uint8_t mode);
uint8_t bsp_prog_trig_get_mode(void);
uint8_t bsp_prog_trig_get_totsl_steps(void);
uint8_t bsp_prog_trig_get_current_step(void);
prog_status_t bsp_prog_trig_set_total_steps(uint8_t total_steps);

void test_tim4(void);
//******************************** Declaring ********************************//

#endif

