/******************************************************************************
 * Copyright (C) 2025 CST, Inc.(Gmbh) or its affiliates.
 *
 * All Rights Reserved.
 *
 * @file bsp_flash.h
 *
 * @par dependencies
 * - "main.h"
 *
 * @author Alan | R&D Dept. | CST
 *
 * @brief Provide the HAL APIs of the flash read and write.
 *
 * Processing flow:
 * call directly.
 *
 * @version         V1.0            2025-04-30 *
 *                  V1.1            2025-05-20      ALan 
 *                  V1.2            2025-07-23      ALan
 * @note saving data to flash
 *       1 tab == 4 spaces!
 *
 *****************************************************************************/
#ifndef __APP_FLASH_H__
#define __APP_FLASH_H__
//******************************** Includes *********************************//
#include "main.h"
#include "app_light.h"
//******************************** Includes *********************************//

//******************************** Defines **********************************//
#define ADDR_FLASH_PAGE_0       ((uint32_t)0x08000000) /* Base @ of Page 0, 2 Kbytes */
#define ADDR_FLASH_PAGE_1       ((uint32_t)0x08000800) /* Base @ of Page 1, 2 Kbytes */
#define ADDR_FLASH_PAGE_2       ((uint32_t)0x08001000) /* Base @ of Page 2, 2 Kbytes */
#define ADDR_FLASH_PAGE_3       ((uint32_t)0x08001800) /* Base @ of Page 3, 2 Kbytes */
#define ADDR_FLASH_PAGE_4       ((uint32_t)0x08002000) /* Base @ of Page 4, 2 Kbytes */
#define ADDR_FLASH_PAGE_5       ((uint32_t)0x08002800) /* Base @ of Page 5, 2 Kbytes */
#define ADDR_FLASH_PAGE_6       ((uint32_t)0x08003000) /* Base @ of Page 6, 2 Kbytes */
#define ADDR_FLASH_PAGE_7       ((uint32_t)0x08003800) /* Base @ of Page 7, 2 Kbytes */
#define ADDR_FLASH_PAGE_8       ((uint32_t)0x08004000) /* Base @ of Page 8, 2 Kbytes */
#define ADDR_FLASH_PAGE_9       ((uint32_t)0x08004800) /* Base @ of Page 9, 2 Kbytes */
#define ADDR_FLASH_PAGE_10      ((uint32_t)0x08005000) /* Base @ of Page 10, 2 Kbytes */
#define ADDR_FLASH_PAGE_11      ((uint32_t)0x08005800) /* Base @ of Page 11, 2 Kbytes */
#define ADDR_FLASH_PAGE_12      ((uint32_t)0x08006000) /* Base @ of Page 12, 2 Kbytes */
#define ADDR_FLASH_PAGE_13      ((uint32_t)0x08006800) /* Base @ of Page 13, 2 Kbytes */
#define ADDR_FLASH_PAGE_14      ((uint32_t)0x08007000) /* Base @ of Page 14, 2 Kbytes */
#define ADDR_FLASH_PAGE_15      ((uint32_t)0x08007800) /* Base @ of Page 15, 2 Kbytes */
#define ADDR_FLASH_PAGE_16      ((uint32_t)0x08008000) /* Base @ of Page 16, 2 Kbytes */
#define ADDR_FLASH_PAGE_17      ((uint32_t)0x08008800) /* Base @ of Page 17, 2 Kbytes */
#define ADDR_FLASH_PAGE_18      ((uint32_t)0x08009000) /* Base @ of Page 18, 2 Kbytes */
#define ADDR_FLASH_PAGE_19      ((uint32_t)0x08009800) /* Base @ of Page 19, 2 Kbytes */
#define ADDR_FLASH_PAGE_20      ((uint32_t)0x0800A000) /* Base @ of Page 20, 2 Kbytes */
#define ADDR_FLASH_PAGE_21      ((uint32_t)0x0800A800) /* Base @ of Page 21, 2 Kbytes */
#define ADDR_FLASH_PAGE_22      ((uint32_t)0x0800B000) /* Base @ of Page 22, 2 Kbytes */
#define ADDR_FLASH_PAGE_23      ((uint32_t)0x0800B800) /* Base @ of Page 23, 2 Kbytes */
#define ADDR_FLASH_PAGE_24      ((uint32_t)0x0800C000) /* Base @ of Page 24, 2 Kbytes */
#define ADDR_FLASH_PAGE_25      ((uint32_t)0x0800C800) /* Base @ of Page 25, 2 Kbytes */
#define ADDR_FLASH_PAGE_26      ((uint32_t)0x0800D000) /* Base @ of Page 26, 2 Kbytes */
#define ADDR_FLASH_PAGE_27      ((uint32_t)0x0800D800) /* Base @ of Page 27, 2 Kbytes */
#define ADDR_FLASH_PAGE_28      ((uint32_t)0x0800E000) /* Base @ of Page 28, 2 Kbytes */
#define ADDR_FLASH_PAGE_29      ((uint32_t)0x0800E800) /* Base @ of Page 29, 2 Kbytes */
#define ADDR_FLASH_PAGE_30      ((uint32_t)0x0800F000) /* Base @ of Page 30, 2 Kbytes */
#define ADDR_FLASH_PAGE_31      ((uint32_t)0x0800F800) /* Base @ of Page 31, 2 Kbytes */
#define ADDR_FLASH_PAGE_32      ((uint32_t)0x08010000) /* Base @ of Page 32, 2 Kbytes */
#define ADDR_FLASH_PAGE_33      ((uint32_t)0x08010800) /* Base @ of Page 33, 2 Kbytes */
#define ADDR_FLASH_PAGE_34      ((uint32_t)0x08011000) /* Base @ of Page 34, 2 Kbytes */
#define ADDR_FLASH_PAGE_35      ((uint32_t)0x08011800) /* Base @ of Page 35, 2 Kbytes */
#define ADDR_FLASH_PAGE_36      ((uint32_t)0x08012000) /* Base @ of Page 36, 2 Kbytes */
#define ADDR_FLASH_PAGE_37      ((uint32_t)0x08012800) /* Base @ of Page 37, 2 Kbytes */
#define ADDR_FLASH_PAGE_38      ((uint32_t)0x08013000) /* Base @ of Page 38, 2 Kbytes */
#define ADDR_FLASH_PAGE_39      ((uint32_t)0x08013800) /* Base @ of Page 39, 2 Kbytes */
#define ADDR_FLASH_PAGE_40      ((uint32_t)0x08014000) /* Base @ of Page 40, 2 Kbytes */
#define ADDR_FLASH_PAGE_41      ((uint32_t)0x08014800) /* Base @ of Page 41, 2 Kbytes */
#define ADDR_FLASH_PAGE_42      ((uint32_t)0x08015000) /* Base @ of Page 42, 2 Kbytes */
#define ADDR_FLASH_PAGE_43      ((uint32_t)0x08015800) /* Base @ of Page 43, 2 Kbytes */
#define ADDR_FLASH_PAGE_44      ((uint32_t)0x08016000) /* Base @ of Page 44, 2 Kbytes */
#define ADDR_FLASH_PAGE_45      ((uint32_t)0x08016800) /* Base @ of Page 45, 2 Kbytes */
#define ADDR_FLASH_PAGE_46      ((uint32_t)0x08017000) /* Base @ of Page 46, 2 Kbytes */
#define ADDR_FLASH_PAGE_47      ((uint32_t)0x08017800) /* Base @ of Page 47, 2 Kbytes */
#define ADDR_FLASH_PAGE_48      ((uint32_t)0x08018000) /* Base @ of Page 48, 2 Kbytes */
#define ADDR_FLASH_PAGE_49      ((uint32_t)0x08018800) /* Base @ of Page 49, 2 Kbytes */
#define ADDR_FLASH_PAGE_50      ((uint32_t)0x08019000) /* Base @ of Page 50, 2 Kbytes */
#define ADDR_FLASH_PAGE_51      ((uint32_t)0x08019800) /* Base @ of Page 51, 2 Kbytes */
#define ADDR_FLASH_PAGE_52      ((uint32_t)0x0801A000) /* Base @ of Page 52, 2 Kbytes */
#define ADDR_FLASH_PAGE_53      ((uint32_t)0x0801A800) /* Base @ of Page 53, 2 Kbytes */
#define ADDR_FLASH_PAGE_54      ((uint32_t)0x0801B000) /* Base @ of Page 54, 2 Kbytes */
#define ADDR_FLASH_PAGE_55      ((uint32_t)0x0801B800) /* Base @ of Page 55, 2 Kbytes */
#define ADDR_FLASH_PAGE_56      ((uint32_t)0x0801C000) /* Base @ of Page 56, 2 Kbytes */
#define ADDR_FLASH_PAGE_57      ((uint32_t)0x0801C800) /* Base @ of Page 57, 2 Kbytes */
#define ADDR_FLASH_PAGE_58      ((uint32_t)0x0801D000) /* Base @ of Page 58, 2 Kbytes */
#define ADDR_FLASH_PAGE_59      ((uint32_t)0x0801D800) /* Base @ of Page 59, 2 Kbytes */
#define ADDR_FLASH_PAGE_60      ((uint32_t)0x0801E000) /* Base @ of Page 60, 2 Kbytes */
#define ADDR_FLASH_PAGE_61      ((uint32_t)0x0801E800) /* Base @ of Page 61, 2 Kbytes */
#define ADDR_FLASH_PAGE_62      ((uint32_t)0x0801F000) /* Base @ of Page 62, 2 Kbytes */
#define ADDR_FLASH_PAGE_63      ((uint32_t)0x0801F800) /* Base @ of Page 63, 2 Kbytes */


