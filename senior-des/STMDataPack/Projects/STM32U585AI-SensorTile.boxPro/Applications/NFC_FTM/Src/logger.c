/**
  ******************************************************************************
  * @file    NFC_FTM\Src\logger.c
  * @author  MMY Application Team
  * @version V1.6.0
  * @date    20-Oct-2023
  * @brief   Logger implementation
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

/*
******************************************************************************
* INCLUDES
******************************************************************************
*/
#include "main.h"
#include "logger.h"
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
/*
******************************************************************************
* LOCAL DEFINES
******************************************************************************
*/
#define USE_LOGGER LOGGER_ON
      
#if (USE_LOGGER == LOGGER_ON)
#define MAX_HEX_STR         4
#define MAX_HEX_STR_LENGTH  128
char hexStr[MAX_HEX_STR][MAX_HEX_STR_LENGTH];
uint8_t hexStrIdx = 0;
#endif /* #if USE_LOGGER == LOGGER_ON */


char* hex2Str(unsigned char * data, size_t dataLen)
{
  #if (USE_LOGGER == LOGGER_ON)
  {
    unsigned char * pin = data;
    const char * hex = "0123456789ABCDEF";
    char * pout = hexStr[hexStrIdx];
    uint8_t i = 0;
    uint8_t idx = hexStrIdx;
    
    if( dataLen > (MAX_HEX_STR_LENGTH/2) )
    {
      dataLen = (MAX_HEX_STR_LENGTH/2);
    }
    
    if(dataLen == 0)
    {
      pout[0] = 0;     
    } 
    else     
    {
      for(; i < dataLen - 1; ++i)
      {
          *pout++ = hex[(*pin>>4)&0xF];
          *pout++ = hex[(*pin++)&0xF];
      }
      *pout++ = hex[(*pin>>4)&0xF];
      *pout++ = hex[(*pin)&0xF];
      *pout = 0;
    }    
    
    hexStrIdx++;
    hexStrIdx %= MAX_HEX_STR;
    
    return hexStr[idx];
  }
  #else
  {
    return NULL;
  }
  #endif /* #if USE_LOGGER == LOGGER_ON */
}


