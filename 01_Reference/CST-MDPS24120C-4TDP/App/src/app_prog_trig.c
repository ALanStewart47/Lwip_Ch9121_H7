/******************************************************************************
 * Copyright (C) 2025 CST, Inc.(Gmbh) or its affiliates.
 *
 * All Rights Reserved.
 *
 * @file bsp_prog_trig.c
 *
 * @par dependencies
 * - bsp_prog_trig.h
 *
 * @author Alan | R&D Dept. | CST
 *
 * @brief Provide the HAL APIs of the program trig.
 *
 * Processing flow:
 * call directly.
 *
 * @version    V1.0 	2025-07-15   
 *             V1.1     2025-07-31  
 *             V1.2     2025-10-30  add master trig slave code
 *             V1.3     2025-12-15  add sync mode     
 *                      
 * @note saving data to flash
 *       1 tab == 4 spaces!
 *
 *****************************************************************************/

//******************************** Includes *********************************//
#include "app_prog_trig.h"
#include "app.h"
#include "app_flash.h"
#include "bsp_trig_gpio.h"
//******************************** Includes *********************************//

//******************************** Defines **********************************//
extern system_param_t g_light_system;


static void master_trig_slave_cb(void);
//******************************** Defines **********************************//

//******************************** Declaring ********************************//
void Prog_test_init(void)
{
    bsp_prog_trig_init();
}

void bsp_prog_trig_task_init(void)
{
    bsp_prog_trig_init();
}


prog_status_t bsp_prog_trig_init(void)
{
    g_light_system.programmable_unit        = PROG_UNIT_MS;
    g_light_system.programmable_mode        = PROG_MODE_SINGLE_STEP;
    g_light_system.programmable_step        = 0;
    g_light_system.programmable_interval    = 0;               
    g_light_system.prog_total_steps         = 0;                       
    g_light_system.prog_start_step          = 0;                       //Reserved
    g_light_system.prog_end_step            = 0;                       //Reserved
    g_light_system.prog_enable              = 0;   
    g_light_system.prog_busy                = 0;
    g_light_system.prog_continuous_running  = CONTINUOUS_IDLE;
    
    memset(g_light_system.prog_table, 0, sizeof(g_light_system.prog_table));
    
    for(uint8_t i = 0; i < PROG_TABLE_NUM_MAX; i++) {
        g_light_system.prog_table[i].trigger_src = TRIG_1;
    }
    return PROG_OK;
}

/*
prog_status_t bsp_prog_trig_enable( bool enable)
{
    //g_light_system.prog_enable = enable ? 1 : 0;
    return PROG_OK;
}*/


prog_status_t bsp_prog_trig_reset(void)
{
    g_light_system.programmable_step = 0;
    return PROG_OK;
}

uint8_t bsp_prog_trig_get_totsl_steps(void)
{
    return g_light_system.prog_total_steps; 
}

prog_status_t bsp_prog_trig_set_total_steps(uint8_t total_steps)
{
    if ( total_steps > PROG_TABLE_NUM_MAX)
        return PROG_ERRORPARAMETER;
    g_light_system.prog_total_steps = total_steps;
    return PROG_OK;
}

prog_status_t bsp_prog_trig_set_current_step(uint8_t step)
{
    if (step > g_light_system.prog_total_steps)
        return PROG_ERRORPARAMETER;

    g_light_system.programmable_step = step - 1;
    return PROG_OK;
}

uint8_t bsp_prog_trig_get_current_step(void)
{
    return g_light_system.programmable_step; // Return step as 1-based index
}


prog_status_t bsp_prog_trig_set_table(uint8_t idx, const prog_table_row_t* table)
{
    if(idx >= PROG_TABLE_NUM_MAX || !table)
        return PROG_ERRORPARAMETER;

    if(idx >= g_light_system.prog_total_steps)
        return PROG_ERRORPARAMETER;

    g_light_system.prog_table[idx] = *table;

    return PROG_OK;
}

prog_status_t bsp_prog_trig_insert_table(uint8_t idx, const prog_table_row_t* table)
{
    if(idx >= PROG_TABLE_NUM_MAX || !table)
        return PROG_ERRORPARAMETER;

    //1.1
    if (idx >= g_light_system.prog_total_steps)
        return PROG_ERRORPARAMETER;

    memmove(&g_light_system.prog_table[idx+1], &g_light_system.prog_table[idx], 
            (g_light_system.prog_total_steps - idx) * sizeof(prog_table_row_t));

    g_light_system.prog_table[idx] = *table;
    g_light_system.prog_total_steps++;

    //1.1
    if (g_light_system.prog_total_steps < PROG_TABLE_NUM_MAX)
        g_light_system.prog_total_steps++;

    return PROG_OK;
}

