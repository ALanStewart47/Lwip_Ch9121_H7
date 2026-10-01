/*
*********************************************************************************************************
 * Copyright (C) 2025 CST, Inc.(Gmbh) or its affiliates.
 *
 * All Rights Reserved.
 *
 * @file bsp_key_display.c
 *
 * @par dependencies
 * - bsp_key_display.h
 *
 * @author ALan | R&D Dept. | CST
 *
 * @brief Provide the HAL APIs of the flash read and write.
 *
 * Processing flow:
 * call directly.
 *
 * @version 	V1.0 	2025-07-17  	ALan 
 * 				V1.1 	2025-07-20  	ALan
 * 				V1.2	2025-08-13      ALan
 *              v1.3    2025-10-27      ALan
 *              V1.4    2025-10-30      ALan
 * @note saving data to flash
 *       1 tab == 4 spaces!
 *
*********************************************************************************************************
*/
//******************************** Includes *********************************//
#include "app.h"
#include "app_key_display.h"
#include "app_slave_param_sync.h"
//******************************** Includes *********************************//

//******************************** Defines **********************************//
uint8_t	g_test_DisPlayPage			      = 0;
uint8_t g_modify_dac_flag			      = 0;  //Output dac mode , user can modify the dac to fix max output value
uint8_t g_brightness_subpage 		      = 0;  // 0:H__x 1:AXXX
uint8_t g_brightness_channel 		      = 0;  // 
uint8_t g_strobe_channel 			      = 0;  // 
//V1.1 	  
uint8_t g_SubPage     				      = 0;                      	
uint8_t g_display_rs232_flag 	          = DISPLAY_RS232_DISABLE; 
//V1.3	  
static uint8_t s_found_slave_display_enable = 0;  // 从机查找显示使能标志 (仅用于显示控制)
scan_display_context_t g_scan_display_ctx = {SCAN_DISPLAY_IDLE, 0, 0, 0};

//V1.4 - 从机参数支持（通道5-36）
uint8_t g_total_slave_count = 0;  // 在线从机数量
#define MASTER_CHANNEL_MAX  4      // 主机通道数 (CH1-4)


#if USING_AIP650
static uint8_t g_display_buffer[4];
#elif USING_TM1637
static uint8_t g_display_buffer[5];
#endif

uint8_t  g_channel_setting_mode 	      = 0;     // 通道设置状态 0:正常 1:通道设置
uint8_t  g_current_channel 			      = 0;     // 当前选中的通道 (0-3)
uint32_t g_channel_setting_timeout	      = 0;     // 通道设置超时计数器
uint8_t  g_channel_flash_flag 		      = 0;     // 通道闪烁标志
uint32_t g_flash_counter 			      = 0;     // 闪烁计数器
uint32_t g_led_flash_counter              = 0;     // LED闪烁计数器
uint32_t g_key_last_press_time            = 0;     // 按键最后按下时间

extern UART_HandleTypeDef huart2;
//******************************** Defines **********************************//


//******************************** Declaring ********************************//
static void GoToNextMainPage(void) 
{
    g_light_system.display_page = (g_light_system.display_page + 1) % PAGE_MAIN_MAX;
    g_SubPage = 0; 
}

uint8_t bsp_get_main_page(void) 
{
	return g_light_system.display_page;
}

static void GoToNextSubPage(void) 
{
    if (g_light_system.display_page == PAGE_BRIGHTNESS_NORMAL) 
	{
        // 更新在线从机数量
        g_total_slave_count = get_found_slave_count();
        // 计算总通道数：主机4通道 + 从机数量*4
        uint8_t total_channels = MASTER_CHANNEL_MAX + g_total_slave_count * MASTER_CHANNEL_MAX;
        g_SubPage = (g_SubPage + 1) % (total_channels + 1); // +1 for H/L mode page
    } 
	else if (g_light_system.display_page == PAGE_STROBE_WIDTH) 
	{
        // 频闪脉宽页面同样支持从机通道
        g_total_slave_count = get_found_slave_count();
        uint8_t total_channels = MASTER_CHANNEL_MAX + g_total_slave_count * MASTER_CHANNEL_MAX;
        g_SubPage = (g_SubPage + 1) % total_channels;
    }
}


