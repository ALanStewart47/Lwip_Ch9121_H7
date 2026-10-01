/******************************************************************************
 * Copyright (C) 2024 CST, Inc.(Gmbh) or its affiliates.
 *
 * All Rights Reserved.
 *
 * @file app_light.c
 *
 * @par dependencies
 * - app_light.h
 * - app.h
 *
 * @author Alan | R&D Dept. | CST
 *
 * @brief Provide the HAL APIs of the flash read and write.
 *
 * Processing flow:
 * call directly.
 *
 * @version V1.0    2025-05-      Alan
 *          V1.1    2025-07-15    Alan
 *          V1.2    2025-08-13    Alan
 *          V1.3    2025-12-15    ALan
 *          
 * @note light state detection, and make sure max dac output value.
 *       1 tab == 4 spaces!
 *
 *****************************************************************************/
#include "app_light.h"
#include "app.h"


system_param_t           g_light_system         = {0};
programmable_sync_mode_t g_prog_sync_mode       = SYNC_MODE_MASTER_SYNC;

#if STM32
    #if CHANNEL_NUM == 4
        uint16_t g_trig_pin[CHANNEL_NUM]        = {TRIG_IN_1_Pin,TRIG_IN_2_Pin,TRIG_IN_3_Pin,TRIG_IN_4_Pin};                       
        GPIO_TypeDef * g_trig_port[CHANNEL_NUM] = {TRIG_IN_1_GPIO_Port,TRIG_IN_2_GPIO_Port,
                                                    TRIG_IN_3_GPIO_Port,TRIG_IN_4_GPIO_Port};
    #endif
#endif

programmable_sync_mode_t get_programmable_sync_mode(void)
{
    return g_prog_sync_mode;
}

uint8_t get_programmable_sync_mode_value(void)
{
    if (g_prog_sync_mode == SYNC_MODE_MASTER_SYNC)
        return 0;
    else
        return 1;
}

/**
 * @brief set programmable sync mode
 * @param mode 0:sync sync 1:standlone
 */
void set_programmable_sync_mode_vlaue(uint8_t mode)
{
    if (mode > 1 )  
        mode = 1;

    if (mode == 1)
        g_prog_sync_mode = SYNC_MODE_STANDLONE;
    else
        g_prog_sync_mode = SYNC_MODE_MASTER_SYNC;
}
						
void light_config_init(void)
{
    g_light_system.save_prog_flag                   = 0;
    g_light_system.brightness_range                 = BRIGHTNESS_255;
    g_light_system.work_mode                        = WORK_MODE_NORMAL;
    g_light_system.normal_mode_state                = H_MODE;
    g_light_system.last_normal_mode_state           = 0;
    g_light_system.alarm_status                     = STATUS_NORMAL;
    g_light_system.programmable_unit                = PROG_UNIT_MS; 
    g_light_system.programmable_mode                = PROG_MODE_SINGLE_STEP;  
    g_light_system.programmable_step                = 0; 
    g_light_system.programmable_interval            = 0; 
    g_light_system.uart_baud_gear                   = 4; 
    g_light_system.display_page                     = PAGE_BRIGHTNESS_NORMAL; 
    g_prog_sync_mode                                = SYNC_MODE_MASTER_SYNC;

    for(uint8_t i = 0; i < CHANNEL_NUM; i++)
    {
        g_light_system.channel[i].brightness        = 0;
        g_light_system.channel[i].enabled           = LIGHT_ENABLE;
        g_light_system.channel[i].state             = LIGHT_OFF;
        g_light_system.channel[i].strobe_width      = 0;
        g_light_system.channel[i].soft_trig_width   = 0;
        g_light_system.channel[i].strobe_busy       = STROBE_TRIG_STATE_IDLE;
        g_light_system.channel[i].soft_trig         = NO_SOFT_TRIGGER_REQUEST;
        g_light_system.channel[i].ext_trig          = 0;
        g_light_system.channel[i].trig_count        = 0;
		g_light_system.channel[i].camout_width      = 1; 
    }
}