prog_status_t bsp_prog_trig_delete_table( uint8_t idx)
{
    if(idx >= g_light_system.prog_total_steps)
        return PROG_ERRORPARAMETER;

    memmove(&g_light_system.prog_table[idx], &g_light_system.prog_table[idx+1], 
            (g_light_system.prog_total_steps - idx - 1) * sizeof(prog_table_row_t));
    g_light_system.prog_total_steps--;

    if (g_light_system.programmable_step >= g_light_system.prog_total_steps)
        g_light_system.programmable_step = 0;

    return PROG_OK;
}

prog_status_t bsp_prog_trig_set_start_step(uint8_t start)
{
    if(start >= g_light_system.prog_total_steps)
        return PROG_ERRORPARAMETER;

    g_light_system.prog_start_step = start;
    
    if (g_light_system.programmable_step < start || 
        g_light_system.programmable_step > g_light_system.prog_end_step)
        g_light_system.programmable_step = start;

    return PROG_OK;
}

prog_status_t bsp_prog_trig_set_end_step(uint8_t end)
{
    if(end >= g_light_system.prog_total_steps || end < g_light_system.prog_start_step)
        return PROG_ERRORPARAMETER;

    g_light_system.prog_end_step = end;
    
    if (g_light_system.programmable_step < g_light_system.prog_start_step || 
        g_light_system.programmable_step > end)
        g_light_system.programmable_step = g_light_system.prog_start_step;

    return PROG_OK;
}

prog_status_t bsp_prog_trig_set_interval(uint16_t interval)
{
    if (interval > PROG_INTERVAL_MAX)
        return PROG_ERRORPARAMETER;

    g_light_system.programmable_interval = interval;
    return PROG_OK;
}

uint16_t bsp_prog_trig_get_interval(void)
{
    return g_light_system.programmable_interval;
}

prog_status_t bsp_prog_trig_set_unit(uint8_t unit)
{
    if (unit != PROG_UNIT_MS && unit != PROG_UNIT_100US)
        return PROG_ERRORPARAMETER;

    g_light_system.programmable_unit = unit;
    return PROG_OK;
}

uint8_t bsp_prog_trig_get_unit(void)
{
    return g_light_system.programmable_unit;
}

uint8_t bsp_prog_trig_get_mode(void)
{
    return g_light_system.programmable_mode;
}

prog_status_t bsp_prog_trig_set_mode(uint8_t mode)
{
    if (mode != PROG_MODE_SINGLE_STEP && mode != PROG_MODE_CONTINUOUS)
        return PROG_ERRORPARAMETER;

    g_light_system.programmable_mode = mode;
    
    if (mode == PROG_MODE_SINGLE_STEP) {
        g_light_system.prog_continuous_running = CONTINUOUS_IDLE;
    }
    return PROG_OK;
}

void bsp_save_prog_table_data(void)
{
    bsp_prog_trig_save_to_flash();
}

/**
 * @brief   Master trigger slave callback function.
 * @note    master trig in slave sync mode
 * @param   None
 */
static void master_trig_slave(void)
{
    if (get_programmable_sync_mode() != SYNC_MODE_MASTER_SYNC)
        return;

    bsp_tim4_start(1,10,master_trig_slave_cb);
    HAL_GPIO_WritePin(MASTER_TRIG_GPIO_Port,MASTER_TRIG_Pin,GPIO_PIN_SET);
}



/**
 * @brief   Executes the programmable trigger based on the current step and trigger source.
 * @note        
 * @param   [in] trigger_src: The source of the trigger (e.g., TRIG_1).
 * @retval  PROG_OK on success, or an error code if the operation fails.
 */
