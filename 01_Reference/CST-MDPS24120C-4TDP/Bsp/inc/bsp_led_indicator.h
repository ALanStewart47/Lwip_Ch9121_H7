/*
*********************************************************************************************************
*
*	模块名称 : 数码管驱动IC-驱动模块
*	文件名称 : bsp_aip650.h
*	版    本 : V1.0
*	说    明 : Aip650的底层驱动代码，使用模拟IIC来驱动，基于Hal库
*
*	修改记录 :
*		版本号  日期       		 作者     说明
*		V1.0    2025-05-14 		 alan  		
*
*********************************************************************************************************
*/
#ifndef __BSP_LED_INDICATOR_H
#define __BSP_LED_INDICATOR_H

#include "main.h"

#define MODE_LED_PIN		GPIO_PIN_9
#define MODE_LED_PORT		GPIOB

#define POWER_LED_PIN		GPIO_PIN_8
#define POWER_LED_PORT		GPIOB

#define ERROR_LED_PIN		GPIO_PIN_12
#define ERROR_LED_PORT		GPIOA


typedef enum
{
    LED_MODE    = 0 ,
    LED_POWER   = 1 ,
    LED_ERROR   = 2 ,
}led_type_t;


//声明数码管显示内容数组
void indicator_led_hard_init(void);
void open_led_indicator(uint8_t led_type);
void close_led_indicator(uint8_t led_type);

void error_led_indicator_handle(void);
void mode_led_indicator_handle(void);
#endif 


