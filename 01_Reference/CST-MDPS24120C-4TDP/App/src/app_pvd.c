/******************************************************************************
 * Copyright (C) 2025 CST, Inc.(Gmbh) or its affiliates.
 *
 * All Rights Reserved.
 *
 * @file bsp_pvd.c
 *
 * @par dependencies
 * - bsp_pvd.h
 *
 * @author Alan | R&D Dept. | CST
 *
 * @brief Provide the HAL APIs of the power down detect.
 *
 * Processing flow:
 * call directly.
 *
 * @version 		V1.0 		2025-05-20 * 
 * @note saving data to flash
 *       1 tab == 4 spaces!
 *
 *****************************************************************************/

#include "app_pvd.h"
#include "app.h"

void pvd_init(void)
{
	nvic_irq_enable(LVD_IRQn,0,0);
	
    rcu_periph_clock_enable(RCU_PMU);

    exti_init(EXTI_16, EXTI_INTERRUPT, EXTI_TRIG_BOTH);

    pmu_lvd_select(PMU_LVDT_7);
}



/*!
    \brief      this function handles LVD exception
    \param[in]  none
    \param[out] none
    \retval     none
*/
void LVD_IRQHandler(void)
{
    if(RESET != exti_interrupt_flag_get(EXTI_16))
    {
        bsp_all_pwm_disable();
        bsp_SaveFlashData();
        exti_interrupt_flag_clear(EXTI_16);
    }
}




