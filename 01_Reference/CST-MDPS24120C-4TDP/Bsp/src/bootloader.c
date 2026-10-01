
#include "bootloader.h"
#include "main.h"



void NVIC_SetVectorTable(uint32_t base, uint32_t offset)
{
    /* close interruption*/
    __set_FAULTMASK(1);

    /* set vector table*/
    //NVIC_SetVectorTable(NVIC_VectTab_FLASH, 0xffset);
    SCB->VTOR = base | offset;

    /* open interruption*/
    __set_FAULTMASK(0);
}





void set_BootLoader_flag(void)
{
	uint16_t Write_Flash_Data = 0xaa; 
	FLASH_EraseInitTypeDef otaFlash;
	uint32_t PageError = 0;
	
	HAL_FLASH_Unlock();			  //FLASH_Unlock();
	
	/*	配置FLASH擦除结构�?	*/
	otaFlash.TypeErase	=	FLASH_TYPEERASE_PAGES;
	otaFlash.PageAddress	=	BOOT_FLAG_ADDR;
	otaFlash.NbPages	=	1; 
	
	HAL_FLASHEx_Erase(&otaFlash, &PageError);				//FLASH_ErasePage(BOOT_FLAG_ADDR);  
	HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD, BOOT_FLAG_ADDR, Write_Flash_Data);	  //FLASH_ProgramHalfWord(BOOT_FLAG_ADDR,Write_Flash_Data);   
	
	HAL_FLASH_Lock();		//FLASH_Lock();
}


/*
*********************************************************************************************************
*	�? �? �?: bsp_FlashErase
*	功能说明: Flash擦除�?
*	�?    参：Address-要擦除的地址
*	�? �? �?: �?
*********************************************************************************************************
*/
//static void bsp_FlashErase(uint32_t	Address)
//{
//	FLASH_EraseInitTypeDef	EraseInitStruct;
//	uint32_t PageError = 0;
//	
//	HAL_FLASH_Unlock();	
//	EraseInitStruct.TypeErase	=	FLASH_TYPEERASE_PAGES;
//	EraseInitStruct.PageAddress	=	Address;
//	EraseInitStruct.NbPages	=	1;
//	HAL_FLASHEx_Erase(&EraseInitStruct,&PageError);
//	HAL_FLASH_Lock();
//}







/*****************************	(END OF FILE)	*********************************/

