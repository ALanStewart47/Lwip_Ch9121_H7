/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32g4xx_hal.h"

#include "stm32g4xx_ll_spi.h"
#include "stm32g4xx_ll_tim.h"
#include "stm32g4xx_ll_bus.h"
#include "stm32g4xx_ll_cortex.h"
#include "stm32g4xx_ll_rcc.h"
#include "stm32g4xx_ll_system.h"
#include "stm32g4xx_ll_utils.h"
#include "stm32g4xx_ll_pwr.h"
#include "stm32g4xx_ll_gpio.h"
#include "stm32g4xx_ll_dma.h"

#include "stm32g4xx_ll_exti.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include "rtt_log.h"
/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define KEY_1_Pin GPIO_PIN_0
#define KEY_1_GPIO_Port GPIOC
#define KEY_2_Pin GPIO_PIN_1
#define KEY_2_GPIO_Port GPIOC
#define KEY_3_Pin GPIO_PIN_2
#define KEY_3_GPIO_Port GPIOC
#define KEY_4_Pin GPIO_PIN_3
#define KEY_4_GPIO_Port GPIOC
#define TRIG_IN_2_Pin GPIO_PIN_0
#define TRIG_IN_2_GPIO_Port GPIOA
#define TRIG_IN_2_EXTI_IRQn EXTI0_IRQn
#define CHX_POWER_Pin GPIO_PIN_1
#define CHX_POWER_GPIO_Port GPIOA
#define RS232_TX_Pin GPIO_PIN_2
#define RS232_TX_GPIO_Port GPIOA
#define RX232_RX_Pin GPIO_PIN_3
#define RX232_RX_GPIO_Port GPIOA
#define POWER_DOWN_Pin GPIO_PIN_4
#define POWER_DOWN_GPIO_Port GPIOA
#define CAM_OUT_1_Pin GPIO_PIN_5
#define CAM_OUT_1_GPIO_Port GPIOA
#define TRIG_IN_1_Pin GPIO_PIN_4
#define TRIG_IN_1_GPIO_Port GPIOC
#define TRIG_IN_1_EXTI_IRQn EXTI4_IRQn
#define MASTER_TRIG_Pin GPIO_PIN_5
#define MASTER_TRIG_GPIO_Port GPIOC
#define CAM_OUT_2_Pin GPIO_PIN_0
#define CAM_OUT_2_GPIO_Port GPIOB
#define MULTI_EN_Pin GPIO_PIN_1
#define MULTI_EN_GPIO_Port GPIOB
#define MULTI_A_Pin GPIO_PIN_2
#define MULTI_A_GPIO_Port GPIOB
#define NET_CS_Pin GPIO_PIN_12
#define NET_CS_GPIO_Port GPIOB
#define NET_SCK_Pin GPIO_PIN_13
#define NET_SCK_GPIO_Port GPIOB
#define NET_MISO_Pin GPIO_PIN_14
#define NET_MISO_GPIO_Port GPIOB
#define NET_MOSI_Pin GPIO_PIN_15
#define NET_MOSI_GPIO_Port GPIOB
#define PWM_CH_1_Pin GPIO_PIN_6
#define PWM_CH_1_GPIO_Port GPIOC
#define PWM_CH_2_Pin GPIO_PIN_7
#define PWM_CH_2_GPIO_Port GPIOC
#define PWM_CH_3_Pin GPIO_PIN_8
#define PWM_CH_3_GPIO_Port GPIOC
#define PWM_CH_4_Pin GPIO_PIN_9
#define PWM_CH_4_GPIO_Port GPIOC
#define TRIG_IN_4_Pin GPIO_PIN_8
#define TRIG_IN_4_GPIO_Port GPIOA
#define TRIG_IN_4_EXTI_IRQn EXTI9_5_IRQn
#define CAM_OUT_3_Pin GPIO_PIN_9
#define CAM_OUT_3_GPIO_Port GPIOA
#define CAM_OUT_4_Pin GPIO_PIN_10
#define CAM_OUT_4_GPIO_Port GPIOA
#define TM1637_CLK_Pin GPIO_PIN_11
#define TM1637_CLK_GPIO_Port GPIOA
#define TM1637_DIO_Pin GPIO_PIN_12
#define TM1637_DIO_GPIO_Port GPIOA
#define MASTER_NSS_Pin GPIO_PIN_15
#define MASTER_NSS_GPIO_Port GPIOA
#define MASTER_SCK_Pin GPIO_PIN_10
#define MASTER_SCK_GPIO_Port GPIOC
#define MASTER_MISO_Pin GPIO_PIN_11
#define MASTER_MISO_GPIO_Port GPIOC
#define MASTER_MOSI_Pin GPIO_PIN_12
#define MASTER_MOSI_GPIO_Port GPIOC
#define TRIG_IN_3_Pin GPIO_PIN_2
#define TRIG_IN_3_GPIO_Port GPIOD
#define TRIG_IN_3_EXTI_IRQn EXTI2_IRQn
#define MULTI_B_Pin GPIO_PIN_3
#define MULTI_B_GPIO_Port GPIOB
#define MULTI_C_Pin GPIO_PIN_4
#define MULTI_C_GPIO_Port GPIOB
#define NET_RST_Pin GPIO_PIN_7
#define NET_RST_GPIO_Port GPIOB
#define NET_INT_Pin GPIO_PIN_9
#define NET_INT_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
