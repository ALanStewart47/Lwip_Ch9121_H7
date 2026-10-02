/******************************************************************************
 * Copyright (C) 2025 CST, Inc.(Gmbh) or its affiliates.
 *
 * All Rights Reserved.
 *
 * @file bsp_adc_dma.h
 *
 * @par dependencies
 * - 
 *
 * @author Alan | R&D Dept. | CST
 *
 * @brief Provide the HAL APIs of the uart by dma
 *
 * Processing flow:
 * call directly.
 *
 * @version     V1.0        2025-05-14          ALan
 * @note    
 *       1 tab == 4 spaces!
 *
 *****************************************************************************/

 #ifndef __BSP_ADC_DMA_H__  //Avoid repeated including same files later
 #define __BSP_ADC_DMA_H__
 
 //******************************** Includes *********************************//

 #include "main.h"
 #include <stdint.h>              
 #include <stdio.h>
 
//******************************** Includes *********************************//

//******************************** Defines **********************************//
/*  º¯Êý·µ»Ø×´Ì¬Ã¶¾Ù                    */
typedef enum
{
  ADC_OK                = 0,           /* Operation completed successfully.  */
  ADC_ERROR             = 1,           /* Run-time error without case matched*/
  ADC_ERRORTIMEOUT      = 2,           /* Operation failed with timeout      */
  ADC_ERRORRESOURCE     = 3,           /* Resource not available.            */
  ADC_ERRORPARAMETER    = 4,           /* Parameter error.                   */
  ADC_ERRORNOMEMORY     = 5,           /* Out of memory.                     */
  ADC_ERRORISR          = 6,           /* Not allowed in ISR context         */
  ADC_RESERVED          = 0x7FFFFFFF   /* Reserved                           */
}adc_status_t;


//******************************** Defines **********************************//

//******************************** Declaring ********************************//
adc_status_t GD_ADC_Start_DMA(uint32_t adc_periph,uint32_t adc_value,uint32_t DataLength);
//******************************** Declaring ********************************//

 #endif /* __ADC_DMA_H__ */ 

