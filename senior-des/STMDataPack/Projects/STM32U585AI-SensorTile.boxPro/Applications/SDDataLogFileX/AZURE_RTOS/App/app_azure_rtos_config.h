/**
  ******************************************************************************
  * @file    app_azure_rtos_config.h
  * @author  System Research & Applications Team - Catania Lab.
  * @version V1.6.0
  * @date    20-Oct-2023
  * @brief   app_azure_rtos config header file
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
#ifndef APP_AZURE_RTOS_CONFIG_H
#define APP_AZURE_RTOS_CONFIG_H
#ifdef __cplusplus
extern "C" {
#endif

/* Exported constants --------------------------------------------------------*/
/* Using static memory allocation via threadX Byte memory pools */

#define USE_MEMORY_POOL_ALLOCATION               1

#define TX_APP_MEM_POOL_SIZE                     1024

#define FX_APP_MEM_POOL_SIZE                     (1024*10)

#ifdef __cplusplus
}
#endif

#endif /* APP_AZURE_RTOS_CONFIG_H */
