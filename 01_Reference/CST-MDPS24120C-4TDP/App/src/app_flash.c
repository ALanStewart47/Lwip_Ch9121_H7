/******************************************************************************
 * Copyright (C) 2024 CST, Inc.(Gmbh) or its affiliates.
 *
 * All Rights Reserved.
 *
 * @file app_flash.c
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
 * @version V1.0 		2025-04-10 *
 *          V1.1 		2025-05-20		ALan
 * 			V1.2 		2025-07-23		ALan
 *          V1.3 		2025-08-13		ALan
 * @note 
 *       1 tab == 4 spaces!
 *
 *****************************************************************************/
#include "app_flash.h"
#include  "app.h"
#include "app_light.h" 


typedef union 
{
	uint32_t 	s_bits;
	float 		f_value;
}dac_union;

void bsp_FlashErase(uint32_t Address);

dac_union dac_bipolar;
dac_union dac_unipolar;

//power down function enable flag 
uint8_t g_power_down_data_enable = POWER_DOWN_ENABLE;		//0 -失能    1-使能
void 	bsp_SaveFlashData(void);


/**
 * @brief 	get page 
 * 
 * @return  flash page
 * */
static uint32_t GetPage(uint32_t Addr)
{
  return (Addr - FLASH_BASE) / FLASH_PAGE_SIZE;;
}


void NVIC_SetVectorTable(uint32_t base, uint32_t offset)
{
//    /* close interruption*/
//	#if STM32
//    	__set_FAULTMASK(1);
//	#else
//		DISABLE_INT();
//	#endif

//    /* set vector table*/
//    //NVIC_SetVectorTable(NVIC_VectTab_FLASH, 0xffset);
//    SCB->VTOR = base | offset;

//    /* open interruption*/
//	#if STM32
//    	__set_FAULTMASK(0);
//	#else
//		ENABLE_INT();
//	#endif
	    /* close interruption*/
    uint32_t i=0;
    
    HAL_RCC_DeInit();
    HAL_DeInit();
    
    SysTick->CTRL = 0;
    SysTick->LOAD = 0;
    SysTick->VAL  = 0;
    for (i = 0; i < 8; i++)
    {
        NVIC->ICER[i]=0xFFFFFFFF;
        NVIC->ICPR[i]=0xFFFFFFFF;
    }      
    DISABLE_INT();
    //__set_FAULTMASK(1);

    /* set vector table*/
    SCB->VTOR = base | (offset  & (uint32_t)0xFFFFFE00);

    /* open interruption*/
    ENABLE_INT();
   // __set_FAULTMASK(0);
}

FLASH_EraseInitTypeDef otaFlash = {0};
uint32_t PageError = 0;

/** 
 * @brief  set bootloader flag
 * @note   used to mark ota flag in flash
 * @param  none
 */
void set_BootLoader_flag(void)
{
    uint64_t Write_Flash_Data = 0xAA; 
    HAL_FLASH_Unlock();			 
//  FLASH->ACR &= ~(1 << 10);
//    /* Clear OPTVERR bit set on virgin samples */
//  __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_OPTVERR);
//  __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_BSY|FLASH_FLAG_EOP|FLASH_FLAG_PGAERR|FLASH_FLAG_PGSERR|FLASH_FLAG_WRPERR);
	otaFlash.TypeErase	=	FLASH_TYPEERASE_PAGES;
	otaFlash.Page	=	GetPage(BOOT_FLAG_ADDR);
	otaFlash.NbPages	=	1; 
	if(HAL_FLASHEx_Erase(&otaFlash, &PageError) != HAL_OK){
        LOG_E("Flash erase failed");
        //while(1);
    }		
    //HAL_FLASH_Lock();		
    //HAL_FLASH_Unlock();			  
	if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD, BOOT_FLAG_ADDR, Write_Flash_Data)!= HAL_OK) {
        
      while(1);
	}
//  __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_BSY|FLASH_FLAG_EOP|FLASH_FLAG_PGAERR|FLASH_FLAG_PGSERR|FLASH_FLAG_WRPERR);
//  FLASH->ACR |= 1 << 10;
    HAL_FLASH_Lock();		
}
/**************************************** bootloader  *****************************************************************/

/**************************************** PVD CONFIG *****************************************************************/

/********* 
 * power down save and init function 
**********/
void enable_powe_down_interface(void)
{
	g_power_down_data_enable = POWER_DOWN_ENABLE;
	SavePowerDown_enable();
}

void disable_powe_down_interface(void )
{
	g_power_down_data_enable = POWER_DOWN_DISABLE;
	SavePowerDown_enable();
}

uint8_t get_power_down_flag(void)
{
	return g_power_down_data_enable;
}
/********* 
 * power down save and init function 
**********/


