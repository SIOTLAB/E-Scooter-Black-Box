/**
  ******************************************************************************
  * @file    NFC_FTM\Src\flashf4_if.c
  * @author  MMY Application Team
  * @version V1.6.0
  * @date    20-Oct-2023
  * @brief   This file provides all the flash layer functions.
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
#include "flashl4_if.h"
#include "main.h"


/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macros ------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/


/* Private functions ---------------------------------------------------------*/
static uint32_t GetPage(uint32_t Address);
static uint32_t FLASH_W_WriteQWord( const uint32_t Address, const uint64_t pData );

/**
  * @brief  Locks the Flash to disable the flash control register access
  * @param  None
  * @return None
  */
void FLASH_W_FlashLock( void )
{
  /* Lock the Flash to disable the flash control register access  *********/
  HAL_FLASH_Lock();
  
  /* Re-enable instruction cache */
  if (HAL_ICACHE_Enable() != HAL_OK)
  {
    Error_Handler(STBOX1_ERROR_FLASH,__FILE__,__LINE__);
  }
}

/**
  * @brief  Unlocks the Flash to enable the flash control register access
  * @param  None
  * @return None
  */
void FLASH_W_FlashUnlock( void )
{
  /* Disable instruction cache prior to internal cacheable memory update */
  if (HAL_ICACHE_Disable() != HAL_OK)
  {
    Error_Handler(STBOX1_ERROR_FLASH,__FILE__,__LINE__);
  }

  /* Unlock the Flash to enable the flash control register access *************/
  HAL_FLASH_Unlock();
}

/**
  * @brief  Erases the required FLASH Pages computed with destination address.
  * @param  Address Start address for erasing data.
  * @param  LastAddress End address of flash area.
  * @retval 0 Erase sectors done with success.
  * @retval 1 Erase error.
  */
uint32_t FLASH_W_PageErase( const uint32_t Address, const uint32_t LastAddress )
{
  FLASH_EraseInitTypeDef EraseInitStruct;
  uint32_t SectorError = 0;
  
  if(CurrentActiveBank==1) {
    EraseInitStruct.Banks       = FLASH_BANK_2;
  } else {
    EraseInitStruct.Banks       = FLASH_BANK_1;
  }

  EraseInitStruct.Page        = GetPage(Address);
  EraseInitStruct.TypeErase   = FLASH_TYPEERASE_PAGES;
  EraseInitStruct.NbPages     = GetPage(LastAddress) - EraseInitStruct.Page + 1;
  
  if(HAL_FLASHEx_Erase(&EraseInitStruct, &SectorError) != HAL_OK){
    /* Error occurred while sector erase. 
      User can add here some code to deal with this error. 
      SectorError will contain the faulty sector and then to know the code error on this sector,
      user can call function 'HAL_FLASH_GetError()'
      FLASH_ErrorTypeDef errorcode = HAL_FLASH_GetError(); */
    Error_Handler(STBOX1_ERROR_FLASH,__FILE__,__LINE__);
  } else {
    STBOX1_PRINTF("End FLASH Erase %ld Pages of 16KB\r\n",EraseInitStruct.NbPages);
  }
   
  return 0;
}

/**
  * @brief  Writes a data buffer in the flash.
  * @param  Address Start address for writing data buffer.
  * @param  pData Pointer on data buffer.
  * @param  Size Size of the data.
  * @retval 0 Data successfully written to Flash memory.
  * @retval 1 Error occurred while writing data in Flash memory.
  */
uint32_t FLASH_W_WriteBuffer( const uint32_t Address, uint32_t pData , const uint32_t Size )
{
  uint32_t cnt = 0;
  uint32_t ret = 0;
  FLASH_W_FlashUnlock();
  
  /* We move at step of 16 */
  for( cnt = 0; cnt < Size; cnt+=16 )
  {
     ret |= FLASH_W_WriteQWord( (Address + cnt), pData+ cnt);
  }
  
  FLASH_W_FlashLock();
  
  return ret;
}

/**
  * @brief  Writes a data in flash (data are 64-bit aligned).
  * @param  Address Start address for writing data buffer.
  * @param  Data Word data value to write.
  * @retval 0 Data successfully written to Flash memory.
  * @retval 1 Error occurred while writing data in Flash memory.
  */
static uint32_t FLASH_W_WriteQWord( const uint32_t Address, const uint64_t pData )
{
  uint32_t ret = 1;
  
  /* Program the user Flash area word by word
    (area defined by Address and LastAddress) ***********/
  
  if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_QUADWORD, Address, pData) == HAL_OK)
    ret = 0;  

  return ret;
}

/**
  * @brief  Set/Reset BFB2 bit to select boot from Flash Bank.
  * @return None
  */
void FLASH_W_ConfBFB2( void )
{
  FLASH_OBProgramInitTypeDef    OBInit; 
  /* Set BFB2 bit to enable boot from Flash Bank2 */
  /* Allow Access to Flash control registers and user Flash */
  HAL_FLASH_Unlock();

  /* Allow Access to option bytes sector */ 
  HAL_FLASH_OB_Unlock();

  /* Get the Dual boot configuration status */
  HAL_FLASHEx_OBGetConfig(&OBInit);

  /* Enable/Disable dual boot feature */
  OBInit.OptionType = OPTIONBYTE_USER;
  OBInit.USERType   = OB_USER_SWAP_BANK;

  if (((OBInit.USERConfig) & (FLASH_OPTR_SWAP_BANK)) == FLASH_OPTR_SWAP_BANK) {
    OBInit.USERConfig &= ~FLASH_OPTR_SWAP_BANK;
    STBOX1_PRINTF("->Disable DualBoot\r\n");
  } else {
    OBInit.USERConfig = FLASH_OPTR_SWAP_BANK;
    STBOX1_PRINTF("->Enable DualBoot\r\n");
  }

  if(HAL_FLASHEx_OBProgram (&OBInit) != HAL_OK) {
    /*
    Error occurred while setting option bytes configuration.
    User can add here some code to deal with this error.
    To know the code error, user can call function 'HAL_FLASH_GetError()'
    */
    Error_Handler(STBOX1_ERROR_FLASH,__FILE__,__LINE__);
  }

  /* Start the Option Bytes programming process */  
  if (HAL_FLASH_OB_Launch() != HAL_OK) {
    /*
    Error occurred while reloading option bytes configuration.
    User can add here some code to deal with this error.
    To know the code error, user can call function 'HAL_FLASH_GetError()'
    */
    Error_Handler(STBOX1_ERROR_FLASH,__FILE__,__LINE__);
  }
  HAL_FLASH_OB_Lock();
  HAL_FLASH_Lock();
}

/**
  * @brief  Gets the page of a given address
  * @param  Addr: Address of the FLASH Memory
  * @retval The page of a given address
  */
static uint32_t GetPage(uint32_t Addr)
{
  uint32_t page = 0;

  if (Addr < (FLASH_BASE + FLASH_BANK_SIZE))
  {
    /* Bank 1 */
    page = (Addr - FLASH_BASE) / FLASH_PAGE_SIZE;
  }
  else
  {
    /* Bank 2 */
    page = (Addr - (FLASH_BASE + FLASH_BANK_SIZE)) / FLASH_PAGE_SIZE;
  }

  return page;
}