static void light_factory_reset_partial(void)
{
    g_light_system.work_mode = WORK_MODE_NORMAL;
    g_light_system.normal_mode_state = H_MODE;
	g_prog_sync_mode                                = SYNC_MODE_MASTER_SYNC;

    for(uint8_t i = 0; i < CHANNEL_NUM; i++)
    {
        g_light_system.channel[i].brightness        = 0;            
        g_light_system.channel[i].strobe_width      = 0;            
        g_light_system.channel[i].enabled           = LIGHT_ENABLE; 
        g_light_system.channel[i].state             = LIGHT_OFF;
        g_light_system.channel[i].strobe_busy       = STROBE_TRIG_STATE_IDLE;
        g_light_system.channel[i].soft_trig         = NO_SOFT_TRIGGER_REQUEST;
    }

    memset(g_light_system.prog_table, 0, sizeof(g_light_system.prog_table));
    g_light_system.programmable_unit                = PROG_UNIT_MS;
    g_light_system.programmable_mode                = PROG_MODE_SINGLE_STEP;
    g_light_system.programmable_step                = 0;
    g_light_system.programmable_interval            = 0;
    g_light_system.uart_baud_gear                   = 4; 
    g_light_system.alarm_status                     = STATUS_NORMAL;
	
	bsp_prog_trig_init();
	bsp_prog_trig_data_erase();
    // Note: This function intentionally does not modify g_light_system.brightness_range
}

void light_factory_reset(void)
{
    light_factory_reset_partial();
    g_light_system.brightness_range                 = BRIGHTNESS_255;
}


light_status_t set_light_brightness_range(uint8_t i)
{
    if(i != BRIGHTNESS_255 && i != BRIGHTNESS_999)
        return LIGHT_ERRORPARAMETER;

    if (g_light_system.brightness_range != i)
    {
        light_factory_reset_partial();
        g_light_system.brightness_range = i;
    }
    return LIGHT_OK;
}

uint8_t get_light_brightness_range(void)
{
    return g_light_system.brightness_range;
}


/**
 * @brief get light value 
 */
uint16_t get_light_value(uint8_t channel)
{
    if( channel >= CHANNEL_NUM )
        return 0;
    
    return g_light_system.channel[channel].brightness;
}

/**
 * @brief set light value
 * @return 1:success 0:fail
 */
light_status_t set_light_value(uint8_t channel, uint16_t value)
{
    if(channel >= CHANNEL_NUM)
        return LIGHT_ERROR;

    if(g_light_system.brightness_range == BRIGHTNESS_255){
        if(value > LIGHT_VALUE_255_MAX)
            value = LIGHT_VALUE_255_MAX;
    }

    if(g_light_system.brightness_range == BRIGHTNESS_999){
        if(value > LIGHT_VALUE_999_MAX)
            value = LIGHT_VALUE_999_MAX;
    }

    g_light_system.channel[channel].brightness = value;
    return LIGHT_OK;
}

uint16_t get_light_strobe_width(uint8_t channel)
{
    if(channel >= CHANNEL_NUM)
        return LIGHT_ERROR;

    return g_light_system.channel[channel].strobe_width;
}

light_status_t set_light_strobe_width(uint8_t channel, uint16_t width)
{
    if(channel >= CHANNEL_NUM)
        return LIGHT_ERROR;

    if(width > LIGHT_STROBE_WIDTH_MAX)
        width = LIGHT_STROBE_WIDTH_MAX;

    g_light_system.channel[channel].strobe_width = width;
    return LIGHT_OK;
}

/************************ light channel enable API ***************************/
uint8_t get_light_enable_state(uint8_t channel)
{
    if(channel >= CHANNEL_NUM)
        return LIGHT_DISABLE;

    return g_light_system.channel[channel].enabled;
}