#if STM32_PVD_ENABLE
void bsp_pvd_init(void)
{
	__HAL_RCC_PWR_CLK_ENABLE();
	HAL_NVIC_SetPriority(PVD_PVM_IRQn, 0, 0);
	HAL_NVIC_EnableIRQ(PVD_PVM_IRQn);
	PWR_PVDTypeDef sConfigPVD;
	sConfigPVD.PVDLevel = PWR_CR2_PLS_LEV6;
	sConfigPVD.Mode     = PWR_PVD_MODE_IT_RISING;
	HAL_PWR_ConfigPVD(&sConfigPVD);
	HAL_PWR_EnablePVD();
}


void bsp_pvd_powerdown_savedata(void)
{
	bsp_SaveFlashData();

	bsp_SaveBaudData();

	//SavePowerDown_enable();
}


void HAL_PWR_PVDCallback(void)
{
	bsp_pvd_powerdown_savedata();
}
#endif

/**************************************** PVD CONFIG *****************************************************************/


/**
 * @brief   erase the flash addr in page
 * @note    		 
 * @param       
 * @retval  
 */
void bsp_FlashErase(uint32_t Address)
{
	FLASH_EraseInitTypeDef	EraseInitStruct;
	uint32_t PageError 			= 0;
	
	HAL_FLASH_Unlock();	
	EraseInitStruct.TypeErase	= FLASH_TYPEERASE_PAGES;
    EraseInitStruct.Page 		= GetPage(Address);
    EraseInitStruct.Banks 		= FLASH_BANK_1;
    EraseInitStruct.NbPages 	= 1;
	HAL_FLASHEx_Erase(&EraseInitStruct,&PageError);
	HAL_FLASH_Lock();
}


/**
 * @brief   save data interface 
 * @note    		 
 * @param       
 * @retval  
 */
void bsp_SaveFlashData(void)
{
	FlashParams_t params_to_save;
    uint64_t *p_data                    = (uint64_t*)&params_to_save;
    uint16_t data_len_in_doubleword     = (sizeof(FlashParams_t) + 7) / 8;  // 向上取整到64位边界
    uint32_t current_addr               = DATA_SAVE_ADDR;

    params_to_save.brightness_range	 	= g_light_system.brightness_range;
    params_to_save.work_mode 		 	= g_light_system.work_mode;
    params_to_save.normal_mode_state	= g_light_system.normal_mode_state;
    params_to_save.display_page 	 	= g_light_system.display_page;
    params_to_save.uart_baud_gear 	 	= g_light_system.uart_baud_gear;
    params_to_save.sync_mode            = get_programmable_sync_mode_value();
    for(uint8_t i = 0; i < CHANNEL_NUM; i++){
        params_to_save.brightness[i] 	= g_light_system.channel[i].brightness;
        params_to_save.enabled[i] 		= g_light_system.channel[i].enabled;
        params_to_save.strobe_width[i] 	= g_light_system.channel[i].strobe_width;
    }

    HAL_FLASH_Unlock();
    uint64_t flag_data = (uint64_t)DataSucFlag;
    HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD, current_addr, flag_data);
    current_addr += 8;

    for(uint16_t i = 0; i < data_len_in_doubleword; i++){
        HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD, current_addr, p_data[i]);
        current_addr += 8;
    }
    HAL_FLASH_Lock();
}

/**
 * @brief   Saves programmable trigger settings to Flash.
 * @note    This function gathers settings from g_light_system and writes them to a dedicated Flash page.
 * @param   None
 * @retval  None
 */
void bsp_prog_trig_save_to_flash(void)
{
    ProgFlashParams_t params_to_save        = {0};
    uint64_t *p_data                        = (uint64_t*)&params_to_save;
    uint16_t data_len_in_doubleword         = (sizeof(ProgFlashParams_t) + 7) / 8;
    uint32_t current_addr                   = PROG_DATA_SAVE_ADDR;

    params_to_save.success_flag 			= ProgDataSucFlag; 
    params_to_save.programmable_unit 		= g_light_system.programmable_unit;
    params_to_save.programmable_mode		= g_light_system.programmable_mode;
    params_to_save.programmable_interval 	= g_light_system.programmable_interval;
    params_to_save.prog_total_steps 		= g_light_system.prog_total_steps;
    memcpy(params_to_save.prog_table, g_light_system.prog_table, sizeof(g_light_system.prog_table));

    bsp_FlashErase(PROG_DATA_SAVE_ADDR);
    HAL_FLASH_Unlock();
    for(uint16_t i = 0; i < data_len_in_doubleword; i++){
        HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD, current_addr, p_data[i]);
        current_addr += 8;
    }
    HAL_FLASH_Lock();
}