static void EnterChannelSettingMode(void)
{
    g_channel_setting_mode 		= 1;
    g_channel_setting_timeout 	= CHANNEL_SETTING_TIMEOUT;
    g_flash_counter 			= 0;
    g_channel_flash_flag 		= 0;
}

static void ExitChannelSettingMode(void)
{
    g_channel_setting_mode 		= 0;
    g_channel_setting_timeout 	= 0;
    g_channel_flash_flag 		= 0;
}

static void SwitchToNextChannel(void)
{
	if (g_light_system.display_page == PAGE_BRIGHTNESS_NORMAL) {
        // 在亮度页面：0=H/L模式, 1-N=通道1-N (N=4+从机数*4)
        uint8_t slave_count     = get_found_slave_count();
        uint8_t total_channels  = CHANNEL_NUM + slave_count * CHANNEL_NUM; // 4 + 从机数 * 4
        g_SubPage               = (g_SubPage + 1) % (total_channels + 1);
    } 
	else if (g_light_system.display_page == PAGE_STROBE_WIDTH) {
        uint8_t slave_count     = get_found_slave_count();
        uint8_t total_channels  = CHANNEL_NUM + slave_count * CHANNEL_NUM;
        g_SubPage               = (g_SubPage + 1) % total_channels;
    }
    g_channel_setting_timeout   = CHANNEL_SETTING_TIMEOUT; // reset timeout
}

static void SwitchToPrevChannel(void)
{
    if (g_light_system.display_page == PAGE_BRIGHTNESS_NORMAL) {
        uint8_t slave_count     = get_found_slave_count();
        uint8_t total_channels  = CHANNEL_NUM + slave_count * CHANNEL_NUM;
        if (g_SubPage == 0) {
            g_SubPage = total_channels;  // 从H/L模式跳到最后一个通道
        } 
        else {
            g_SubPage--;
        }
    } 
    else if (g_light_system.display_page == PAGE_STROBE_WIDTH) {
        uint8_t slave_count     = get_found_slave_count();
        uint8_t total_channels  = CHANNEL_NUM + slave_count * CHANNEL_NUM;
        if (g_SubPage == 0) {
            g_SubPage           = total_channels - 1;
        } 
        else {
            g_SubPage--;
        }
    }
    g_channel_setting_timeout = CHANNEL_SETTING_TIMEOUT; // reset timeout
}

// 新增：增加当前通道的数值
static void IncreaseChannelValue(void)
{
    if (g_light_system.display_page == PAGE_BRIGHTNESS_NORMAL) 
    {
        if (g_SubPage == 0) {
            // H/L模式页面：切换H/L状态，并广播给所有从机
            uint8_t current_state   = get_light_normal_mode_state();
            current_state           = (current_state + 1) % 2;
            uint8_t slave_count     = get_found_slave_count();

            set_light_normal_mode_state(current_state);
            for (uint8_t slave_id = 1; slave_id <= slave_count; slave_id++) {
                set_slave_normal_mode_state(slave_id, current_state);  // 修复: 从机应该和主机状态一致
            }
            set_light_mode(WORK_MODE_NORMAL);
        } 
        else 
        {
            // 通道页面：判断是主机通道还是从机通道
            uint8_t channel = g_SubPage - 1;  // 转换为0-based
            
            if (channel < MASTER_CHANNEL_MAX) {
                // 主机通道 (CH1-4)
                uint16_t brightness = get_light_value(channel);
                uint16_t max_val    = (get_light_brightness_range() == BRIGHTNESS_255) ? 255 : 999;
                if (brightness < max_val) {
                    set_light_value(channel, brightness + 1);
                }
                set_light_enable_state(channel, LIGHT_ENABLE);
            } else {
                // 从机通道 (CH5+)
                uint8_t slave_channel_index         = channel - MASTER_CHANNEL_MAX; // 从0开始的从机通道索引
                uint8_t slave_id                    = slave_channel_index / MASTER_CHANNEL_MAX + 1; // 从机ID (1-based)
                uint8_t slave_ch                    = slave_channel_index % MASTER_CHANNEL_MAX; // 从机内部通道 (0-3)
                
                // 从缓存获取当前亮度
                const slave_channel_param_t *param  = get_slave_channel_param(slave_id, slave_ch);
                uint16_t brightness                 = param ? param->brightness : 0;
				uint16_t max_val    				= (get_light_brightness_range() == BRIGHTNESS_255) ? 255 : 999;

                if (brightness < max_val) {
                    set_slave_brightness(slave_id, slave_ch, brightness + 1);
                }
            }
        }
    }
    else if (g_light_system.display_page == PAGE_STROBE_WIDTH) 
    {
        uint8_t channel = g_SubPage;
        
        if (channel < MASTER_CHANNEL_MAX) {
            // 主机通道脉宽
            uint16_t width = get_light_strobe_width(channel);
            if (width < 999) {
                set_light_strobe_width(channel, width + 1);
            }
        } else {
            // 从机通道脉宽
            uint8_t slave_channel_index = channel - MASTER_CHANNEL_MAX;
            uint8_t slave_id            = slave_channel_index / MASTER_CHANNEL_MAX + 1;
            uint8_t slave_ch            = slave_channel_index % MASTER_CHANNEL_MAX;
    
            const slave_channel_param_t *param = get_slave_channel_param(slave_id, slave_ch);
            uint16_t width              = param ? param->strobe_width : 0;
            
            if (width < 999) {
                set_slave_strobe_width(slave_id, slave_ch, width + 1);
            }
        }
    }
}

