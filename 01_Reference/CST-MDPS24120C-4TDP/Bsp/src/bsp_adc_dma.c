/******************************************************************************
 * Copyright (C) 2025 CST, Inc.(Gmbh) or its affiliates.
 *
 * All Rights Reserved.
 *
 * @file bsp_adc_dma.c
 *
 * @par dependencies
 * - bsp_adc_dma.h
 *
 * @author Alan | R&D Dept. | CST
 *
 * @brief Provide the HAL APIs of the adc by dma
 *
 * Processing flow:
 * call directly.
 *
 * @version    V1.0     2025-05-14      ALan 
 * @note    
 *              1 tab == 4 spaces!
 *
 *****************************************************************************/
#include "bsp_adc_dma.h"
#include "systick.h"

uint16_t adc_value[1];

/**
 * @brief Initialize ADC0 with DMA for continuous sampling
 *
 * This function configures ADC0 with the following settings:
 * - Continuous mode enabled
 * - Scan mode enabled
 * - Right data alignment
 * - Single channel (PA0) sampling
 * - DMA transfer enabled
 * - Independent ADC mode
 *
 * @param com Unused parameter
 * @param baudval Unused parameter
 *
 * @note ADC samples from PA0 with 55.5 cycles sampling time
 */
static void adc_init(void)
{
    /* enable GPIO clock */
    rcu_periph_clock_enable(RCU_GPIOA);

    /* enable ADC0 clock */
    rcu_periph_clock_enable(RCU_ADC0);
    rcu_adc_clock_config(RCU_CKADC_CKAPB2_DIV6);        

    /* connect port to ADC0 */
    gpio_init(GPIOA, GPIO_MODE_AIN, GPIO_OSPEED_10MHZ, GPIO_PIN_0);
#if DETECT_POWER_ENABLE
    gpio_init(GPIOA, GPIO_MODE_AIN, GPIO_OSPEED_10MHZ, GPIO_PIN_1);
#endif
    /* ADC mode config: work independently */
    //adc_mode_config(ADC_MODE_FREE); 
    adc_mode_config(ADC_DAUL_REGULAL_PARALLEL_INSERTED_PARALLEL);
    /* ADC continuous function enable */
    adc_special_function_config(ADC0, ADC_CONTINUOUS_MODE, ENABLE);
    /* ADC scan function enable */
    adc_special_function_config(ADC0, ADC_SCAN_MODE, ENABLE);
    /* ADC data alignment config */
    adc_data_alignment_config(ADC0, ADC_DATAALIGN_RIGHT);

    /* ADC channel length config */
    #if DETECT_POWER_ENABLE
        adc_channel_length_config(ADC0, ADC_REGULAR_CHANNEL, 4);
    #else
	    adc_channel_length_config(ADC0, ADC_REGULAR_CHANNEL, 3);
    #endif
    //adc_channel_length_config(ADC0, ADC_REGULAR_CHANNEL, 1);

    /* ADC regular channel config */ 
    adc_regular_channel_config(ADC0, 0, ADC_CHANNEL_0, ADC_SAMPLETIME_239POINT5);
    /* ADC temperature sensor channel config */
    adc_regular_channel_config(ADC0, 1, ADC_CHANNEL_16, ADC_SAMPLETIME_239POINT5);
    /* ADC internal reference voltage channel config */
    adc_regular_channel_config(ADC0, 2, ADC_CHANNEL_17, ADC_SAMPLETIME_239POINT5);
    /* ADC regular channel config */
#if DETECT_POWER_ENABLE
    adc_regular_channel_config(ADC0, 3, ADC_CHANNEL_1, ADC_SAMPLETIME_239POINT5);
#endif  
    /* ADC temperature sensor channel config */
   //adc_external_trigger_source_config(ADC0, ADC_INSERTED_CHANNEL, ADC0_1_2_EXTTRIG_INSERTED_NONE);
    //adc_external_trigger_config(ADC0, ADC_INSERTED_CHANNEL, ENABLE);
    adc_tempsensor_vrefint_enable();

    /* ADC trigger config */
    adc_external_trigger_source_config(ADC0, ADC_REGULAR_CHANNEL, ADC0_1_2_EXTTRIG_REGULAR_NONE);
    adc_external_trigger_config(ADC0, ADC_REGULAR_CHANNEL, ENABLE);

    /* ADC DMA function enable */
    adc_dma_mode_enable(ADC0);
    /* enable ADC interface */
    adc_enable(ADC0);
    delay_1ms(1);
    /* ADC calibration and reset calibration */
    adc_calibration_enable(ADC0);

    /* ADC software trigger enable */
   adc_software_trigger_enable(ADC0, ADC_REGULAR_CHANNEL);
}

/**
 * @brief Configure DMA for ADC data transfer
 *
 * Initializes DMA channel for ADC peripheral to memory transfer in circular mode.
 * Sets up source/destination addresses, data width, transfer direction and length.
 *
 * @param adc_value Pointer to memory buffer for storing ADC values
 * @param DataLength Number of data items to transfer
 */
void dma_config(uint32_t adc_value, uint32_t DataLength)
//void dma_config(void)
{
    /* ADC_DMA_channel configuration */
    dma_parameter_struct dma_data_parameter;
    
    /* ADC DMA_channel configuration */
    dma_deinit(DMA0, DMA_CH0);

    dma_struct_para_init(&dma_data_parameter);
    
    /* initialize DMA single data mode */
    dma_data_parameter.periph_addr  = (uint32_t)(&ADC_RDATA(ADC0));
    dma_data_parameter.periph_inc   = DMA_PERIPH_INCREASE_DISABLE;
    dma_data_parameter.memory_addr  = adc_value;
    dma_data_parameter.memory_inc   = DMA_MEMORY_INCREASE_ENABLE;
    dma_data_parameter.periph_width = DMA_PERIPHERAL_WIDTH_16BIT;
    dma_data_parameter.memory_width = DMA_MEMORY_WIDTH_16BIT;  
    dma_data_parameter.direction    = DMA_PERIPHERAL_TO_MEMORY;
    dma_data_parameter.number       = DataLength;
    dma_data_parameter.priority     = DMA_PRIORITY_MEDIUM;
    dma_init(DMA0, DMA_CH0, &dma_data_parameter);

    dma_circulation_enable(DMA0, DMA_CH0);
  
    /* enable DMA channel */
    dma_channel_enable(DMA0, DMA_CH0);
}

/**
 * @brief Start ADC conversion with DMA transfer
 * @param adc_periph ADC peripheral identifier (ADC0, ADC1, etc.)
 * @param adc_value Memory address to store ADC conversion results
 * @param DataLength Number of data to be converted
 * @return ADC_OK if successful, ADC_ERROR if invalid peripheral
 */
adc_status_t GD_ADC_Start_DMA(uint32_t adc_periph, uint32_t adc_value, uint32_t DataLength)
{
    if (adc_periph == ADC0)
    {     
        dma_config(adc_value,DataLength);

        adc_init();

        return ADC_OK;
    }
    return ADC_ERROR;
}