void bsp_prog_trig_data_erase(void)
{
    bsp_FlashErase(PROG_DATA_SAVE_ADDR);
}


/**
 * @brief   save data in backup way 
 * @note    		 
 * @param       
 * @retval  
 */

void bsp_SaveFlashBackupData(void)
{
//	bsp_FlashErase(BACK_DATA_SAVE_ADDR);
//	
//	fmc_unlock();

//	fmc_halfword_program(BACK_DATA_SAVE_ADDR, (uint16_t)BackDataSucFlag);
//	
//	for(uint8_t i = 0; i < CHANNEL_NUM ; i++)
//	{
//		fmc_halfword_program((BACK_DATA_SAVE_ADDR+2+ (i*2)), get_light_value(i));
//	}
//	fmc_halfword_program((BACK_DATA_SAVE_ADDR+18), (uint16_t)get_light_mode());
//	fmc_halfword_program((BACK_DATA_SAVE_ADDR+20), (uint16_t)g_DisPlayPage);
//	fmc_lock();
}


/**
 * @brief   read data interface ,check the save data flag , and read data, erase the flash addr in the end	 
 * @note   	will be called by app_flash_init_and_restore_data
 * @param       
 * @retval  
 */
void bsp_ReadFlashData(void)
{
    if(*(uint16_t*)DATA_SAVE_ADDR != DataSucFlag) return;
    FlashParams_t params_from_flash;
    uint64_t *p_data = (uint64_t*)&params_from_flash;
    uint16_t data_len_in_doubleword = (sizeof(FlashParams_t) + 7) / 8;
    uint32_t current_addr = DATA_SAVE_ADDR + 8; 
    for(uint16_t i = 0; i < data_len_in_doubleword; i++)
    {
        p_data[i] = *(uint64_t*)current_addr;
        current_addr += 8;
    }


    if(params_from_flash.brightness_range > BRIGHTNESS_255) g_light_system.brightness_range = BRIGHTNESS_255;
    else g_light_system.brightness_range = params_from_flash.brightness_range;

    if(params_from_flash.work_mode > WORK_MODE_PROG) g_light_system.work_mode = WORK_MODE_NORMAL;
    else g_light_system.work_mode = params_from_flash.work_mode;

    if(params_from_flash.normal_mode_state > H_MODE) g_light_system.normal_mode_state = L_MODE;
    else g_light_system.normal_mode_state = params_from_flash.normal_mode_state;

    if(params_from_flash.display_page >= PAGE_MAIN_MAX) g_light_system.display_page = PAGE_BRIGHTNESS_NORMAL;
    else g_light_system.display_page = params_from_flash.display_page;

    if (params_from_flash.sync_mode > SYNC_MODE_STANDLONE) {
        params_from_flash.sync_mode = 1;
    }
    set_programmable_sync_mode_vlaue(params_from_flash.sync_mode);

     //if(params_from_flash.uart_baud_gear >= BAUD_MAX) g_light_system.uart_baud_gear = BAUD_19200;
     //else g_light_system.uart_baud_gear = params_from_flash.uart_baud_gear;
    //g_light_system.uart_baud_gear = params_from_flash.uart_baud_gear; 

    for(uint8_t i = 0; i < CHANNEL_NUM; i++)
    {
        set_light_value(i, params_from_flash.brightness[i]);
        
        if(params_from_flash.enabled[i] > LIGHT_ENABLE) g_light_system.channel[i].enabled = LIGHT_ENABLE;
        else g_light_system.channel[i].enabled = params_from_flash.enabled[i];

        if(params_from_flash.strobe_width[i] > LIGHT_STROBE_WIDTH_MAX) g_light_system.channel[i].strobe_width = LIGHT_STROBE_WIDTH_MAX;
        else g_light_system.channel[i].strobe_width = params_from_flash.strobe_width[i];
    }

    bsp_FlashErase(DATA_SAVE_ADDR);
}


/**
 * @brief   Loads programmable trigger settings from Flash.
 * @note    This function reads settings from a dedicated Flash page and applies them to g_light_system.
 * @param   None
 * @retval  None
 */