light_status_t set_light_enable_state(uint8_t channel, uint8_t state)
{
    if(channel >= CHANNEL_NUM)
        return LIGHT_ERROR;

    if(state != LIGHT_ENABLE && state != LIGHT_DISABLE)
        return LIGHT_ERROR;

    g_light_system.channel[channel].enabled = state;
    return LIGHT_OK;
}

light_status_t set_all_light_enable_state(uint8_t state)
{
    if(state != LIGHT_ENABLE && state != LIGHT_DISABLE)
        return LIGHT_ERROR;

    for(uint8_t i = 0; i < CHANNEL_NUM; i++)
    {
        g_light_system.channel[i].enabled = state;
    }
    return LIGHT_OK;
}

/**
 * @brief get light mode .
 * @return 		  
 * */
uint8_t get_light_mode(void)
{
	return  g_light_system.work_mode;
}

/**
 * @brief set light mode .
 * @note  set H mode or L mode	
 * 
 * @return 1:success 0:fail		  
 * */
light_status_t set_light_mode(uint8_t mode)
{
    if( (mode!= WORK_MODE_NORMAL) && (mode != WORK_MODE_STROBE) &&
        (mode != WORK_MODE_PROG))
         return  LIGHT_ERROR;

	g_light_system.work_mode = mode;
    return  LIGHT_OK;
}

uint8_t get_light_normal_mode_state(void)
{
    return g_light_system.normal_mode_state;
}

light_status_t set_light_normal_mode_state(uint8_t state)
{
    if( (state!= H_MODE) && (state != L_MODE))
         return  LIGHT_ERROR;

    g_light_system.normal_mode_state = state;
    return  LIGHT_OK;
}
/************************ light mode API ***************************/

/************************ light state API ***************************/
light_status_t set_light_state(uint8_t channel , uint8_t state)
{
    if(channel >= CHANNEL_NUM)return LIGHT_ERROR;
    if( state > LIGHT_ON)return LIGHT_ERROR;

    g_light_system.channel[channel].state = state;
    return LIGHT_OK;
}

/**
 * @brief get light state .
 * 
 * @return 1:light on 0:light off
 */
uint8_t get_light_state(uint8_t channel)
{
    if(channel < CHANNEL_NUM){
        return g_light_system.channel[channel].state;
    }
    else{
        return 0;
    }
}
/************************ light state API ***************************/

/**
 * @brief get light trig state .
 * @return TRIG_OUT: trig out state 
 *          TRIG_IN: trig in state
 */
uint8_t get_trig_state(uint8_t channel)
{
    if( channel >= CHANNEL_NUM )
         return TRIG_OUT;

    uint8_t state = 0;

#if GD32
    state = gpio_input_bit_get(g_trig_port[channel],g_trig_pin[channel]);
#elif STM32
    state = HAL_GPIO_ReadPin(g_trig_port[channel],g_trig_pin[channel]);
#endif

    if(state == SET){
        return TRIG_OUT;
    }
    else{
        return TRIG_IN;
    }
}

/************************ System alarm API ***************************/
uint8_t get_light_alarm(void)
{
	return  g_light_system.alarm_status;
}

light_status_t set_light_alarm(uint8_t status)
{
    if( status >= STATUS_ERROR_MAX )
        return  LIGHT_ERROR;

	g_light_system.alarm_status = status;
    return  LIGHT_OK;
}
/************************ System alarm API ***************************/

/************************ RS232 API ***************************/
uint8_t get_232_baud_gear(void)
{
	return g_light_system.uart_baud_gear;
}

light_status_t set_232_baud_gear(uint8_t gear)
{
    if((gear < BAUD_4800) || (gear > BAUD_115200))
        return  LIGHT_ERROR;

	g_light_system.uart_baud_gear = gear;
	return  LIGHT_OK;
}
/************************ RS232 API ***************************/

