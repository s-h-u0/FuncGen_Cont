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
#include "stm32f4xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "z_displ_ILI9XXX.h"
#include "z_touch_xpt2046.h"
#include "mux_sn74lvc1g3157.h"
#include "dpot_AD5292.h"
#include "dds_AD9833.h"

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

void HAL_TIM_MspPostInit(TIM_HandleTypeDef *htim);

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define DISPL_CS_Pin GPIO_PIN_0
#define DISPL_CS_GPIO_Port GPIOF
#define DISPL_RST_Pin GPIO_PIN_1
#define DISPL_RST_GPIO_Port GPIOF
#define DISPL_DC_Pin GPIO_PIN_2
#define DISPL_DC_GPIO_Port GPIOF
#define TOUCH_MISO_Pin GPIO_PIN_2
#define TOUCH_MISO_GPIO_Port GPIOC
#define DISPL_MOSI_Pin GPIO_PIN_3
#define DISPL_MOSI_GPIO_Port GPIOC
#define DISPL_LED_Pin GPIO_PIN_0
#define DISPL_LED_GPIO_Port GPIOA
#define AD9833_CS_Pin GPIO_PIN_4
#define AD9833_CS_GPIO_Port GPIOA
#define AD9833_CLK_Pin GPIO_PIN_5
#define AD9833_CLK_GPIO_Port GPIOA
#define AD9833_MOSI_Pin GPIO_PIN_7
#define AD9833_MOSI_GPIO_Port GPIOA
#define TOUCH_CS_Pin GPIO_PIN_5
#define TOUCH_CS_GPIO_Port GPIOC
#define DISPL_SCK_Pin GPIO_PIN_10
#define DISPL_SCK_GPIO_Port GPIOB
#define TOUCH_INT_Pin GPIO_PIN_12
#define TOUCH_INT_GPIO_Port GPIOD
#define TOUCH_INT_EXTI_IRQn EXTI15_10_IRQn
#define MCP3428_SDA_Pin GPIO_PIN_9
#define MCP3428_SDA_GPIO_Port GPIOC
#define MCP3428_SCK_Pin GPIO_PIN_8
#define MCP3428_SCK_GPIO_Port GPIOA
#define AD9833_CSA15_Pin GPIO_PIN_15
#define AD9833_CSA15_GPIO_Port GPIOA
#define AD5292_CS_Pin GPIO_PIN_2
#define AD5292_CS_GPIO_Port GPIOD
#define AD5292_RDY_Pin GPIO_PIN_3
#define AD5292_RDY_GPIO_Port GPIOD
#define DIP_BIT_1_Pin GPIO_PIN_4
#define DIP_BIT_1_GPIO_Port GPIOD
#define DIP_BIT_2_Pin GPIO_PIN_5
#define DIP_BIT_2_GPIO_Port GPIOD
#define DIP_BIT_4_Pin GPIO_PIN_6
#define DIP_BIT_4_GPIO_Port GPIOD
#define DIP_BIT_8_Pin GPIO_PIN_7
#define DIP_BIT_8_GPIO_Port GPIOD
#define CLK_MUX_PIN_S_Pin GPIO_PIN_6
#define CLK_MUX_PIN_S_GPIO_Port GPIOB
#define MCP4728_SCK_Pin GPIO_PIN_8
#define MCP4728_SCK_GPIO_Port GPIOB
#define MCP4728_SDA_Pin GPIO_PIN_9
#define MCP4728_SDA_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */
#define Ext_CLK_ToDDS  SWITCH_CH_B2  /* 外部クロック：S=High → B2 ↔ A */
#define Int_CLK_ToDDS  SWITCH_CH_B1  /* 内部クロック：S=Low  → B1 ↔ A */

extern TIM_HandleTypeDef htim4;
/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */

