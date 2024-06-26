/**
  ******************************************************************************
  * @file    BLESensorsPnPL\Inc\BLE_Manager_Conf.h
  * @author  System Research & Applications Team - Agrate/Catania Lab.
  * @version V1.6.0
  * @date    20-Oct-2023
  * @brief   BLE Manager configuration file
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
#ifndef __BLE_MANAGER_CONF_H__
#define __BLE_MANAGER_CONF_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Exported define ------------------------------------------------------------*/
#define BLUE_CORE BLUENRG_LP

/* Uncomment the following define for BlueST-SDK V2 */
#define BLE_MANAGER_SDKV2
  
/* Define the Max dimesion of the Bluetooth characteristics for Debug Console services  */
#define DEFAULT_MAX_STDOUT_CHAR_LEN     244
#define DEFAULT_MAX_STDERR_CHAR_LEN     244
   
#define BLE_MANAGER_MAX_ALLOCABLE_CHARS 8U
  
/* For enabling the capability to handle BlueNRG Congestion */
#define ACC_BLUENRG_CONGESTION

/* Define the Delay function to use inside the BLE Manager (HAL_Delay/osDelay) */
#define BLE_MANAGER_DELAY HAL_Delay
  
/****************** Memory managment functions **************************/
#define BLE_MALLOC_FUNCTION      malloc
#define BLE_FREE_FUNCTION        free
#define BLE_MEM_CPY              memcpy
  
/*---------- Print messages from BLE Manager files at middleware level -----------*/
/* Uncomment/Comment the following define for  disabling/enabling print messages from BLE Manager files */
#define BLE_MANAGER_DEBUG

#ifdef BLE_MANAGER_DEBUG
  
  #define BLE_DEBUG_LEVEL 1

  #include "SensorTileBoxPro.h"
  #include "STBOX1_config.h"
  #include "bluenrg_conf.h"
  #define BLE_MANAGER_PRINTF	STBOX1_PRINTF

#else
  #define BLE_MANAGER_PRINTF(...)
#endif

/******************Experimental defines **************************/
//#define BLE_MANAGER_NO_PARSON

#ifdef __cplusplus
}
#endif

#endif /* __BLE_MANAGER_CONF_H__*/

