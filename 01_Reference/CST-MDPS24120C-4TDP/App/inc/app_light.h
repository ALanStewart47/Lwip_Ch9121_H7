/******************************************************************************
 * Copyright (C) 2024 CST, Inc.(Gmbh) or its affiliates.
 *
 * All Rights Reserved.
 *
 * @file app_light.h
 *
 * @par dependencies
 * - main.h
 * 
 *
 * @author Alan | R&D Dept. | CST
 *
 * @brief Provide the HAL APIs of the light ctrl.
 *
 * Processing flow:
 * call directly.
 *
 * @version V1.0    2025-05-      Alan
 *          V1.1    2025-07-15    ALan
 *          
 * @note light state detection, and make sure max dac output value.
 *       1 tab == 4 spaces!
 *
 *****************************************************************************/
#ifndef __APP_LIGHT_H__
#define __APP_LIGHT_H__

//******************************** Includes *********************************//
#include "main.h"
//******************************** Includes *********************************//

//******************************** Defines **********************************//
#define MAJOR_VERSION		    1  	//major version 
#define	MINOR_VERSION		    0	//minor version 

#define CHANNEL_NUM			    4

#define CPL                     0
#define DPS                     1

#define STM32                   1
#define GD32                    0

#define SLAVE_SPI_MAX_NUM       8

//
#define LIGHT_VALUE_MIN         0
#define LIGHT_VALUE_255_MAX     255
#define LIGHT_VALUE_999_MAX     999
#define LIGHT_STROBE_WIDTH_MAX  999

//Programmable
#define PROG_TABLE_NUM_MAX      64

//PWM
#define USER_TIM_NUM            1 
#define USER_TIM_PWM_NUM        4
#define USER_PWM_FREQ           1452
//PWM Timer
#define PWM_TIMER_0_ENABLE      1
#define PWM_TIMER_1_ENABLE      0
#define PWM_TIMER_2_ENABLE      1
#define PWM_TIMER_3_ENABLE      0

#define PROG_TRIG_STEP_MAX      64
#define PROG_TRIG_CH_MAX        CHANNEL_NUM
#define PROG_TRIG_NUM_MAX       8
#define PROG_PULSE_WIDTH_MAX    999
#define PROG_INTERVAL_MAX       999

typedef enum{
    CH1                    = 0,
    CH2,
    CH3,
    CH4,
    CH5,
    CH6,
    CH7,
    CH8
}light_channle_num;

typedef enum{
    PROG_TIMER_CH1          = 1  ,  // Timer for pulse width measurement
    PROG_TIMER_CH2          = 2  ,  // Timer for trigger interval measurement
    PROG_TIMER_CH3          = 3     // Timer for camera output timing
}prog_trig_timer_ch_t;

typedef enum{
    TRIG_OUT                = 0,
    TRIG_IN 
}light_trig_state;

typedef enum{
    LIGHT_OFF               = 0,
    LIGHT_ON
}light_on_off_state;

typedef enum{
	L_MODE                  = 0,
	H_MODE
}normal_modes_state_t;

typedef enum{
	STROBE_TRIG_STATE_IDLE  = 0, 
	STROBE_TRIG_STATE_WORK  = 1,
}trig_work_t;

typedef enum{
	PROG_STATE_IDLE         = 0, 
	PROG_STATE_WORK         = 1,
}prog_work_t;

typedef enum {
    WORK_MODE_NORMAL        = 0,
    WORK_MODE_STROBE        = 1,
    WORK_MODE_PROG          = 2
} light_work_mode_t;

typedef enum{
    BRIGHTNESS_999          = 0,
    BRIGHTNESS_255
}light_brightness_value_max_t;

typedef enum{
    NO_SOFT_TRIGGER_REQUEST = 0,
    SOFT_TRIGGER_REQUEST    = 1,
}strobe_soft_trig_t;

typedef enum{
    STATUS_NORMAL           = 0,
    STATUS_OVERCURRENT      = 1,
    STATUS_OVERHEAT         = 2,
    STATUS_SPI_COMM_ERROR   = 3,   //SPI hardware communication error
    STATUS_ERROR_MAX        = 4
}alarm_status_t;

typedef enum{
    LIGHT_DISABLE           = 0,
    LIGHT_ENABLE            = 1
}light_enable_state_t;

typedef enum{
    PROG_UNIT_MS            = 0,   
    PROG_UNIT_100US         = 1 
}programmable_unit_t;

typedef enum{
    PROG_MODE_SINGLE_STEP   = 0, 
    PROG_MODE_CONTINUOUS    = 1  
}programmable_mode_t;

typedef enum{
    SYNC_MODE_MASTER_SYNC   = 0,
    SYNC_MODE_STANDLONE     = 1
}programmable_sync_mode_t;

typedef enum{
  CONTINUOUS_IDLE           = 0,
  CONTINUOUS_RUNNING        = 1
}prog_continuous_state_e;