// 新增：减少当前通道数值
static void DecreaseChannelValue(void)
{
    if (g_light_system.display_page == PAGE_BRIGHTNESS_NORMAL) 
    {
        if (g_SubPage == 0) 
        {
            // H/L模式页面：切换H/L状态，并广播给所有从机
            uint8_t current_state = get_light_normal_mode_state();
            current_state = 1 - current_state;  // 0?1 切换
            set_light_normal_mode_state(current_state);
            // 广播给所有从机 (实时获取从机数量)
            uint8_t slave_count = get_found_slave_count();
            for (uint8_t slave_id = 1; slave_id <= slave_count; slave_id++) {
                set_slave_normal_mode_state(slave_id, current_state);  // 修复: 从机应该和主机状态一致
            }
            set_light_mode(WORK_MODE_NORMAL);
        } 
        else 
        {
            // 通道页面：判断是主机通道还是从机通道
            uint8_t channel = g_SubPage - 1;
            
            if (channel < MASTER_CHANNEL_MAX) {
                // 主机通道
                uint16_t brightness = get_light_value(channel);
                if (brightness > 0) {
                    set_light_value(channel, brightness - 1);
                }
                set_light_enable_state(channel, LIGHT_ENABLE);
            } else {
                // 从机通道
                uint8_t slave_channel_index = channel - MASTER_CHANNEL_MAX;
                uint8_t slave_id = slave_channel_index / MASTER_CHANNEL_MAX + 1;
                uint8_t slave_ch = slave_channel_index % MASTER_CHANNEL_MAX;
                
                const slave_channel_param_t *param = get_slave_channel_param(slave_id, slave_ch);
                uint16_t brightness = param ? param->brightness : 0;
                
                if (brightness > 0) {
                    set_slave_brightness(slave_id, slave_ch, brightness - 1);
                }
            }
        }
    }
    else if (g_light_system.display_page == PAGE_STROBE_WIDTH) 
    {
        uint8_t channel = g_SubPage;
        
        if (channel < MASTER_CHANNEL_MAX) {
            // 主机通道脉宽
            uint16_t width = get_light_strobe_width(channel);
            if (width > 0) {
                set_light_strobe_width(channel, width - 1);
            }
        } else {
            // 从机通道脉宽
            uint8_t slave_channel_index = channel - MASTER_CHANNEL_MAX;
            uint8_t slave_id = slave_channel_index / MASTER_CHANNEL_MAX + 1;
            uint8_t slave_ch = slave_channel_index % MASTER_CHANNEL_MAX;
            
            const slave_channel_param_t *param = get_slave_channel_param(slave_id, slave_ch);
            uint16_t width = param ? param->strobe_width : 0;
            
            if (width > 0) {
                set_slave_strobe_width(slave_id, slave_ch, width - 1);
            }
        }
    }
}

//*************** RS232 ***************//
uint8_t get_display_rs232_flag(void){
	return g_display_rs232_flag;
}

