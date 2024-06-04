/**
  ******************************************************************************
  * @file    NFC_FTM\Inc\flashl4_if.h
  * @author  MMY Application Team
  * @version V1.6.0
  * @date    20-Oct-2023
  * @brief   Header file for flash_if.c
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
#ifndef FLASH_IF_H
#define FLASH_IF_H

#ifdef __cplusplus
 extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
   
/** @addtogroup Flash_Interface
  * @{
  */
   
   
/* Exported types ------------------------------------------------------------*/

/* Exported constants --------------------------------------------------------*/
/* Define the flash memory start address */
#define USER_FLASH_STARTADDRESS    ((uint32_t)0x08000000) 

/* Define the address from where firmware will be stored. */
#define FIRMWARE_ADDRESS        0x08100000      /* Upgraded firmware base address */

/* Last Page Address for firmware */
#define FIRMWARE_FLASH_LAST_PAGE_ADDRESS  0x081FFFFF - 4      /* Upgraded firmware last address */

/* Define the user application size */
#define FIRMWARE_FLASH_SIZE   (FIRMWARE_FLASH_LAST_PAGE_ADDRESS - FIRMWARE_ADDRESS + 1)     /* Upgraded firmware area size */

/* Define the address from where user data will be stored. */
#define USER_DATA_ADDRESS        (FIRMWARE_ADDRESS)      /* User data base address */

/* Last Page Address for data */
#define USER_DATA_FLASH_LAST_PAGE_ADDRESS  (FIRMWARE_FLASH_LAST_PAGE_ADDRESS)   /* User data last address */

/* Define the user data size */
#define USER_DATA_FLASH_SIZE   (USER_DATA_FLASH_LAST_PAGE_ADDRESS - USER_DATA_ADDRESS + 1)    /* User data area size */

/* Exported macros -----------------------------------------------------------*/
/* Exported functions ------------------------------------------------------- */ 

void FLASH_W_FlashLock( void );
void FLASH_W_FlashUnlock( void );
uint32_t FLASH_W_PageErase( const uint32_t Address, const uint32_t LastAddress );
uint32_t FLASH_W_WriteBuffer( const uint32_t Address, const uint32_t pData , const uint32_t Size );
void FLASH_W_ConfBFB2( void );

/**
  * @}
  */
  
#ifdef __cplusplus
}
#endif

#endif  /* FLASH_IF_H */