typedef struct {
    uint16_t brightness;         // 
    uint8_t  enabled;            // 
    uint8_t  state;              //
    uint16_t strobe_width;       // 
    uint16_t soft_trig_width;    // 
    uint8_t  strobe_busy;        // 1:strobing £¬0:idle 
    uint8_t  soft_trig;          // soft trig flag (1:soft trigger request, 0:no soft trigger request)
    uint8_t  ext_trig;           // external trigger flag (1:external trigger request, 0:no external trigger request)
    uint32_t trig_count;         // external trigger count
    uint16_t camout_width; // strobe busy count
} light_channel_param_t;

typedef struct {
    uint16_t brightness[CHANNEL_NUM];       // 
    uint16_t prog_strobe_width;             // 
    //0.2.1
    uint8_t  trigger_src; 
    uint8_t  reserved;
} prog_table_row_t;


typedef struct {
    uint8_t  save_prog_flag;
    uint8_t  version[2];                
    uint8_t  brightness_range;          // 0:0-255, 1:0-999
    uint8_t  work_mode;                 // 1:normal 2:strobe 3:prog
    uint8_t  normal_mode_state;
    uint8_t  last_normal_mode_state;
    uint8_t  display_page;           
    uint8_t  uart_baud_gear;         
    uint8_t  alarm_status;              // 0-nor 1-over current  2 over heat

    uint8_t  programmable_unit;         // 0:ms 1:0.1ms
    uint8_t  programmable_mode;          
    uint8_t  programmable_step;         // now step
    uint16_t programmable_interval;     
    //0.2.1
    uint8_t  prog_total_steps;           
    uint8_t  prog_start_step;           //reserve
    uint8_t  prog_end_step;             //reserve
    uint8_t  prog_enable;                
    uint8_t  prog_busy;                 
    uint8_t  prog_continuous_running; 

    light_channel_param_t channel[CHANNEL_NUM];         
    light_channel_param_t channel_last[CHANNEL_NUM];      
    prog_table_row_t prog_table[PROG_TABLE_NUM_MAX];     
   // uint8_t  reserved[32];            
} system_param_t;

typedef enum{
  LIGHT_OK                = 0,    /* Operation completed successfully.  */
  LIGHT_ERROR             = 1,    /* Run-time error without case matched*/
  LIGHT_ERRORTIMEOUT      = 2,    /* Operation failed with timeout      */
  LIGHT_ERRORRESOURCE     = 3,    /* Resource not available.            */
  LIGHT_ERRORPARAMETER    = 4,    /* Parameter error.                   */
  LIGHT_ERRORNOMEMORY     = 5,    /* Out of memory.                     */
  LIGHT_ERRORISR          = 6,    /* Not allowed in ISR context         */
  LIGHT_RESERVED          = 7,    /* Reserved                           */
}light_status_t;
//******************************** Defines **********************************//

//******************************** Declaring ********************************//
extern uint8_t g_light_mode;
extern system_param_t g_light_system; 
void            light_config_init(void);
//void            light_config_check(void );
uint16_t        get_light_value(uint8_t channel);
light_status_t  set_light_value(uint8_t channel, uint16_t value);
uint8_t         get_light_name(uint8_t channel);
uint8_t         get_light_mode(void);
light_status_t  set_light_mode(uint8_t mode);
light_status_t  set_light_state(uint8_t channel , uint8_t state);
uint8_t         get_trig_state(uint8_t channel);
uint8_t         get_light_state(uint8_t channel);
light_status_t  set_light_state(uint8_t channel,uint8_t state);
void            normal_mode_light_ctrl_handle(uint8_t channel);
light_status_t  bsp_light_on(uint8_t channel);
light_status_t  bsp_light_off(uint8_t channel);
void            bsp_light_all_off(void);
void            light_brightness_handle(void);
void            light_handle(void);
void            trig_callback(uint8_t channel);
uint8_t         get_light_alarm(void);
light_status_t  set_light_alarm(uint8_t status);
light_status_t  set_light_brightness_range(uint8_t i);
uint8_t         get_light_brightness_range(void); 
uint16_t        get_light_strobe_width(uint8_t channel);
light_status_t  set_light_strobe_width(uint8_t channel, uint16_t width);
uint8_t         get_light_enable_state(uint8_t channel);
light_status_t  set_light_enable_state(uint8_t channel, uint8_t state);
//V1.1 
void            light_factory_reset(void);
light_status_t  set_light_brightness_range(uint8_t i);
uint8_t         get_light_brightness_range(void);
light_status_t  set_light_normal_mode_state(uint8_t mode);
uint8_t         get_light_normal_mode_state(void);
light_status_t  set_232_baud_gear(uint8_t gear);
uint8_t         get_232_baud_gear(void);
light_status_t 	set_all_light_enable_state(uint8_t state);
programmable_sync_mode_t get_programmable_sync_mode(void);
void set_programmable_sync_mode_vlaue(uint8_t mode);
uint8_t get_programmable_sync_mode_value(void);
//******************************** Declaring ********************************//


#endif 