void enable_display_rs232_flag(void){
	g_display_rs232_flag = DISPLAY_RS232_ENABLE;
}

void disable_display_rs232_flag(void){
	g_display_rs232_flag = DISPLAY_RS232_DISABLE;
}

void switch_rs232_mode(void){
	if(g_display_rs232_flag == DISPLAY_RS232_ENABLE){
		g_display_rs232_flag = DISPLAY_RS232_DISABLE;
	}
	else{
		g_display_rs232_flag = DISPLAY_RS232_ENABLE;
	}
}
//*************** RS232 ***************//


//V1.3
//*************** Master found slave display control ***************//
uint8_t is_found_slave_display_enabled(void)
{
    return s_found_slave_display_enable;
}

void enable_found_slave_display(void)
{
    s_found_slave_display_enable = 1;
}

void disable_found_slave_display(void)
{
    s_found_slave_display_enable = 0;
}
//*************** master found slave display control ***************//



int bsp_KeyIncLightValue(const uint16_t _usDisPlayPage)
{
	uint16_t ch			= _usDisPlayPage - 1;
	uint16_t _usValue	= 0;	
	uint16_t value_max 	= 0;
	
	if(	(ch	<	CHANNEL_NUM))
	{
		if(get_light_brightness_range() == BRIGHTNESS_255)
			value_max = LIGHT_VALUE_255_MAX;
		else if(get_light_brightness_range() == BRIGHTNESS_999)
			value_max = LIGHT_VALUE_999_MAX;
		
		set_light_enable_state(ch, LIGHT_ENABLE); 

		if(get_light_value(ch)	>=	value_max)
		{
			set_light_value(ch, value_max);
			return TRUE;
		}
		else
		{
			_usValue = get_light_value(ch);
			if(	_usValue >= value_max){
				_usValue = value_max;
			}
			else{
				++_usValue;
			}
			set_light_value(ch,_usValue);
			return TRUE;
		}
	}
	else
	{
		return FALSE;
	}
}

int bsp_KeyDecLightValue(const uint16_t _usDisPlayPage)
{
	uint16_t ch			=	_usDisPlayPage	-	1;		
	uint16_t _usValue	=	0;	
		
	if(ch < CHANNEL_NUM)
	{
		set_light_enable_state(ch, LIGHT_ENABLE); 
		
		if(get_light_value(ch) == LIGHT_VALUE_MIN){
			return 0;
		}
		else{
			_usValue = get_light_value(ch);
			if(_usValue	==	0){
				return 0;
			}
			else{
				--_usValue;
				set_light_value(ch,_usValue);
			}
			return 0;
		}
	}
	else
	{
		return -1;
	}
}

void normal_mode_increase(void)
{
	if (g_light_system.normal_mode_state == L_MODE) 
	{
		g_light_system.normal_mode_state = H_MODE;
	} 
	else if (g_light_system.normal_mode_state == H_MODE) 
	{
		g_light_system.normal_mode_state = L_MODE;
	}
}

static void bsp_Mode_HL_Display(void)
{
#if USING_AIP650
	static const uint8_t template[] = {12, 15, 15, 0}; 
	uint8_t h_l_state = g_light_system.normal_mode_state;

	memcpy(g_display_buffer, template, sizeof(template));

	g_display_buffer[3] = h_l_state;	
	Tube_DisNum(g_display_buffer);
#elif USING_TM1637
	static const uint8_t template[] = {16, 17, 17, 17,0}; //H____x
	uint8_t h_l_state = g_light_system.normal_mode_state;

	memcpy(g_display_buffer, template, sizeof(template));

	if (g_channel_setting_mode && g_channel_flash_flag) {
        g_display_buffer[0] = 17;  // 空白，实现闪烁效果
    } else {
        g_display_buffer[0] = 16;  // 'H'
    }

	g_display_buffer[4] = h_l_state;	
	Tube_DisNum(g_display_buffer);
#endif
}

