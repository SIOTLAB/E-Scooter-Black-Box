/**
  ******************************************************************************
  * @file    NFC_FTM\Src\fw_command.c
  * @author  MMY Application Team
  * @version V1.6.0
  * @date    20-Oct-2023
  * @brief   FTM Commands
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

/* Includes ------------------------------------------------------------------*/
#include "st25ftm_config.h"
#include "fw_command.h"


/** @addtogroup ST25_Discovery_Demo
  * @{
  */

/** @defgroup Flash_Command Flash Command
  * @brief This module implements high level functions to write firmware or data to flash.
  * @details The module covers following functions:
  * - Erase Flash command.
  * - Write buffer to flash.
  * - Jump to firmware command.
  * @{
  */
  
/* Private typedef -----------------------------------------------------------*/
/* Private defines -----------------------------------------------------------*/
/* Private macros ------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
uint32_t              JumpAddress;
pFunction             Jump_To_Application;

/* Private function prototypes -----------------------------------------------*/
/* Private functions ---------------------------------------------------------*/

/**
  * @brief  Command To erase specific Flash memory area.
  * @param  Address Start address for erasing data.
  * @retval 0 Erase sectors done with success.
  * @retval 1 Erase error.
  */
uint32_t COMMAND_EraseFlash( const uint32_t Address )
{
  uint32_t ret = 0;
  FLASH_W_FlashUnlock( );
  /* Erase FLASH sectors to download image */
  if( Address == FIRMWARE_ADDRESS )
  {
      if( FLASH_W_PageErase( FIRMWARE_ADDRESS, FIRMWARE_FLASH_LAST_PAGE_ADDRESS ) != 0x00 )
    {
      /* Error Erase */
      ret = 1;
    }
  }
//  else
//  {
//      if( FLASH_If_EraseSectors( USER_DATA_ADDRESS, USER_DATA_FLASH_LAST_PAGE_ADDRESS ) != 0x00 )
//    {
//      /* Error Erase */
//      ret = 1;
//    }
//  }
  FLASH_W_FlashLock( );
  
  return ret;
}

/**
  * @brief  Writes buffer to Flash memory.
  * @param  StartAddress Start address for writing data.
  * @param  offset Offset of data to write.
  * @param  pData Buffer pointer to write.
  * @param  size Size of data to write.
  * @retval 0 Write Success.
  * @retval 1 Write Error.
  */
uint32_t Command_WriteBufferToFlash( const uint32_t StartAddress, const uint32_t offset, const uint8_t * const pData, const uint32_t size )
{
  uint32_t ret = 0;
  uint32_t p_Data =(uint32_t)pData;
  
  /* Check */
  if(size%16) {
    STBOX1_PRINTF("Command_WriteBufferToFlash Size=%d not multiple of 16\r\n",size);
  }
  
  if( StartAddress == FIRMWARE_ADDRESS )
  {
    ret = FLASH_W_WriteBuffer( (FIRMWARE_ADDRESS + offset), p_Data, size);
  }
  else
  {
    ret = FLASH_W_WriteBuffer( (USER_DATA_ADDRESS + offset), p_Data, size);
  }
  
  return ret;
}

/**
  * @brief  Jump to user program.
  * @param  None No parameters.
  * @return None.
  */
void COMMAND_Jump( void )
{
  FLASH_W_ConfBFB2();
}

/**
  * @}
  */

/**
  * @}
  */
