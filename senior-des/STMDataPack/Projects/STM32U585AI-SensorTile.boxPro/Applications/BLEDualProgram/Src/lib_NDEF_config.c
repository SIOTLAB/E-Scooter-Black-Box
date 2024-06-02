/**
  ******************************************************************************
  * @file    BLEDualProgram\Src\lib_NDEF_config.c
  * @author  System Research & Applications Team - Agrate/Catania Lab.
  * @version V1.6.0
  * @date    20-Oct-2023
  * @brief   This file is a template to be modified in the project to configure
  *          how the lib_NDEF is supposed to access the nfctag memory.
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

#include "lib_NDEF_config.h"
#include "SensorTileBoxPro_nfctag.h"

/**
  * @brief  Reads data in the nfctag at specific address
  * @param  pData : pointer to store read data
  * @param  offset: Address to read
  * @param  Size : Size in bytes of the value to be read
  * @retval NDEF_OK if success, NDEF_ERROR in case of failure
  */
int32_t NDEF_Wrapper_ReadData(uint8_t* pData, uint32_t offset, uint32_t length )
{
  if(BSP_NFCTAG_ReadData(BSP_NFCTAG_INSTANCE, pData, offset, length ) != NFCTAG_OK)
    return NDEF_ERROR;
  return NDEF_OK;
}

/**
  * @brief  Zrites data in the nfctag at specific address
  * @param  pData : pointer to the data to be written
  * @param  offset: Address to write
  * @param  Size : Number of bytes to be written
  * @retval NDEF_OK if success, NDEF_ERROR in case of failure
  */
int32_t NDEF_Wrapper_WriteData(const uint8_t* pData, uint32_t offset, uint32_t length )
{
  if(BSP_NFCTAG_WriteData(BSP_NFCTAG_INSTANCE, pData, offset, length ) != NFCTAG_OK)
    return NDEF_ERROR;
  return NDEF_OK;
}

/**
  * @brief  Compute the NFCTAG Memory Size.
  * @return uint32_t Memory size in bytes.
  */
uint32_t NDEF_Wrapper_GetMemorySize(void)
{
  return BSP_NFCTAG_GetByteSize(BSP_NFCTAG_INSTANCE);
}
