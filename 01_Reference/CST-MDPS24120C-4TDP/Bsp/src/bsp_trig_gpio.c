/******************************************************************************
 * Copyright (C) 2025 CST, Inc.(Gmbh) or its affiliates.
 *
 * All Rights Reserved.
 *
 * @file bsp_trig_gpio.c
 *
 * @par dependencies
 * - bsp_trig_gpio.h
 *
 * @author Alan | R&D Dept. | CST
 *
 * @brief Provide the HAL APIs of the trig
 *
 * Processing flow:
 * call directly.
 *
 * @version    V1.0     2025-05-09      ALan 
 *             V1.1     2025-08-14      ALan
 * @note    
 *              1 tab == 4 spaces!
 *
 *****************************************************************************/

//******************************** Includes *********************************//
#include "bsp_trig_gpio.h"
#include "app_prog_trig.h"
//******************************** Includes *********************************//


//******************************** Defines ********************************//

#if STM32
/*rcu_periph_enum user_trig_clks[USER_TRIG_NUM] = {RCU_GPIOA,RCU_GPIOA,RCU_GPIOB,RCU_GPIOB,
                                            RCU_GPIOB,RCU_GPIOB,RCU_GPIOB,RCU_GPIOB};*/

uint16_t user_trig_gpio_pin[USER_TRIG_NUM] = {TRIG_IN_1_Pin,TRIG_IN_2_Pin,TRIG_IN_3_Pin,TRIG_IN_4_Pin};
                            
GPIO_TypeDef * user_trig_gpio_port[USER_TRIG_NUM] = {TRIG_IN_1_GPIO_Port,TRIG_IN_2_GPIO_Port,
                                                    TRIG_IN_3_GPIO_Port,TRIG_IN_4_GPIO_Port};

IRQn_Type user_trig_irq[USER_TRIG_NUM] = {TRIG_IN_1_EXTI_IRQn,TRIG_IN_2_EXTI_IRQn,
                                            TRIG_IN_3_EXTI_IRQn,TRIG_IN_4_EXTI_IRQn};

#endif

//Array to store trigger counts for each user trigger input
uint32_t trig_count[USER_TRIG_NUM]      = {0};
uint32_t light_out_count[USER_TRIG_NUM] = {0};
uint32_t cam_count[USER_TRIG_NUM]       = {0};


//Array of trigger callback functions for each user trigger input.
static trig_callback_t trig_callbacks[USER_TRIG_NUM] = {0};

#define CAM_OUT_1_OPEN()  HAL_GPIO_WritePin(CAM_OUT_1_GPIO_Port, CAM_OUT_1_Pin, GPIO_PIN_SET)
#define CAM_OUT_1_CLOSE() HAL_GPIO_WritePin(CAM_OUT_1_GPIO_Port, CAM_OUT_1_Pin, GPIO_PIN_RESET)

#define CAM_OUT_2_OPEN()  HAL_GPIO_WritePin(CAM_OUT_2_GPIO_Port, CAM_OUT_2_Pin, GPIO_PIN_SET)
#define CAM_OUT_2_CLOSE() HAL_GPIO_WritePin(CAM_OUT_2_GPIO_Port, CAM_OUT_2_Pin, GPIO_PIN_RESET)

#define CAM_OUT_3_OPEN()  HAL_GPIO_WritePin(CAM_OUT_3_GPIO_Port, CAM_OUT_3_Pin, GPIO_PIN_SET)
#define CAM_OUT_3_CLOSE() HAL_GPIO_WritePin(CAM_OUT_3_GPIO_Port, CAM_OUT_3_Pin, GPIO_PIN_RESET)

#define CAM_OUT_4_OPEN()  HAL_GPIO_WritePin(CAM_OUT_4_GPIO_Port, CAM_OUT_4_Pin, GPIO_PIN_SET)
#define CAM_OUT_4_CLOSE() HAL_GPIO_WritePin(CAM_OUT_4_GPIO_Port, CAM_OUT_4_Pin, GPIO_PIN_RESET)


//******************************** Defines ********************************//
 /**
 * @brief	trig gpio hardware init 
 * @note	
 * @param	trig_num :TRIG_1 ~ TRIG_8  
 * @retval	void
 */