static void bsp_BrightnessDisplay(uint8_t ch)
{
    uint16_t val;
    uint8_t channel_num = ch + 1; // 默认显示通道号 (1-based)
    
    // 判断是主机通道还是从机通道
    if (ch < MASTER_CHANNEL_MAX) {
        // 主机通道 (CH1-4)
        val = get_light_value(ch);
    } else {
        // 从机通道 (CH5+)
        uint8_t slave_channel_index = ch - MASTER_CHANNEL_MAX;
        uint8_t slave_id = slave_channel_index / MASTER_CHANNEL_MAX + 1;
        uint8_t slave_ch = slave_channel_index % MASTER_CHANNEL_MAX;
        
        // 从缓存中获取亮度
        const slave_channel_param_t *param = get_slave_channel_param(slave_id, slave_ch);
        val = param ? param->brightness : 0;
        
        // 通道号显示为5-36
        channel_num = ch + 1;
    }

	    // 通道显示部分（前两位），支持闪烁
    if (g_channel_setting_mode && g_channel_flash_flag) {
        // 通道设置模式下闪烁时显示空白
        g_display_buffer[0] = 17;  // 空白
        g_display_buffer[1] = 17;  // 空白
    } else {
        g_display_buffer[0] = channel_num / 10 % 10;
        g_display_buffer[1] = channel_num % 10;
    }
    g_display_buffer[2] = (val / 100) % 10;
    g_display_buffer[3] = (val / 10) % 10;
    g_display_buffer[4] = val % 10;
    Tube_DisNum(g_display_buffer);
}

static void bsp_StrobePageDisplay(uint8_t ch)
{
	uint16_t width;
	uint8_t channel_num = ch + 1;
	
	// 判断是主机通道还是从机通道
    if (ch < MASTER_CHANNEL_MAX) {
        // 主机通道
        width = g_light_system.channel[ch].strobe_width;
    } else {
        // 从机通道
        uint8_t slave_channel_index = ch - MASTER_CHANNEL_MAX;
        uint8_t slave_id = slave_channel_index / MASTER_CHANNEL_MAX + 1;
        uint8_t slave_ch = slave_channel_index % MASTER_CHANNEL_MAX;
        
        // 从缓存中获取脉宽
        const slave_channel_param_t *param = get_slave_channel_param(slave_id, slave_ch);
        width = param ? param->strobe_width : 0;
        
        // 通道号显示为5-36
        channel_num = ch + 1;
    }

    if (g_channel_setting_mode && g_channel_flash_flag) {
        // 通道设置模式下闪烁时显示空白
        g_display_buffer[0] = 17;  // 空白
        g_display_buffer[1] = 17;  // 空白
    } else {
        g_display_buffer[0] = channel_num / 10 % 10;
        g_display_buffer[1] = channel_num % 10;
    }
	
    g_display_buffer[2] = (width / 100) % 10;
    g_display_buffer[3] = (width / 10) % 10;
    g_display_buffer[4] = width % 10;
    Tube_DisNum(g_display_buffer);
}

#if 0
static void bsp_ModePageDisplay(void)
{
#if USING_AIP650
    uint8_t s_DisBuff[4] = {13, 15, 15, 0};
	memcpy(g_display_buffer, s_DisBuff, sizeof(s_DisBuff));
	
	// 模式统一显示主机的模式
	uint8_t mode = g_light_system.work_mode;

    g_display_buffer[3] = mode + 1; // 1/2/3
    Tube_DisNum(g_display_buffer);
#elif USING_TM1637
    static const uint8_t template[5] = {18, 17, 17, 17, 0}; //U__x
    memcpy(g_display_buffer, template, sizeof(template));

    g_display_buffer[4] = g_light_system.work_mode + 1; // 1/2/3
    Tube_DisNum(g_display_buffer);
#endif
}
#endif

void bsp_clear_display(void)
{  
	static const uint8_t template[] = {17, 17, 17, 17,17}; //H____x
    memcpy(g_display_buffer , template , sizeof(g_display_buffer)); // 17 = ' '
    Tube_DisNum(g_display_buffer);
}

void bsp_exit_channel_setting_display(void)
{
    uint8_t digitArr[6] = {0}; // Display "no" with LED1 on
    for(uint8_t i = 0; i < 5; i++)
    {
        digitArr[i] = 0x40; // 14 = '-'
        tm1637_dis(digitArr);
      
    }
}

/**
 * @brief display dac modify mode
 */
