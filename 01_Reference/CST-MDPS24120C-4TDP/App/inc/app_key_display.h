/*
*********************************************************************************************************
*
*	模块名称 : 按键显示模块
*	文件名称 : bsp_key_display.h
*	版    本 : V1.0
*	说    明 : 采用串口中断+FIFO模式实现多个串口的同时访问
*	修改记录 :
*		版本号  日期       	作者    说明
*		V1.0    2024-05-28 	Alans  	正式发布
*
*********************************************************************************************************
*/
#ifndef __APP_KEY_DISPLAY_H
#define __APP_KEY_DISPLAY_H

#include "bsp_aip650.h"
#include "bsp_key.h"
#include <stdint.h>


#define PAGE_MAX			    CHANNEL_NUM + 1

#define PAGE_BRIGHTNESS		    0
#define PAGE_STROBE_BASE	    1
#define PAGE_MODE			    (CHANNEL_NUM + 1)
#define PAGE_TOTAL			    (CHANNEL_NUM + 2)

#define USING_API650		    0
#define USING_TM1637		    1

#define CHANNEL_SETTING_TIMEOUT         500         // 5second timeout (Assuming a call every 10ms)
#define FLASH_INTERVAL                  40          // Blink interval (250ms)
#define IDLE_TIMEOUT_MS                 5000        // 5second no key press idle timeout

#define SCAN_ANIMATION_UPDATE_INTERVAL  20      // animation update interval
#define SCAN_ANIMATION_STEPS            5       // animation step
#define SCAN_MIN_ANIMATION_DISPLAYS     2     // 最小动画显示次数
#define SCAN_RESULT_DISPLAY_TIME_MS     1500   // 结果显示时间（毫秒）

typedef enum {
    PAGE_BRIGHTNESS_NORMAL = 0, // 亮度与常亮/常灭设置页
    PAGE_STROBE_WIDTH      = 1, // 频闪脉宽设置页
    //PAGE_WORK_MODE         = 2, // 工作模式设置页
    PAGE_MAIN_MAX               // 主页面总数
} main_page_t;

typedef enum
{
    MODE_PAGE = 0,	
    CHANNEL_1_PAGE = 1,	
    CHANNEL_2_PAGE = 2,	
    CHANNEL_3_PAGE  ,	
    CHANNEL_4_PAGE  ,	
    CHANNEL_5_PAGE  ,	
    CHANNEL_6_PAGE  ,	
    CHANNEL_7_PAGE  ,	
    CHANNEL_8_PAGE  ,	
}display_page_name;

typedef enum
{
    MODIFY_MODE_DISABLE = 0,	//修改模式关闭
    MODIFY_MODE_ENABLE		    //修改模式开启
}display_modify_mode;

typedef enum
{
    DISPLAY_RS232_DISABLE = 0,	
    DISPLAY_RS232_ENABLE		
}display_rs232_mode_t;

typedef enum {
    SCAN_DISPLAY_IDLE,        
    SCAN_DISPLAY_ANIMATING,   
    SCAN_DISPLAY_RESULT       
} scan_display_state_t;

typedef struct{
    scan_display_state_t state;
    uint32_t             display_start_time;
    uint8_t              result_displayed;
    uint8_t              animation_displayed_count;
}scan_display_context_t;

extern scan_display_context_t g_scan_display_ctx;



//显示页面变量，尽量不要直接修改变量，请通过函数修改
//extern uint8_t	g_DisPlayPage;

extern uint8_t	g_test_DisPlayPage;



/*	数码管显示的核心函数	*/
void bsp_ChangeDisPlayPage(uint16_t	_usChannel);
/*	数码管显示的核心函数	*/
void bsp_DisplayHandler(void);
/*	按键处理函数	*/
void app_Key_Handler(void);
/*  数码管显示错误状态函数  */

void bsp_ErrorDisplay(void);

uint8_t bsp_get_main_page(void);

int bsp_KeyIncLightValue(const uint16_t _usDisPlayPage);
int bsp_KeyDecLightValue(const uint16_t _usDisPlayPage);
void normal_mode_increase(void);
void bsp_clear_display(void);
void bsp_exit_channel_setting_display(void);

// V1.3 - 从机查找显示控制
uint8_t is_found_slave_display_enabled(void);
void enable_found_slave_display(void);
void disable_found_slave_display(void);

#endif
