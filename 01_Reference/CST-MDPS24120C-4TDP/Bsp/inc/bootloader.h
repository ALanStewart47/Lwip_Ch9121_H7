/*
*********************************************************************************************************
*
*	模块名称 : 串口升级配置(For STM32F1)
*	文件名称 : bootloader.h
*	版    本 : V1.0
*	说    明 : 用于自定协议的串口升级配置
*	修改记录 :
*		版本号  日期         作者       说明
*		V1.0    2024-06-03  	Alan  	正式发布
*
*********************************************************************************************************
*/
#ifndef __BOOTLOADER_H
#define __BOOTLOADER_H

#include "stdio.h"
#include "stdint.h"
#include "stdlib.h"

//#include "gd32f30x.h"


/*	---按需修改地址	*/
#define NVIC_VectTab_FLASH        	((uint32_t)0x08000000)					//Bootloader地址
#define APPLICATION_POSADDR_A 	  	(0x00003800) 							//APP偏移量 
#define BOOT_FLAG_ADDR  			(0x8003400)								//要擦除的地址

#define FLASH_256K					1

#if FLASH_64K
	#define DATA_SAVE_ADDR							((uint32_t)(0x800F000))		//数据保存地址		//2K
	#define BACK_DATA_SAVE_ADDR						((uint32_t)(0x800F800))		//数据保存地址		//2K
	#define BAUD_DATA_SAVE_ADDR						((uint32_t)(0x800FE00))		//数据保存地址		//2K
#endif


#if FLASH_256K
	#define ENABLE_SAVE_ADDR						((uint32_t)(0x803E000))		//数据保存地址		//2K
	#define DATA_SAVE_ADDR							((uint32_t)(0x803E800))		//数据保存地址		//2K
	#define BACK_DATA_SAVE_ADDR						((uint32_t)(0x803F000))		//数据保存地址		//2K
	#define BAUD_DATA_SAVE_ADDR						((uint32_t)(0x803F800))		//数据保存地址		//2K
#endif

#define	DataSucFlag									0x66					//数据写入成功标志位
#define	POWE_DOWN_SucFlag							0x77					//数据写入成功标志位
#define	BackDataSucFlag								0x88					//后备数据写入成功标志位
#define	BaudDataSucFlag								0x99					//波特率-写入成功标志位


/*	设置向量偏移	*/
void NVIC_SetVectorTable(uint32_t base, uint32_t offset);
/*	写入升级标志位	*/
void set_BootLoader_flag(void);

uint8_t bsp_CheckFlashSucess(void);

/*	保存必要数据	*/
void bsp_SaveFlashData(void);
/*	读取数据	*/
void bsp_ReadFlashData(void);

void bsp_SaveFlashBackupData(void);

void bsp_SaveBaudData(void);

//uint32_t bsp_CheckBaud(void);

#endif
/***************************** (END OF FILE) *********************************/
