/******************************************************************************
 * Copyright (C) 2024 CST, Inc.(Gmbh) or its affiliates.
 *
 * All Rights Reserved.
 *
 * @file app.c
 *
 * @par dependencies
 * - app.h
 *
 * @author Alan | R&D Dept. | CST
 *
 * @brief Provide the HAL APIs of the app.
 *
 * Processing flow:
 * call directly.
 *
 * @version V1.0 2025-03-28 *
 * 			V1.1 2025-11-01  ALan  add slave param sync
 * 			V1.2 2025-12-09  ALan  fixed some bug
 * @note 
 *       1 tab == 4 spaces!
 *
 *****************************************************************************/

//******************************** Includes *********************************//
#include "app.h"
#include "app_slave_param_sync.h"
#include "app_router.h"
#include "protocol_share_header.h"
//******************************** Includes *********************************//

//******************************** Defines **********************************//
void TIM_CallBack1(void);
void light_alarm_handle(void );
app_system_status g_system_t;

extern TIM_HandleTypeDef htim6;
extern TIM_HandleTypeDef htim7;
//******************************** Defines **********************************//
 
//******************************** Declaring ********************************//
 #if IWDG_ENABLE
 extern IWDG_HandleTypeDef hiwdg;
 void bsp_feedDog(void)
 {
	HAL_IWDG_Refresh(&hiwdg);
 }
 #endif


/**
 * @brief	init_ScheduleTack
 * @note	
 * @param	void
 * @retval	void
 */
void init_ScheduleTack(void)
{
	#if IWDG_ENABLE
		bsp_StartAutoTimer(IWDG_TMR,IWDG_TIME);					//50ms IWDG_TMR 
	#endif
}
           
void app_slave_param_sync_completed_callback(bool success)
{
	if (success) {
		if (get_programmable_sync_mode() == SYNC_MODE_MASTER_SYNC) {
			start_prog_sync();
			LOG("Param sync completed successfully");
		}
	}
	else {
		LOG_W("Param sync completed with errors");
	}
}


/**
 * @brief	bsp_Init
 * @note	
 * @param	void
 * @retval	void
 */
void bsp_Init(void)
{
#if KEY_DISPLAY_ENABLE
	bsp_feedDog();
	HAL_Delay(300);
	bsp_feedDog();
	HAL_Delay(300);
	bsp_feedDog();
	HAL_Delay(300);
	bsp_feedDog();
	bsp_display_hard_init(BRIGHTNESS_3);
	//bsp_InitAip650();
	bsp_system_open_display(2000);	//wait for power stable
#else 
	HAL_Delay(2000);				//wait for power stable
#endif
	light_config_init();
#if RS232_ENABLE
	uart_task_init();
#endif
	bsp_tim_pwm_init(3);			//tim3 CH1-4
	bsp_trig_init_in_falling();
	bsp_cam_out_init();
	bsp_adc_init();
	bsp_prog_trig_init();

	app_flash_init_and_restore_data();

//soft timer , must be open
	bsp_InitTimer();
//strobe or prog timer, if this is necessary, keep it open
	bsp_init_multi_timer();
//base on soft timer , long time task init 
	init_ScheduleTack();

#if MASTER_CODE
	bsp_hcf4051_init();			
	bsp_InitSPIBus();  				// init spi master bus 
	app_spi_master_handle_init();
	app_slave_param_sync_init();  	// initial from slave param sync
	router_init();					// init router retry queue
	HAL_TIM_Base_Start_IT(&htim7);  // spi communication base timer
#endif

	bsp_feedDog();

#if W5500_IS_ON
	bsp_w5500_init();
	HAL_TIM_Base_Start_IT(&htim6);
#endif
	
#if KEY_DISPLAY_ENABLE	
	bsp_InitKey();
#endif
	LOG("System On!");
	//V1.1 power on search slave
	enable_found_slave_display();
	bsp_feedDog();
    master_find_slave_ctrl(true);
}
 /**
  * @brief customization timed tack
  * @note  
  */
void run_ScheduleTack(void)
{
#if IWDG_ENABLE
	if(bsp_CheckTimer(IWDG_TMR)){
		bsp_feedDog();
	}
#endif
}
 
 /**
 * @brief	bsp_RunPer10ms_interrupt
 * @note	fast task, should be short time
 * @param	void
 * @retval	void
 */
void bsp_RunPer10ms_interrupt(void)
{	
}

 /**
 * @brief	bsp_RunPer1ms_interrupt
 * @note	fast task, should be short time
 * @param	void
 * @retval	void
 */
void bsp_RunPer1ms_interrupt(void)
{
}	

/**
 * @brief	bsp_10ms_task
 * @note    normal task, can be long time
 * @param	void
 * @retval	void
 */
void bsp_10ms_task(void)
{
	light_alarm_handle();
	// send local change to slave --V1.2
	send_pending_param_changes();

	if (get_light_alarm() != STATUS_NORMAL) return;

	bsp_over_current_detect();
	#if KEY_DISPLAY_ENABLE	
		app_Key_Handler();
		bsp_DisplayHandler();
	#endif
	#if IWDG_ENABLE
		bsp_feedDog();
	#endif

	#if KEY_DISPLAY_ENABLE	
		bsp_KeyScan10ms();
	#endif
}

/**
 * @brief	bsp_1ms_task
 * @note	normal task, can be long time
 * @param	void
 * @retval	void
 */
void bsp_1ms_task(void)
{
	//send_pending_param_changes();
}


//******************************** system task ********************************//
/**
 * @brief	process_time_task
 * @note	1ms,10ms time task that long time task can be put here
 * @param	void	
 * @retval	void
 */
void process_time_task(void)
{
	if(bsp_Check1msTask()){
		bsp_1ms_task();
	}
	if(bsp_Check10msTask()){
		bsp_10ms_task();
	}
}

/**
 * @brief	app_task
 * @note	
 * @param	void
 * @retval	void
 */
void app_task(void)
{
	run_ScheduleTack();
	uart_task();
	
	#if W5500_IS_ON
		app_udp_dhcp_handle(0);
		for(uint8_t i = 1; i < 5; i++){
			app_tcp_handle(i);					// udp socket handle
		}
	#endif

	if(get_light_alarm() == STATUS_NORMAL){
		if(g_light_system.work_mode != WORK_MODE_PROG)
		{
			light_brightness_handle();
		}
		light_handle();	
	}

	#if MASTER_CODE
		if (get_light_alarm() != STATUS_SPI_COMM_ERROR) 
		{
			router_handle();					// process router retry queue
			bsp_spi_process();  				// SPI code function , don't block
			app_master_handle();				// SPI Master handle : find slave , heartbeat ,prog sync
			app_slave_param_sync_handle();		// SPI Slave param sync
		}
	#endif
	process_time_task();
}

/**
 * @brief	bsp_Idle
 * @note	
 * @param	void
 * @retval	void
 */
void bsp_Idle(void)
{
}

/**
 * @brief	light_alarm_handle
 * @note	
 * @param	void
 * @retval	void
 */
void light_alarm_handle(void )
{
	if(get_light_alarm() != STATUS_NORMAL)
	{
		bsp_all_pwm_disable();
		
		#if KEY_DISPLAY_ENABLE	
			bsp_ErrorDisplay();
		#endif
	
		#if IWDG_ENABLE
			bsp_feedDog();
		#endif
	}
}

//******************************** system task ********************************//


//******************************** Declaring ********************************//