void bsp_prog_trig_load_from_flash(void)
{

	ProgFlashParams_t params_from_flash = {0};
	uint64_t *p_data = (uint64_t*)&params_from_flash;
	uint16_t data_len_in_doubleword = (sizeof(ProgFlashParams_t) + 7) / 8;
	uint32_t current_addr = PROG_DATA_SAVE_ADDR;
	for(uint16_t i = 0; i < data_len_in_doubleword; i++)
	{
		p_data[i] = *(uint64_t*)current_addr;
		current_addr += 8;
	}

    if(params_from_flash.success_flag != ProgDataSucFlag) {
        return; 
    }

    g_light_system.programmable_unit = 
        (params_from_flash.programmable_unit > PROG_UNIT_100US) ? PROG_UNIT_MS : params_from_flash.programmable_unit;
    
    g_light_system.programmable_mode = 
        (params_from_flash.programmable_mode > PROG_MODE_CONTINUOUS) ? PROG_MODE_SINGLE_STEP : params_from_flash.programmable_mode;
    
    g_light_system.programmable_interval = 
        (params_from_flash.programmable_interval > PROG_INTERVAL_MAX) ? 0 : params_from_flash.programmable_interval;
    
    g_light_system.prog_total_steps = 
        (params_from_flash.prog_total_steps > PROG_TABLE_NUM_MAX) ? 0 : params_from_flash.prog_total_steps;

    if (g_light_system.prog_total_steps > 0) 
    {
        uint16_t max_brightness = (g_light_system.brightness_range == BRIGHTNESS_255) ? 
                                  LIGHT_VALUE_255_MAX : LIGHT_VALUE_999_MAX;
        for (uint8_t i = 0; i < g_light_system.prog_total_steps; i++) 
        {
            g_light_system.prog_table[i] = params_from_flash.prog_table[i];
            
            prog_table_row_t* current_step = &g_light_system.prog_table[i];

            for (uint8_t ch = 0; ch < CHANNEL_NUM; ch++) {
                if (current_step->brightness[ch] > max_brightness) {
                    current_step->brightness[ch] = max_brightness;
                }
            }

            if (current_step->prog_strobe_width > PROG_PULSE_WIDTH_MAX) {
                current_step->prog_strobe_width = PROG_PULSE_WIDTH_MAX;
            }

            if (current_step->trigger_src > TRIG_1) {
                current_step->trigger_src = TRIG_1;
            }
        }
    }
}


/**
 * @brief   flash app init and read flash data
 * @note   	 
 * @param       
 * @retval  
 */
void app_flash_init_and_restore_data(void )
{
	//g_power_down_data_enable = POWER_DOWN_ENABLE;
	//SavePowerDown_enable();	
	bsp_pvd_init();

	//if(get_power_down_flag() == POWER_DOWN_ENABLE)
	bsp_ReadFlashData();
    bsp_prog_trig_load_from_flash();
	//else
	//	bsp_FlashErase(DATA_SAVE_ADDR);		
}



/**************************************** Uart Baud **************************************************************** */

void bsp_SaveBaudData(void)
{
	bsp_FlashErase(BAUD_DATA_SAVE_ADDR);
	HAL_FLASH_Unlock();
	HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD, BAUD_DATA_SAVE_ADDR, (uint64_t)BaudDataSucFlag);
	HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD, BAUD_DATA_SAVE_ADDR+8, (uint64_t)g_light_system.uart_baud_gear);
	HAL_FLASH_Lock();
}


void bsp_CheckBaud(void)
{
	uint16_t _usFlag	=	0;

	_usFlag = (uint16_t) *(uint64_t *)(BAUD_DATA_SAVE_ADDR);
	if( _usFlag == BaudDataSucFlag )
	{
		g_light_system.uart_baud_gear	= (uint8_t)*(uint64_t *)(BAUD_DATA_SAVE_ADDR + 8);

		if(g_light_system.uart_baud_gear >= BAUD_MAX)
			g_light_system.uart_baud_gear = BAUD_19200;
	}
	else
	{
		g_light_system.uart_baud_gear = BAUD_19200;
	}
}


void SavePowerDown_enable(void)
{
	bsp_FlashErase(ENABLE_SAVE_ADDR);
	
	HAL_FLASH_Unlock();
	HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD, ENABLE_SAVE_ADDR, 	(uint64_t)POWE_DOWN_SucFlag);
	HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD, ENABLE_SAVE_ADDR+8, (uint64_t)g_power_down_data_enable);
	HAL_FLASH_Lock();
}


void readPowerDown_enable(void)
{
	uint8_t _usFlag	 = 0;
	uint8_t s_enable = 0;
    _usFlag          = (uint8_t)*(uint64_t *)(ENABLE_SAVE_ADDR);

	if(_usFlag == POWE_DOWN_SucFlag )
	{
		s_enable = (uint8_t)*(uint64_t *)(ENABLE_SAVE_ADDR+2);

		if(s_enable != POWER_DOWN_ENABLE && s_enable != POWER_DOWN_DISABLE)
			SavePowerDown_enable();

		if(s_enable == POWER_DOWN_ENABLE){
			g_power_down_data_enable = POWER_DOWN_ENABLE;
		}
		else{
			g_power_down_data_enable = POWER_DOWN_DISABLE;
		}
	}

}

	
/**************************************** Uart Baud **************************************************************** */