trig_status_t bsp_trig_gpio_init(uint8_t trig_num)
{
#if GD32
    if(trig_num > TRIG_8)
        return TRIG_ERROR;

    rcu_periph_clock_enable(user_trig_clks[trig_num]);
    rcu_periph_clock_enable(RCU_AF);

    gpio_init(  user_trig_gpio_port[trig_num], 
                GPIO_MODE_IN_FLOATING, 
                GPIO_OSPEED_50MHZ, 
                user_trig_gpio_pin[trig_num]);
#endif
    return TRIG_OK;
}


 /**
 * @brief	trig gpio exti init
 * @note	deafult priority is 0-0 , highest priority 
 * @param	trig_num :TRIG_1 ~ TRIG_8  ;  
 *          exti_mode : 0-RISING_FALLING_MODE
 *                      1-RISING_MODE
 *                      2-FALLING_MODE
 * @retval	void
 */
trig_status_t bsp_trig_exti_init(uint8_t trig_num, exti_mode_t exti_mode)
{
#if GD32
    exti_trig_type_enum exti_trig = EXTI_TRIG_RISING;

    if(TRIG_8 < trig_num)
        return TRIG_ERROR;

    if(FALLING_MODE < exti_mode)
        return TRIG_ERROR;

    if(RISING_FALLING_MODE == exti_mode)
        exti_trig = EXTI_TRIG_BOTH;
    
    if( RISING_MODE == exti_mode )
        exti_trig = EXTI_TRIG_RISING;

    if( FALLING_MODE == exti_mode )
        exti_trig = EXTI_TRIG_FALLING;

    nvic_irq_enable(user_trig_irq[trig_num], 1U, 0U);

    gpio_exti_source_select(trig_port_source[trig_num], trig_pin_source[trig_num]);

    exti_init(user_trig_exti_line[trig_num], EXTI_INTERRUPT, exti_trig);

    exti_interrupt_flag_clear(user_trig_exti_line[trig_num]);
#endif
    return TRIG_OK;
}

 /**
 * @brief	trig overall init
 * @note	the default exti trig mode is falling edge 
 * @param	
 * @retval	void
 */
trig_status_t bsp_trig_init(exti_mode_t exti_mode )
{
#if GD32
    //exti_mode_t s_exti_mode = FALLING_MODE; 
    for(uint8_t i = 0; i< USER_TRIG_NUM ; i++)
    {
        bsp_trig_exti_init(i,exti_mode);
        bsp_trig_gpio_init(i);
    }
#endif
    return TRIG_OK;
}



trig_status_t bsp_trig_init_in_falling(void )
{
#if GD32
    //exti_mode_t s_exti_mode = FALLING_MODE; 
    for(uint8_t i = 0; i< USER_TRIG_NUM ; i++)
    {
        bsp_trig_exti_init(i,FALLING_MODE);
        bsp_trig_gpio_init(i);
    }
#endif
    trig_interrupt_enable();
    return TRIG_OK;
}



 /**
 * @brief	trig mode config
 * @note	the default exti trig mode is falling edge 
 * @param	
 * @retval	void
 */
trig_status_t bsp_trig_mode_config(trig_num_t trig_num, exti_mode_t exti_mode)
{
    if( trig_num > TRIG_8 )
       return TRIG_ERROR; 

    if( exti_mode > FALLING_MODE )
       return TRIG_ERROR;   

    bsp_trig_exti_init(trig_num,exti_mode);
    return TRIG_OK;
}


 /**
 * @brief	register trig callback function
 * @note	
 * @param	trig_num : TRIG_1 ~ TRIG_8
 *          callback: trig callback function
 * @retval	void
 */
trig_status_t bsp_trig_register_callback(uint8_t trig_num, trig_callback_t callback)
{
    if (trig_num >= USER_TRIG_NUM || callback == NULL) {
        return TRIG_ERROR;
    }

    trig_callbacks[trig_num] = callback;
    return TRIG_OK;
}

/**
 * @brief Enables trigger interrupts for all channels.
 *
 * Registers the light control handler callback for each channel's trigger interrupt.
 * The number of channels is determined by CHANNEL_NUM macro.
 */
void trig_interrupt_enable(void)
{
    for (uint8_t i = 0; i < CHANNEL_NUM; i++)
    {
        bsp_trig_register_callback(i, trig_callback);  // from app_light.c 
    }
}

 /**
 * @brief	trig callback function
 * @note	
 * @param	num : TRIG_1 ~ TRIG_8
 * @retval	void
 */
trig_status_t bsp_trig_callback(trig_num_t num)
{
    if(num >= USER_TRIG_NUM)
        return TRIG_ERROR;

    if(trig_callbacks[num])
        trig_callbacks[num](num);  

    trig_count[num]++;
    if(trig_count[num] > 0xFFFFFFFE)
        trig_count[num] = 0;
    
    return TRIG_OK;
}


 /**
 * @brief	grt trig count than into interrupt
 * @note	
 * @param	num : TRIG_1 ~ TRIG_8
 * @retval	void
 */
