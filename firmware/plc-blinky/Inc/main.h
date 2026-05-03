/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
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
#include "stm32f1xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

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
#define scan_pulse_Pin GPIO_PIN_0
#define scan_pulse_GPIO_Port GPIOA
#define IND_OUT_0_Pin GPIO_PIN_1
#define IND_OUT_0_GPIO_Port GPIOA
#define IND_IN_6_Pin GPIO_PIN_4
#define IND_IN_6_GPIO_Port GPIOA
#define IND_IN_7_Pin GPIO_PIN_5
#define IND_IN_7_GPIO_Port GPIOA
#define IND_OUT_3_Pin GPIO_PIN_6
#define IND_OUT_3_GPIO_Port GPIOA
#define IND_OUT_1_Pin GPIO_PIN_7
#define IND_OUT_1_GPIO_Port GPIOA
#define START_BTN_Pin GPIO_PIN_0
#define START_BTN_GPIO_Port GPIOB
#define STOP_BTN_Pin GPIO_PIN_1
#define STOP_BTN_GPIO_Port GPIOB
#define IND_IN_0_Pin GPIO_PIN_10
#define IND_IN_0_GPIO_Port GPIOB
#define IND_IN_1_Pin GPIO_PIN_11
#define IND_IN_1_GPIO_Port GPIOB
#define IND_IN_2_Pin GPIO_PIN_12
#define IND_IN_2_GPIO_Port GPIOB
#define IND_IN_3_Pin GPIO_PIN_13
#define IND_IN_3_GPIO_Port GPIOB
#define IND_IN_4_Pin GPIO_PIN_14
#define IND_IN_4_GPIO_Port GPIOB
#define IND_IN_5_Pin GPIO_PIN_15
#define IND_IN_5_GPIO_Port GPIOB
#define IND_OUT_2_Pin GPIO_PIN_8
#define IND_OUT_2_GPIO_Port GPIOA
#define RS485_DE_Pin GPIO_PIN_3
#define RS485_DE_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