static void display_rs232_mode(void)
{
#if USING_AIP650
	uint8_t s_DisBuff[4]={16,15,15,0}; 	//b__x

	memcpy(g_display_buffer, s_DisBuff, sizeof(s_DisBuff));

	g_display_buffer[3] = g_light_system.uart_baud_gear%10;
	Tube_DisNum(g_display_buffer);
#elif USING_TM1637
    uint8_t template[5]={11,17,17,17,0};
    template[4] = g_light_system.uart_baud_gear % 10; // 显示波特率档位
    memcpy(g_display_buffer, template, sizeof(template));
    Tube_DisNum(g_display_buffer);
#endif
}


//v1.3
/**
 * @brief Display the scan result on the display.
 * @param scan_result The result of the scan (success or stopped).
 * @note This function manages the display of the scan result for a specified duration.
 */
static void display_scan_result(scan_result_t scan_result)
{
    if (g_scan_display_ctx.result_displayed == 0)
    {
        g_scan_display_ctx.result_displayed     = 1;
        g_scan_display_ctx.display_start_time   = bsp_GetRunTime();
        g_scan_display_ctx.state                = SCAN_DISPLAY_RESULT;

        if (scan_result == SCAN_RESULT_SUCCESS){
            LOG("Display: scan result success");
            uint8_t slave_buf[5] = {15, 17, 17, 17, 0};
            slave_buf[4] = get_found_slave_count();
            memcpy(g_display_buffer, slave_buf, sizeof(slave_buf));
            Tube_DisNum(g_display_buffer);
        }
        else{ // SCAN_RESULT_STOPPED
            LOG("Display: scan result stopped");
            uint8_t stop_buf[5] = {15, 17, 17, 17, 14}; // F__E
            if (get_found_slave_count() > 0){
                stop_buf[4] = get_found_slave_count();
            }
            else{
            stop_buf[4] = 14; // 'E'
            }
            memcpy(g_display_buffer, stop_buf, sizeof(stop_buf));
            Tube_DisNum(g_display_buffer);
        }
    }

    if (bsp_CheckRunTime(g_scan_display_ctx.display_start_time) > SCAN_RESULT_DISPLAY_TIME_MS) {
        disable_found_slave_display();
        clear_scan_result();
        g_scan_display_ctx.result_displayed = 0;
        g_scan_display_ctx.state = SCAN_DISPLAY_IDLE;
        LOG("Display: exiting slave found display");
    }
}


/**
 * @brief Display scanning animation on the display.
 * @note This function updates the display to show a scanning animation.
 * 
 */
static void display_scan_animation(void) 
{
    static uint8_t scan_display_counter = 0;
    static uint8_t scan_animation_step = 0;

    g_scan_display_ctx.result_displayed = 0;
    g_scan_display_ctx.state = SCAN_DISPLAY_ANIMATING;

    scan_display_counter++;
    if (scan_display_counter >= SCAN_ANIMATION_UPDATE_INTERVAL) {
        scan_display_counter = 0;
        uint8_t anim_buffer[5] = {17, 17, 17, 17, 17};
        anim_buffer[scan_animation_step] = 15;  // 显示当前步骤的 'F'
        memcpy(g_display_buffer, anim_buffer, sizeof(anim_buffer));
        Tube_DisNum(g_display_buffer);

        scan_animation_step = (scan_animation_step + 1) % SCAN_ANIMATION_STEPS;  // 循环 0-4
        if (scan_animation_step == 0) {  // 完成一个完整循环
            g_scan_display_ctx.animation_displayed_count++;
            LOG("Display: scanning... cycle %d", g_scan_display_ctx.animation_displayed_count);
        }
    }
}




/**
 * @brief Display handler for the main display logic.
 * @note This function handles the display logic based on the current page and subpage.
 */