uint32_t bsp_trig_get_count(uint8_t num)
{
    if(num > TRIG_8)
        num = TRIG_8;

    return trig_count[num];
}

void bsp_trig_add_count(uint8_t  num)
{
    if(num > TRIG_8)
        num = TRIG_8;

    trig_count[num]++;
}

void bsp_trig_clear_all(void)
{
    for (uint8_t i = 0; i < USER_TRIG_NUM; i++){
        trig_count[i] = 0;
    }
}

uint32_t bsp_cam_get_count(uint8_t num)
{
    if(num > TRIG_8)
        num = TRIG_8;

    return cam_count[num];
}

void bsp_cam_add_count(uint8_t num)
{
    if(num > TRIG_8)
        num = TRIG_8;

    cam_count[num]++;
}

void bsp_cam_clear_all(void)
{
    for (uint8_t i = 0; i < USER_TRIG_NUM; i++){
        cam_count[i] = 0;
    }
}

/**
 *  @brief light ouput 
 */
uint32_t bsp_light_output_get_count(uint8_t num)
{
    if (num > TRIG_8)
        num = TRIG_8;

    return light_out_count[num];
}

void bsp_light_out_add_count(uint8_t num)
{
    if (num > TRIG_8)
        num = TRIG_8;
    
    light_out_count[num]++;   
}

void bsp_light_output_clear_all(void)
{
    for (uint8_t i = 0; i < USER_TRIG_NUM; i++){
        light_out_count[i] = 0;
    }
}


trig_status_t bsp_cam_out_init(void)
{
#if GD32
    rcu_periph_clock_enable(RCU_GPIOB);
    rcu_periph_clock_enable(RCU_AF);

    gpio_init(  CAM_OUT_GPIO_PORT, 
                GPIO_MODE_OUT_PP, 
                GPIO_OSPEED_2MHZ, 
                CAM_OUT_GPIO_PIN);

    gpio_bit_reset(CAM_OUT_GPIO_PORT, CAM_OUT_GPIO_PIN);
    return TRIG_OK;
#elif STM32
    CAM_OUT_1_CLOSE();
    CAM_OUT_2_CLOSE();   
    CAM_OUT_3_CLOSE();
    CAM_OUT_4_CLOSE();
    return TRIG_OK;
#endif
    
}

trig_status_t bsp_cam_output_enable(void)
{
#if GD32
    gpio_bit_set(CAM_OUT_GPIO_PORT, CAM_OUT_GPIO_PIN);
#elif STM32
    CAM_OUT_1_OPEN();
    bsp_cam_add_count(TRIG_1);
#endif
    return TRIG_OK;
}

trig_status_t bsp_cam_output_disable(void)
{
#if GD32
    gpio_bit_reset(CAM_OUT_GPIO_PORT, CAM_OUT_GPIO_PIN);
#elif STM32
    CAM_OUT_1_CLOSE();
#endif
    return TRIG_OK;
}

trig_status_t bsp_cam_output_open_in_strobe(uint8_t channel)
{
    //if(channel < TRIG_1 || channel > TRIG_8)
	 if (channel > TRIG_8)
        return TRIG_ERROR;

    switch(channel)
    {
        case TRIG_1:
            CAM_OUT_1_OPEN();
            bsp_cam_add_count(TRIG_1);
            break;
        case TRIG_2:
            CAM_OUT_2_OPEN();
            bsp_cam_add_count(TRIG_2);
            break;
        case TRIG_3:
            CAM_OUT_3_OPEN();
            bsp_cam_add_count(TRIG_3);
            break;
        case TRIG_4:
            CAM_OUT_4_OPEN();
            bsp_cam_add_count(TRIG_4);
            break;
        default:
            break;
    }
    return TRIG_OK;
}

trig_status_t bsp_cam_output_close_in_strobe(uint8_t channel)
{
    //if(channel < TRIG_1 || channel > TRIG_8)
	if (channel > TRIG_8)
        return TRIG_ERROR;

    switch(channel)
    {
        case TRIG_1:
            CAM_OUT_1_CLOSE();
            break;
        case TRIG_2:
            CAM_OUT_2_CLOSE();
            break;
        case TRIG_3:
            CAM_OUT_3_CLOSE();
            break;
        case TRIG_4:
            CAM_OUT_4_CLOSE();
            break;
        default:
            break;
    }
    return TRIG_OK;
}




