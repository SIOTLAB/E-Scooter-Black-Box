/**
  ******************************************************************************
  * @file    SDDataLogFileX\Core\Inc\app_threadx.h
  * @author  MCD Application Team
  * @version V1.6.0
  * @date    20-Oct-2023
  * @brief   ThreadX applicative header file
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
#ifndef __APP_THREADX_H
#define __APP_THREADX_H

#ifdef __cplusplus
 extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "tx_api.h"

/* Exported functions prototypes ---------------------------------------------*/
UINT App_ThreadX_Init(VOID *memory_ptr);
void MX_ThreadX_Init(void);

#ifdef __cplusplus
}
#endif
#endif /* __APP_THREADX_H__ */
