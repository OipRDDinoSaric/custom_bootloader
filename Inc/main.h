/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : main.h
 * @brief          : Header for main.c file.
 *                   This file contains the common defines of the application.
 ******************************************************************************
 * @attention
 *
 * <h2><center>&copy; Copyright (c) 2020 STMicroelectronics.
 * All rights reserved.</center></h2>
 *
 * This software component is licensed by ST under Ultimate Liberty license
 * SLA0044, the "License"; You may not use this file except in compliance with
 * the License. You may obtain a copy of the License at:
 *                             www.st.com/SLA0044
 *
 ******************************************************************************
 */
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C"
{
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdbool.h>
#include <stdio.h>
#include "CustomBootLoader.h"

#ifdef _DEBUG
#define LOG_EN true /*!< if build is running as Debug enable logging */
#else
#define LOG_EN false /*!< if build is running as Release disable logging */
#endif
#if LOG_EN == true
/**
 * Tutorial for semihosting to Console:
 * System Workbench for STM32 -> Help -> Help Contents -> Semihosting
 */
#define INFO(f_, ...) printf("INFO:%s:", __func__); \
						  printf((f_), ##__VA_ARGS__)

#define DEBUG(f_, ...) printf("DEBG:%s:%d:%s:", __FILE__, __LINE__, __func__); \
						  printf((f_), ##__VA_ARGS__)

#define WARNING(f_, ...) printf("WARN:%s:%d:%s:", __FILE__, __LINE__, __func__); \
						  printf((f_), ##__VA_ARGS__)

#define ERROR(f_, ...) printf("ERRO:%s:%d:%s:", __FILE__, __LINE__, __func__); \
						  printf((f_), ##__VA_ARGS__)

#else
#define INFO(f_, ...);
#define DEBUG(f_, ...);
#define WARNING(f_, ...);
#define ERROR(f_, ...);
#endif

#define CRLF \r\n

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
#define PC14_OSC32_IN_Pin GPIO_PIN_14
#define PC14_OSC32_IN_GPIO_Port GPIOC
#define PC15_OSC32_OUT_Pin GPIO_PIN_15
#define PC15_OSC32_OUT_GPIO_Port GPIOC
#define PH0_OSC_IN_Pin GPIO_PIN_0
#define PH0_OSC_IN_GPIO_Port GPIOH
#define PH1_OSC_OUT_Pin GPIO_PIN_1
#define PH1_OSC_OUT_GPIO_Port GPIOH
#define BTN_BLUE_Pin GPIO_PIN_0
#define BTN_BLUE_GPIO_Port GPIOA
#define CMD_TX_Pin GPIO_PIN_2
#define CMD_TX_GPIO_Port GPIOA
#define CMD_RX_Pin GPIO_PIN_3
#define CMD_RX_GPIO_Port GPIOA
#define BOOT1_Pin GPIO_PIN_2
#define BOOT1_GPIO_Port GPIOB
#define DBG_RX_Pin GPIO_PIN_9
#define DBG_RX_GPIO_Port GPIOD
#define LED_GREEN_Pin GPIO_PIN_12
#define LED_GREEN_GPIO_Port GPIOD
#define LED_ORANGE_Pin GPIO_PIN_13
#define LED_ORANGE_GPIO_Port GPIOD
#define LED_RED_Pin GPIO_PIN_14
#define LED_RED_GPIO_Port GPIOD
#define LED_BLUE_Pin GPIO_PIN_15
#define LED_BLUE_GPIO_Port GPIOD
#define SWDIO_Pin GPIO_PIN_13
#define SWDIO_GPIO_Port GPIOA
#define SWCLK_Pin GPIO_PIN_14
#define SWCLK_GPIO_Port GPIOA
#define SWO_Pin GPIO_PIN_3
#define SWO_GPIO_Port GPIOB
/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