prog_status_t bsp_prog_trig_exec (uint8_t trigger_src)
{
    if (g_light_system.work_mode != WORK_MODE_PROG)                   return PROG_ERROR;
    if (g_light_system.prog_total_steps == 0)                         return PROG_ERROR;
    if (g_light_system.prog_busy == PROG_STATE_WORK)                  return PROG_ERROR;
    if (g_light_system.prog_continuous_running == CONTINUOUS_RUNNING) return PROG_ERROR;

    master_trig_slave();
	bsp_trig_add_count(TRIG_1);

    uint32_t actual_pulse_width;
    prog_table_row_t* step = &g_light_system.prog_table[g_light_system.programmable_step];

    if (step->trigger_src == trigger_src) 
    {
        if (g_light_system.programmable_mode == PROG_MODE_CONTINUOUS) {
            g_light_system.prog_continuous_running = CONTINUOUS_RUNNING;
        }
        if(step->prog_strobe_width > 999)
            step->prog_strobe_width = 999;  

        g_light_system.prog_busy                   = PROG_STATE_WORK;
        bsp_light_all_off();

        for (uint8_t i = 0; i < CHANNEL_NUM; i++) {
            bsp_dps_light_value_config (i, g_light_system.brightness_range, step->brightness[i]);
            if ((step->brightness[i] > 0) && (step->prog_strobe_width != 0)) {
                bsp_light_on           (i);
                bsp_light_out_add_count(i);
            }
        }      

        if (g_light_system.programmable_unit == PROG_UNIT_MS) 
            actual_pulse_width = step->prog_strobe_width * 10000;    // 1ms, time base is 100ns = 0.1us , so multiply 10000
        else // 100us
            actual_pulse_width = step->prog_strobe_width * 1000;     // 100us, time base is 100ns = 0.1us , so multiply 1000

        //open programmable trigger timer
        if ( 0 == actual_pulse_width) {
            prog_trig_pulse_width_callback();
        }
        else {
            bsp_tim2_start_cc(PROG_TIMER_CH1,(uint32_t)actual_pulse_width,prog_trig_pulse_width_callback);
            bsp_output_camera_signal();
        }
        return PROG_OK;
    }
    return PROG_ERROR;
}


prog_status_t bsp_prog_trig_start_pulse(const uint16_t pulse_width)
{
    uint32_t actual_pulse_width;
    if (g_light_system.programmable_unit == PROG_UNIT_MS) 
        actual_pulse_width = pulse_width * 10000;    // 1ms, time base is 100ns = 0.1us , so multiply 10000
    else 
        actual_pulse_width = pulse_width * 1000;     // 100us, time base is 100ns = 0.1us , so multiply 1000

    prog_table_row_t* step = &g_light_system.prog_table[g_light_system.programmable_step];
      
    for (uint8_t i = 0; i < CHANNEL_NUM; i++) {
        if (step->brightness[i] > 0) {
            bsp_light_on(i);
        }
    }
    //open programmable trigger timer
    bsp_tim2_start_cc(PROG_TIMER_CH1,actual_pulse_width,prog_trig_pulse_width_callback);
    return PROG_OK;
}

/**
 * @brief   Handles the programmable trigger pulse width callback.
 * @note    This function is called by the timer callback.
 * @param   None
 * @return  None
 */
void prog_trig_pulse_width_callback(void)
{
    uint32_t actual_programmable_interval;
    bsp_light_all_off();
    //bsp_output_camera_signal();
	g_light_system.prog_busy = PROG_STATE_IDLE;

    g_light_system.programmable_step++;
    if (g_light_system.programmable_step >= g_light_system.prog_total_steps)
    {
        g_light_system.programmable_step = 0;
        if (g_light_system.programmable_mode == PROG_MODE_CONTINUOUS) {
            g_light_system.prog_continuous_running = CONTINUOUS_IDLE;
        }
        return;
    }
    
    actual_programmable_interval = g_light_system.programmable_interval * 10000; //time base is 100ns = 0.1us, so multiply 10000
    //continue to next step if in continuous mode
    if (g_light_system.programmable_mode == PROG_MODE_CONTINUOUS) 
    {   
        if (actual_programmable_interval > 0) {
            //open programmable trigger interval timer
            bsp_tim2_start_cc(   PROG_TIMER_CH2, 
                                    actual_programmable_interval, 
                                    prog_trig_continuous_next_step);
        } 
        else{
            prog_trig_continuous_next_step();
        } 
    }
   
}

static void master_trig_slave_cb(void)
{
    HAL_GPIO_WritePin(MASTER_TRIG_GPIO_Port,MASTER_TRIG_Pin,GPIO_PIN_RESET);
    //LOG_I("complate trig slave");
}

//0.2.1
/**
 * @brief   Executes the next step in continuous programmable mode.
 * @note    This function is called after the programmable pulse width callback.
 * @param   None
 * @return  None
 */
void prog_trig_continuous_next_step(void)
{
    if (g_light_system.work_mode != WORK_MODE_PROG || 
        g_light_system.programmable_mode != PROG_MODE_CONTINUOUS ||
        g_light_system.prog_busy == PROG_STATE_WORK) {
        return;
    }
    prog_table_row_t* step = &g_light_system.prog_table[g_light_system.programmable_step];
    
    g_light_system.prog_busy = PROG_STATE_WORK;

    if(step->prog_strobe_width > 999) step->prog_strobe_width = 999;
    
    for (uint8_t i = 0; i < CHANNEL_NUM; i++) {
        bsp_dps_light_value_config(i, g_light_system.brightness_range, step->brightness[i]);
        if ((step->brightness[i] > 0) && (step->prog_strobe_width != 0)) {
            bsp_light_on           (i);
            bsp_light_out_add_count(i);
        }
    }

    uint32_t actual_pulse_width;
    //if(step->prog_strobe_width > 999) step->prog_strobe_width = 999;
    //if(step->prog_strobe_width == 0) step->prog_strobe_width = 1;
    
    if (g_light_system.programmable_unit == PROG_UNIT_MS) 
        actual_pulse_width = step->prog_strobe_width * 10000;    // 1ms, time base is 100ns = 0.1us , so multiply 10000
    else 
        actual_pulse_width = step->prog_strobe_width * 1000;     // 100us, time base is 100ns = 0.1us , so multiply 1000

    bsp_tim2_start_cc(PROG_TIMER_CH1, actual_pulse_width, prog_trig_pulse_width_callback);
    bsp_output_camera_signal();
}



/**
 * @brief   Soft trigger for programmable mode.
 * @note    This function is called to trigger the programmable mode manually.
 * @param   None
 * @return  PROG_OK on success, or an error code if the operation fails.
 */
prog_status_t bsp_prog_trig_soft_trigger(void)
{
    return bsp_prog_trig_exec(TRIG_1); 
}


void bsp_output_camera_callback(void)
{
    bsp_cam_output_disable();
}


void bsp_output_camera_signal(void)
{
    if (g_light_system.programmable_unit == PROG_UNIT_MS) 
    {
        // 1ms 
        bsp_cam_output_enable();
        bsp_tim2_start_cc(PROG_TIMER_CH3, 10000, bsp_output_camera_callback);
    } 
    else 
    {
        // 100us ,time base is 100ns = 0.1us , so 100us = 1000*0.1us
        bsp_cam_output_enable();
        bsp_tim2_start_cc(PROG_TIMER_CH3, 1000, bsp_output_camera_callback);
    }
}


void prog_trig_callback(uint8_t channel)
{
    uint8_t trigger_src = TRIG_1;

    if (g_light_system.prog_continuous_running == CONTINUOUS_RUNNING) 
        return;

    if (channel == CH1) 
        bsp_prog_trig_exec(trigger_src);

}


void prog_trig_handle(void)
{
    if( g_light_system.work_mode != WORK_MODE_PROG )return;
    if(g_light_system.prog_busy != PROG_STATE_WORK) return;
    if(g_light_system.prog_enable == 0) return;
	
	uint32_t actual_pulse_width;
    prog_table_row_t* step = &g_light_system.prog_table[g_light_system.programmable_step];

	
        for (uint8_t i = 0; i < CHANNEL_NUM; i++) {
            bsp_dps_light_value_config(i, g_light_system.brightness_range, step->brightness[i]);
        }
        
        // start the programmable strobe pulse
        if(step->prog_strobe_width > 999)
            step->prog_strobe_width = 999;
        if(step->prog_strobe_width == 0)
            step->prog_strobe_width = 1;   

        if (g_light_system.programmable_unit == PROG_UNIT_MS) 
            actual_pulse_width = step->prog_strobe_width * 10000;       //time base is 100ns = 0.1us
        else 
            actual_pulse_width = step->prog_strobe_width * 1000;       // 100us

        for (uint8_t i = 0; i < CHANNEL_NUM; i++) {
            if (step->brightness[i] > 0) {
                bsp_light_on(i);
            }
        }
        //open programmable trigger timer
        //bsp_InitTrigDelayTimer(TIMER1, TIMER1_IRQn, TRIG_DELAY_100US);
		
        bsp_tim2_start_cc(PROG_TIMER_CH1,actual_pulse_width,prog_trig_pulse_width_callback);
		g_light_system.prog_enable =0;
}


//******************************** Declaring ********************************//

