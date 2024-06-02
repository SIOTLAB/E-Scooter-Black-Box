/**
  ******************************************************************************
  * @file    BLESensorsPnPL\Inc\STBOX1_config.h
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

#define STTS22H_ODR 1.0f /* ODR = 1.0Hz */
#define LSM6DSV16X_ACC_ODR 120.0f /* ODR = 120Hz */
#define LSM6DSV16X_ACC_FS 4 /* FS = 4g */
#define LSM6DSV16X_GYRO_ODR 120.0f /* ODR = 120Hz */
#define LSM6DSV16X_GYRO_FS 2000 /* FS = 2000dps */
#define LIS2DU12_ACC_ODR 100.0f /* ODR = 100Hz */
#define LIS2DU12_ACC_FS 4 /* FS = 4g */
#define LPS22DF_ODR 25.0f /* ODR = 25.0Hz */
#define LIS2MDL_MAG_ODR 100.0f /* ODR = 100Hz */
#define LIS2MDL_MAG_FS 50 /* FS = 50gauss */

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
#define STBOX1_PACKAGENAME "BLESensorsPnPL"

/* Firmware IDs */
#define STBOX1A_BLUEST_SDK_FW_ID 0x11
#define STBOX1B_BLUEST_SDK_FW_ID 0x02

#ifdef STBOX1_ENABLE_PRINTF
  #define STBOX1_PRINTF(...) printf(__VA_ARGS__)
#else /* STBOX1_ENABLE_PRINTF */
  #define STBOX1_PRINTF(...)  
#endif /* STBOX1_ENABLE_PRINTF */

#endif /* __STBOX1_CONFIG_H */