void bsp_DisplayHandler(void)
{
	// 处理通道设置超时
    if (g_channel_setting_mode) {
        if (g_channel_setting_timeout > 0) {
            g_channel_setting_timeout--;
        } else {
            ExitChannelSettingMode();
        }
        
        // 处理闪烁
        g_flash_counter++;
        if (g_flash_counter >= FLASH_INTERVAL) {
            g_flash_counter = 0;
            g_channel_flash_flag = !g_channel_flash_flag;
        }
    }

    // Check for idle timeout (5 seconds no key press)
    // Skip timeout check during special modes
    if (!is_found_slave_display_enabled() && 
        get_display_rs232_flag() != DISPLAY_RS232_ENABLE &&
        !g_channel_setting_mode &&
        get_light_alarm() == STATUS_NORMAL) {
        
        /*if (bsp_CheckRunTime(g_key_last_press_time) > IDLE_TIMEOUT_MS) {
            // Reset display to home page
            if (g_light_system.display_page != 0 || g_SubPage != 0) {
                g_light_system.display_page = 0;
                g_SubPage = 0;
            }
            // Pause parameter synchronization
            //if (!is_param_sync_paused()) {
            //    pause_slave_param_sync();
            //}
        }*/
    }

	if(get_light_alarm() != STATUS_NORMAL)
	{
		bsp_ErrorDisplay();
	}

    if (get_display_rs232_flag() == DISPLAY_RS232_ENABLE) {
        display_rs232_mode();
        return;
    }


    if (is_found_slave_display_enabled()) {
        scan_result_t scan_result = get_scan_result();

        // 如果扫描完成且动画已显示足够次数，则显示结果
        if (scan_result != SCAN_RESULT_NONE && g_scan_display_ctx.animation_displayed_count >= SCAN_MIN_ANIMATION_DISPLAYS) {
            display_scan_result(scan_result);
        } else {
            // 否则继续显示动画
            display_scan_animation();
        }
        return;
    } 
    else {
        // 退出扫描模式时重置所有状态
        g_scan_display_ctx.result_displayed = 0;
        g_scan_display_ctx.animation_displayed_count = 0;
        g_scan_display_ctx.state = SCAN_DISPLAY_IDLE;
    }

    switch (g_light_system.display_page) {
        case PAGE_BRIGHTNESS_NORMAL:
            if (g_SubPage == 0) {
                bsp_Mode_HL_Display();
            } else {
                bsp_BrightnessDisplay(g_SubPage - 1);
            }
            break;
        case PAGE_STROBE_WIDTH:
            bsp_StrobePageDisplay(g_SubPage);
            break;
        //case PAGE_WORK_MODE:
        //    bsp_ModePageDisplay();
        //    break;
    }
	Led_Dis();

}


void bsp_ErrorDisplay(void)
{
	if( get_light_alarm() != STATUS_NORMAL )
    {	
        uint8_t error_number = get_light_alarm();
       	uint8_t template[] = {14, 17, 17, 17,0}; //E____x
        template[4] = error_number ; 
		memcpy(g_display_buffer , template , sizeof(g_display_buffer)); // 17 = ' '
        Tube_DisNum(g_display_buffer);
    }
}

/**
 * @brief	 app_Key_Handler
 * @note     This function handles key events and updates the display or system state accordingly.	      
 * @param    void
 * @retval   void
 */