#define STM32_PVD_ENABLE            1
#define STM32_FLASH_ENABLE          1

#define FLASH_256K				    0
#define FLASH_128K				    1

#define FMC_PAGE_SIZE               ((uint16_t)0x800U)
#define NVIC_VectTab_FLASH        	((uint32_t)0x08000000)		//Bootloader addr
#define APPLICATION_POSADDR_A 	  	(0x00003800) 				//APP offset
#define BOOT_FLAG_ADDR  			((uint32_t)0x8003400)
										

#if FLASH_64K
	#define DATA_SAVE_ADDR			((uint32_t)(0x800F000))				//2K
	#define BACK_DATA_SAVE_ADDR		((uint32_t)(0x800F800))				//2K
	#define BAUD_DATA_SAVE_ADDR		((uint32_t)(0x800FE00))				//2K
#endif

#if FLASH_128K
    #define W5500_FLASH_ADDR		((uint32_t)(0x801C800))		//W5500 flash addr
	#define ENABLE_SAVE_ADDR		((uint32_t)(0x801D000))		//data save enable	
    #define PROG_DATA_SAVE_ADDR     ((uint32_t)(0x801D800))     //prog data 
	#define DATA_SAVE_ADDR			((uint32_t)(0x801E000))		//power down		
	#define BACK_DATA_SAVE_ADDR		((uint32_t)(0x801E800))		//backup data		
	#define BAUD_DATA_SAVE_ADDR		((uint32_t)(0x801F000))		//baud rate		    