/************************ LIGHT ON/OFF API ***************************/
light_status_t bsp_light_on(uint8_t channel)
{
    if(channel >= CHANNEL_NUM)
        return LIGHT_ERROR;

    if(g_light_system.channel[channel].state != LIGHT_ON)
    {
        bsp_pwm_enable(channel);
        set_light_state(channel,LIGHT_ON);
    }
    return LIGHT_OK;

}
light_status_t bsp_light_off(uint8_t channel)
{
    if(channel >= CHANNEL_NUM)
        return LIGHT_ERROR;

    if(g_light_system.channel[channel].state != LIGHT_OFF)
    {
        bsp_pwm_disable(channel);
        set_light_state(channel,LIGHT_OFF);
    }
    return LIGHT_OK;
}

void bsp_light_all_off(void)
{
    for(uint8_t i = 0; i < CHANNEL_NUM; i++)
    {
        bsp_all_pwm_disable();    //set pwm value to 0
        set_light_state(i,LIGHT_OFF);
		g_light_system.channel_last[i].brightness = 0;
    }
}
/************************ LIGHT ON/OFF API ***************************/


void light_brightness_handle(void)
{
    for(uint8_t i = 0; i < CHANNEL_NUM; i++)
    {
        if(g_light_system.channel_last[i].brightness != g_light_system.channel[i].brightness)
        {
            g_light_system.channel_last[i].brightness = g_light_system.channel[i].brightness;

            bsp_dps_light_value_config(i,g_light_system.brightness_range,g_light_system.channel[i].brightness);   
        }
    }
}


/**
 * @brief handle light in normal mode .
 * @note  if channel[x].enabled is false, then light off.
 *        in H mode ,light on when trig out; and light off when light value is 0 or trig in.
 *        in L mode ,light on when trig in and light value is not 0; and light off when trig out.
 */
void normal_mode_light_ctrl_handle(uint8_t channel)
{
    if( get_light_mode() != WORK_MODE_NORMAL )return;

    if (channel >= CHANNEL_NUM)
        channel = CHANNEL_NUM - 1 ;

    if (!g_light_system.channel[channel].enabled) {
        bsp_light_off(channel);
        return;
    }

    if(g_light_system.normal_mode_state == H_MODE){
        if( ( get_light_value(channel) == 0 ) || ( get_trig_state(channel) == TRIG_IN ) ) {
            bsp_light_off(channel);
        }
        else {
            bsp_light_on(channel);
        }
    }
    else if(g_light_system.normal_mode_state == L_MODE){
        if( ( get_trig_state(channel) == TRIG_IN ) && ( get_light_value(channel) != 0 ) ) {
            bsp_light_on(channel);
        }
        else{
            bsp_light_off(channel);
        }
    }
}




void normal_mode_light_handle(void)
{
    if( get_light_mode() != WORK_MODE_NORMAL )return;

    for(uint8_t i = 0; i < CHANNEL_NUM; i++)
    {
        normal_mode_light_ctrl_handle(i);
    }
}

void light_mode_handle(void)
{
    static uint8_t last_mode = 0;
    uint8_t mode = get_light_mode();

    if(mode != last_mode)
    {
        if(mode == WORK_MODE_STROBE || mode == WORK_MODE_PROG)
        {
            bsp_light_all_off();  
        }
        last_mode = mode;
    }
}


/**
 *  Global handle
 * @brief light on off handle .
 * @note   
 *     in H mode ,light on when trig out; and light off when light value is 0 or trig in.
 *     in L mode ,light on when trig in and light value is not 0; and light off when trig out.
 */
void light_handle(void)
{
    light_mode_handle();
    normal_mode_light_handle();
    soft_trig_mode_handle();
}


//Global Trigger Callback

/**
 * @brief Trigger callback function for light control and strobe.
 *
 * This function is called when a trigger event occurs for a specific channel.
 * It handles the light control in normal mode and also triggers the strobe callback
 * for the specified channel.
 *
 * @param channel The channel number on which the trigger event occurred.
 */
void trig_callback(uint8_t channel)
{
    normal_mode_light_ctrl_handle(channel);
    strobe_trig_callback(channel);
}