void app_Key_Handler(void)
{
	uint8_t ucKeyCode = bsp_GetKey();
	if (ucKeyCode == KEY_NONE) return;

    //V1.3 - 扫描显示期间禁用按键
    if (is_found_slave_display_enabled()) return;

    // Resume parameter synchronization if paused (wake up from idle)
    //if (is_param_sync_paused()) {
    //   resume_slave_param_sync();
    //}

	//_key_last_press_time = bsp_GetRunTime();

	if(ucKeyCode == SYS_LONG_K1K3) {
        switch_rs232_mode();
        return;
    }
    if (ucKeyCode == SYS_LONG_K2K3){
        if (!is_found_slave_display_enabled()) {
            enable_found_slave_display();
            master_find_slave_ctrl(true);
        }
        return;
    }

	if (g_display_rs232_flag == DISPLAY_RS232_ENABLE) {
        if (ucKeyCode == KEY_DOWN_K3) {
            inc_rs232_baud_gear();
            bsp_rs232_reinit();
            bsp_SaveBaudData();
        } else if (ucKeyCode == KEY_DOWN_K4) {
            dec_rs232_baud_gear();
            bsp_rs232_reinit();
            bsp_SaveBaudData();
        }
        return;
    }

	if (g_channel_setting_mode) 
	{
        switch (ucKeyCode) {
            case KEY_DOWN_K2:  // CH键：退出通道设置模式
                ExitChannelSettingMode();
                break;
                
            case KEY_DOWN_K3:  // +键：短按切换下一个通道，长按增加数值
                SwitchToNextChannel();
                break;
                
            case KEY_LONG_K3:  // +键长按：增加当前通道数值
                IncreaseChannelValue();
                g_channel_setting_timeout = CHANNEL_SETTING_TIMEOUT; // 重置超时
                break;
                
            case KEY_4_DOWN:   // -键：短按切换上一个通道，长按减少数值
                SwitchToPrevChannel();
                break;
                
            case KEY_LONG_K4:  // -键长按：减少当前通道数值
                DecreaseChannelValue();
                g_channel_setting_timeout = CHANNEL_SETTING_TIMEOUT; // 重置超时
                break;
                
            default:
                break;
        }
        return;
    }

	switch (ucKeyCode)
	{	
		case KEY_DOWN_K1:
			GoToNextMainPage();			
			break;

		case KEY_LONG_K1:
			//GoToNextMainPage();
		break;
	
		case KEY_DOWN_K2:
			GoToNextSubPage(); 
		break;

        case KEY_LONG_K2:
            if (g_light_system.display_page == PAGE_BRIGHTNESS_NORMAL ||
                g_light_system.display_page == PAGE_STROBE_WIDTH) {
                EnterChannelSettingMode();
            }
        break;

		case KEY_DOWN_K3:	
			if (g_light_system.display_page == PAGE_BRIGHTNESS_NORMAL)
			{
                if (g_SubPage == 0) 
				{ 
					normal_mode_increase();
					set_light_mode(WORK_MODE_NORMAL);
					// 广播 H/L 状态到所有在线从机
					uint8_t slave_count = get_found_slave_count();
					uint8_t h_l_state = get_light_normal_mode_state();
					for (uint8_t slave_id = 1; slave_id <= slave_count; slave_id++) {
						set_slave_normal_mode_state(slave_id, h_l_state);
					}
                }  
				else 
				{ 
					// 使用支持从机通道的函数
					IncreaseChannelValue();
                }
            } 
			else if (g_light_system.display_page == PAGE_STROBE_WIDTH) 
			{
                // 使用支持从机通道的函数
                IncreaseChannelValue();
            } 	
			/*else if (g_light_system.display_page == PAGE_WORK_MODE) 
			{
                uint8_t mode = get_light_mode();
                if (mode < WORK_MODE_PROG) {
                    mode++;
                    set_light_mode(mode);
                    // 广播到所有在线从机
                    uint8_t slave_count = get_found_slave_count();
                    for (uint8_t slave_id = 1; slave_id <= slave_count; slave_id++) {
                        set_slave_work_mode(slave_id, mode);
                    }
                }
            }*/
		break;

		case KEY_4_DOWN:
			if (g_light_system.display_page == PAGE_BRIGHTNESS_NORMAL) 
			{
                if (g_SubPage == 0) 
				{ 
					normal_mode_increase();
					set_light_mode(WORK_MODE_NORMAL);
					// 广播 H/L 状态到所有在线从机
					uint8_t slave_count = get_found_slave_count();
					uint8_t h_l_state = get_light_normal_mode_state();
					for (uint8_t slave_id = 1; slave_id <= slave_count; slave_id++) {
						set_slave_normal_mode_state(slave_id, h_l_state);
					}
                } 
				else
				{ 
					// 使用支持从机通道的函数
					DecreaseChannelValue();
                }
            } 
			else if (g_light_system.display_page == PAGE_STROBE_WIDTH) 
			{
                // 使用支持从机通道的函数
                DecreaseChannelValue();
            } 
			/*else if (g_light_system.display_page == PAGE_WORK_MODE) 
			{
                uint8_t mode = get_light_mode();
                if (mode > WORK_MODE_NORMAL) {
                    mode--;
                    set_light_mode(mode);
                    // 广播到所有在线从机
                    uint8_t slave_count = get_found_slave_count();
                    for (uint8_t slave_id = 1; slave_id <= slave_count; slave_id++) {
                        set_slave_work_mode(slave_id, mode);
                    }
                }
            }*/
		break;

		default:
			break;

	}
	
}
//******************************** Declaring ********************************//