#endif

#if FLASH_256K
	#define ENABLE_SAVE_ADDR		((uint32_t)(0x803E000))			//2K
	#define DATA_SAVE_ADDR			((uint32_t)(0x803E800))			//2K
	#define BACK_DATA_SAVE_ADDR		((uint32_t)(0x803F000))			//2K
	#define BAUD_DATA_SAVE_ADDR		((uint32_t)(0x803F800))			//2K
#endif

#define	DataSucFlag					0x66					
#define	POWE_DOWN_SucFlag			0x77					
#define	BackDataSucFlag				0x88					
#define	BaudDataSucFlag				0x99					
#define ProgDataSucFlag             0xCC

typedef enum{
    POWER_DOWN_DISABLE = 0,
    POWER_DOWN_ENABLE = 1
}power_down_flag_e;

typedef struct {
    uint8_t  brightness_range;       
    uint8_t  work_mode;              
    uint8_t  normal_mode_state;      
    uint8_t  display_page;           
    uint8_t  uart_baud_gear;
    //Byte aligned, reserved for future use         
    uint8_t  sync_mode;       

    uint16_t brightness[CHANNEL_NUM];    
    uint16_t strobe_width[CHANNEL_NUM];  
    uint8_t  enabled[CHANNEL_NUM];       
    //If CHANNEL_NUM is odd, add 1 byte here to ensure byte alignment
} FlashParams_t;

typedef struct {
    uint16_t success_flag;              // ProgDataSucFlag
    uint16_t programmable_unit;          
    uint16_t programmable_mode;          
    uint16_t programmable_interval;      
    uint16_t prog_total_steps;           
    uint16_t reserved[3];               
    prog_table_row_t prog_table[PROG_TABLE_NUM_MAX]; 
} ProgFlashParams_t;

//******************************** Defines **********************************//

//******************************** Declaring ********************************//
void NVIC_SetVectorTable(uint32_t base, uint32_t offset);
#if STM32_PVD_ENABLE
void bsp_pvd_init(void);
#endif
void set_BootLoader_flag(void);
void bsp_FlashErase(uint32_t Address);
void bsp_SaveFlashData(void);
void bsp_ReadFlashData(void);
void bsp_SaveFlashBackupData(void);
void bsp_SaveBaudData(void);
void bsp_CheckBaud(void);
void app_flash_init_and_restore_data(void);
void SavePowerDown_enable(void);
void readPowerDown_enable(void);
void disable_powe_down_interface(void );
void enable_powe_down_interface(void);
uint8_t get_power_down_flag(void);
//V1.2  
void bsp_prog_trig_save_to_flash(void);
void bsp_prog_trig_load_from_flash(void);
void bsp_prog_trig_data_erase(void);
void app_flash_init_and_restore_data(void);
//******************************** Declaring ********************************//

#endif 

