/**
  ******************************************************************************
  * @file    SDDataLogFileX\Core\Inc\main.h
  * @author  System Research & Applications Team - Catania Lab.
  * @version V1.6.0
  * @date    20-Oct-2023
  * @brief   Header for main.c file.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2023 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32u5xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "SensorTileBoxPro.h"
#include "SensorTileBoxPro_sd.h"
#include "fx_stm32_sd_driver.h"
#include "STBOX1_config.h"
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
extern void Error_Handler(char *File,int32_t Line);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
/* USER CODE BEGIN Private defines */   

/* USER CODE END Private defines */

/* Exported Variables --------------------------------------------------------*/

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
