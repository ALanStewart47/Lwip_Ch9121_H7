/******************************************************************************
 * Copyright (C) 2025 CST, Inc.(Gmbh) or its affiliates.
 *
 * All Rights Reserved.
 *
 * @file bsp_aip650.c
 *
 * @par dependencies
 * - bsp_aip650.h
 *
 * @author Alan | R&D Dept. | CST
 *
 * @brief Provide the HAL APIs of AIP650.
 *
 * Processing flow:
 * call directly.
 *
 * @version    V1.0     2025-05-09      ALan 
 * @note    
 *              1 tab == 4 spaces!
 *
 *****************************************************************************/
#include "bsp_aip650.h"
#include "systick.h"

/*	IO操作	*/
#define SCL_Set 		gpio_bit_set(   AIP650_PORT_CLK, AIP650_PIN_CLK );//(GPIOA->BSRR = 1<<14)
#define SDA_Set 		gpio_bit_set(   AIP650_PORT_DAT, AIP650_PIN_DAT );//(GPIOA->BSRR = 1<<13)
#define SCL_Reset		gpio_bit_reset( AIP650_PORT_CLK, AIP650_PIN_CLK );//(GPIOB->BSRR = 1<<(16+14))
#define SDA_Reset 	    gpio_bit_reset( AIP650_PORT_DAT, AIP650_PIN_DAT ); //(GPIOB->BSRR = 1<<(16+13))

#define SDA_Read 		(gpio_output_bit_get(AIP650_PORT_DAT, AIP650_PIN_DAT))

/*	I2C延时 */
#define IIC_uS 5

/* 使能GPIO时钟 */
#define		AIP650_GPIO_CLK_ENABLE(){	\
			rcu_periph_clock_enable(RCU_GPIOA);	\
			rcu_periph_clock_enable(RCU_GPIOB);	\
			rcu_periph_clock_enable(RCU_GPIOC);	\
			rcu_periph_clock_enable(RCU_GPIOD);	\
};	
	

/** 
*	数码管显示的数字、字母对应的代码数组
*	0,1,2,3,4,5,6,7,8,9,E,r,H,P,-,空,t,c		
**/											
const uint8_t DISPLAY_NUM[18]={0x3f,0x06,0x5b,0x4f,0x66,0x6d,0x7d,0x07,0x7f,0x6f,0x79,0x50,0x76,0x73,0x40,0x00,0x78,0x58};


static void AIP650_Wr_RAM(uint8_t Address, uint8_t Data);
							
													
/*
*********************************************************************************************************
*	函 数 名: bsp_InitKey
*	功能说明: 初始化按键. 该函数被 Bsp_InitLedDisplay() 调用。
*	形    参:  无
*	返 回 值: 无
*********************************************************************************************************
*/
void AIP650_InitHard(void)
{
	AIP650_GPIO_CLK_ENABLE();
	
    gpio_init(  AIP650_PORT_CLK, 
                GPIO_MODE_OUT_PP, 
                GPIO_OSPEED_50MHZ, 
                AIP650_PIN_CLK);			

    gpio_init(  AIP650_PORT_DAT, 
                GPIO_MODE_OUT_PP, 
                GPIO_OSPEED_50MHZ, 
                AIP650_PIN_DAT);	
}
													
/*
*********************************************************************************************************
*	函 数 名: AIP650_InitConf
*	功能说明: 配置AIP650芯片，系统使能，开启显示。该函数被 Bsp_InitLedDisplay() 调用。
*	形    参:  无
*	返 回 值: 无
*********************************************************************************************************
*/
void AIP650_InitConf(void)
{
	//命令代码：4801 ：系统使能，开启显示，最高亮度
	AIP650_Wr_RAM(0x48,1);
}


/*
*********************************************************************************************************
*	函 数 名: bsp_InitLedDisplay
*	功能说明: 初始化数码管. 该函数被 bsp_Init() 调用。
*	形    参:  无
*	返 回 值: 无
*********************************************************************************************************
*/
void bsp_InitAip650(void)
{
	AIP650_InitHard();
	AIP650_InitConf();
}


/*
************************************************
* 函 数 名: Delay_uS
* 功能说明: I2C总线位延迟
* 形    参：无
* 返 回 值: 无
************************************************
*/
static void Delay_uS(uint32_t udelay)
{
  __IO uint32_t Delay = udelay * 33 / 16;//(SystemCoreClock / 8U / 1000000U) (0.75)(48)
    
  do
  {
    __NOP();
  }
  while (Delay --);
}


/*
*********************************************************************************************************
*	函 数 名: IIC_Start
*	功能说明: 
*	形    参:  	无
*	返 回 值: 	无
*********************************************************************************************************
*/
static void IIC_Start(void)
{
    SCL_Set;
    Delay_uS(IIC_uS);
    SDA_Set;
    Delay_uS(IIC_uS);
    SDA_Reset;
    Delay_uS(IIC_uS);
    SCL_Reset;
    Delay_uS(IIC_uS);
}

/*
*********************************************************************************************************
*	函 数 名: IIC_Stop
*	功能说明: 
*	形    参:  	无
*	返 回 值: 	无
*********************************************************************************************************
*/
static void IIC_Stop(void)
{
    SCL_Reset;
    Delay_uS(IIC_uS);
    SDA_Reset;
    Delay_uS(IIC_uS);
    SCL_Set;
    Delay_uS(IIC_uS);
    SDA_Set;
    Delay_uS(IIC_uS);
}


/*
*********************************************************************************************************
*	函 数 名: IIC_Wait_Ack
*	功能说明: 
*	形    参:  	无
*	返 回 值: 	无
*********************************************************************************************************
*/
static uint8_t IIC_Wait_Ack(void)
{	
	uint8_t timeout = 1;
	SCL_Set;
	Delay_uS(5);
	SCL_Reset;
	while((SDA_Read)&&(timeout<=100))
	{
	  timeout++;
	}
	Delay_uS(5);
	SCL_Reset;
	return 0;
}


/*
*********************************************************************************************************
*	函 数 名: IIC_Wr_Byte
*	功能说明: 	I2C写数据
*	形    参:  	无
*	返 回 值: 	无
*********************************************************************************************************
*/
static void IIC_Wr_Byte(uint8_t Data)
{
    uint8_t i = 0;

    for(i = 0; i < 8; i++)
    {
        SCL_Reset;
        Delay_uS(IIC_uS);

        if(Data >> 7)
        {
            SDA_Set;
        }
        else
        {
            SDA_Reset;
        }
        Data <<= 1;
        Delay_uS(IIC_uS);

        SCL_Set;
        Delay_uS(IIC_uS);
    }
    SCL_Reset;
    Delay_uS(IIC_uS);
    SDA_Set;
    Delay_uS(IIC_uS);
}


/**
 * @brief Write data to AIP650 RAM
 * @param Address   :The RAM address to write to
 *        Data      :The data byte to write
 * @note This function uses IIC interface to communicate with AIP650
 */
static void AIP650_Wr_RAM(uint8_t Address, uint8_t Data)
{
    IIC_Start();
    IIC_Wr_Byte(Address);
    IIC_Wait_Ack();
    IIC_Wr_Byte(Data);
    IIC_Wait_Ack();
    IIC_Stop();
}


/**
 * @brief Display numbers on the tube segments
 * @param Dis Pointer to an array of 4 digits (0-9) to be displayed
 * @note Uses AIP650_Wr_RAM to write display data to each digit segment (DIG0-DIG3)
 */
void Tube_DisNum(uint8_t *Dis)
{
    /*显示*/
    AIP650_Wr_RAM(CMD_DIG0, DISPLAY_NUM[Dis[0]]);
    AIP650_Wr_RAM(CMD_DIG1, DISPLAY_NUM[Dis[1]]);
    AIP650_Wr_RAM(CMD_DIG2, DISPLAY_NUM[Dis[2]]);
    AIP650_Wr_RAM(CMD_DIG3, DISPLAY_NUM[Dis[3]]);
}


/**
 * @brief Display error code on AIP650 display module
 * @param Err_code Error code to display (0-9). Values >=10 will be clamped to 9.
 * @note Displays "ErrX" where X is the error code digit.
 *       Uses AIP650_Wr_RAM to write to display RAM directly.
 */
void bsp_DisplayErrAip650(uint8_t Err_code)
{
    // Clamp error code to 9
    if(Err_code	>=	10)
	{
				Err_code=9;
	}
	//display "ErrX" where X is the error code digit
   AIP650_Wr_RAM(CMD_DIG0, DISPLAY_NUM[10]);					//E
   AIP650_Wr_RAM(CMD_DIG1, DISPLAY_NUM[11]);					//r
   AIP650_Wr_RAM(CMD_DIG2, DISPLAY_NUM[11]);					//r
   AIP650_Wr_RAM(CMD_DIG3, DISPLAY_NUM[Err_code]);		//0~9
	
}

void bsp_system_open_display(uint32_t delay_time )
{
    uint32_t s_time = 0;

    AIP650_Wr_RAM(CMD_DIG0, DISPLAY_NUM[15]);		//no
    AIP650_Wr_RAM(CMD_DIG1, DISPLAY_NUM[15]);		//no
    AIP650_Wr_RAM(CMD_DIG2, DISPLAY_NUM[15]);		//no
    AIP650_Wr_RAM(CMD_DIG3, DISPLAY_NUM[15]);		//no

    s_time = delay_time/4;
	/* 显示-	*/
   AIP650_Wr_RAM(CMD_DIG0, DISPLAY_NUM[14]);		//-
    delay_1ms(s_time);
   AIP650_Wr_RAM(CMD_DIG1, DISPLAY_NUM[14]);		//-
    delay_1ms(s_time);
   AIP650_Wr_RAM(CMD_DIG2, DISPLAY_NUM[14]);		//-
    delay_1ms(s_time);
   AIP650_Wr_RAM(CMD_DIG3, DISPLAY_NUM[14]);		//-
	delay_1ms(s_time);
}




