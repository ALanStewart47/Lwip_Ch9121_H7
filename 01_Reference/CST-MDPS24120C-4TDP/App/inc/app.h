/******************************************************************************
 * Copyright (C) 2024 CST, Inc.(Gmbh) or its affiliates.
 *
 * All Rights Reserved.
 *
 * @file app.h
 *
 * @par dependencies
 * - stdint.h
 * - stdio.h
 *
 * @author Alan | R&D Dept. | CST
 *
 * @brief Provide the HAL APIs of the app.
 *
 * Processing flow:
 * call directly.
 *
 * @version V1.0 2025-03-28 *
 * @note 
 *       1 tab == 4 spaces!
 *
 *****************************************************************************/
 #ifndef __APP_H__
 #define __APP_H__
 
 //******************************** Includes *********************************//
 #include "stdint.h"
 #include "stdio.h"

#include "bsp_timer.h"
//#include "bsp_uart_dma.h"
#include "bsp_trig_gpio.h"
#include "bsp_tim_pwm.h"
//#include "bsp_aip650.h"
#include "bsp_key.h"
//#include "bsp_adc_dma.h"
#include "bsp_w5500.h"
//0.2
//#include "bsp_led_indicator.h"
#include "stm32_tm1637.h"
#include "bsp_spi_bus_dma.h"
#include "bsp_hcf4051.h"


#include "systick.h"

#include "app_light.h"
#include "app_key_display.h"
#include "app_uart.h"
#include "app_flash.h"
#include "app_adc_handle.h"
//#include "app_pvd.h"
#include "app_w5500.h"


////0.2
#include "app_trig.h"
//#include "bsp_led_indicator.h"
////0.2.1
#include "app_prog_trig.h"


////0.2.3
//#include "protocol_public.h"
//#include "elog.h"

////0.2.3.1
#include "app_master_command.h"

 //******************************** Includes *********************************//

 //******************************** Defines **********************************//
#define ENABLE_INT()	__enable_irq()	
#define DISABLE_INT()	__disable_irq()	

#ifndef TRUE
	#define TRUE  1
#endif

#ifndef FALSE
	#define FALSE 0
#endif

#define KEY_DISPLAY_ENABLE		1	//
#define OTA_UART_ENABLE			1
#define IWDG_ENABLE				1
#define DEBUG_MODE				1
#define DETECT_POWER_ENABLE		0
#define W5500_IS_ON   			1 //1:开启  0:关闭
#define APP_ROUTER_ENABLE		1 //1:开启  0:关闭
#define RS232_ENABLE			1

#define MASTER_CODE				1
#define SLAVE_CODE				1

#define	TMC_RESEND_TIME					15				
#define	IWDG_TIME						50				
#define	FLASH_BACKUP_TIME				1200000			
#define CHECK_POWER_TIME				6				
#define MODE_LED_TIME					500				
#define ADC_CALC						100

typedef struct
{
	uint8_t state ;
	uint8_t priority ; 
}app_system_status;

typedef enum
{
	SYSTEM_OPEN_STATE = 0,
	SYSTEM_RUN_STATE 
}system_state_e;

typedef enum
{
	NO_ERROR			= 0	,
	OVER_CURRENT_ERROR		, 			
}error_state_e;

extern  app_system_status g_system_t;

//******************************** Defines **********************************//

//******************************** Declaring ********************************//
void bsp_Init(void);
void init_ScheduleTack(void);
void run_ScheduleTack(void);
void bsp_RunPer10ms_interrupt(void);
void bsp_RunPer1ms_interrupt(void);
void app_task(void);
void bsp_Idle(void);
 void app_system_run(void);
 uint8_t get_app_system_state(void);
 uint8_t get_error_flag(void);
 void set_error_flag(error_state_e flag);
//******************************** Declaring ********************************//

 #endif /* __APP_H__ */
 

