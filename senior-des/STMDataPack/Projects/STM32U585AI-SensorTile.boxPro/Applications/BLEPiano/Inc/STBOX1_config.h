/**
  ******************************************************************************
  * @file    BLEPiano\Inc\STBOX1_config.h
  * @author  System Research & Applications Team - Agrate/Catania Lab.
  * @version V1.6.0
  * @date    20-Oct-2023
  * @brief   FP-SNS-STBOX1 configuration
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
#ifndef __STBOX1_CONFIG_H
#define __STBOX1_CONFIG_H

/* Exported define ------------------------------------------------------------*/

/* For enabling the printf */
#define STBOX1_ENABLE_PRINTF

/* Update/Blink Battery/LED Every second */
#define STBOX1_UPDATE_LED_BATTERY 10000

/**************************************
 *  Lab/Experimental section defines  *
***************************************/

/**************************************
 * Don't Change the following defines *
***************************************/

/* Package Version only numbers 0->9 */
#define STBOX1_VERSION_MAJOR '1'
#define STBOX1_VERSION_MINOR '6'
#define STBOX1_VERSION_PATCH '0'

/* Package Name */
#define STBOX1_PACKAGENAME "BLEPiano"

/* Firmware IDs */
#define STBOX1A_BLUEST_SDK_FW_ID 0x10
#define STBOX1B_BLUEST_SDK_FW_ID 0x03

#ifdef STBOX1_ENABLE_PRINTF
  #define STBOX1_PRINTF(...) printf(__VA_ARGS__)
#else /* STBOX1_ENABLE_PRINTF */
  #define STBOX1_PRINTF(...)  
#endif /* STBOX1_ENABLE_PRINTF */

#endif /* __STBOX1_CONFIG_H */

