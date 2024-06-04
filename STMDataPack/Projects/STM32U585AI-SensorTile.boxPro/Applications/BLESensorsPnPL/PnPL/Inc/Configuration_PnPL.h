/**
  ******************************************************************************
  * @file    Configuration_PnPL.h
  * @author  SRA
  * @brief   Configuration PnPL Component Manager
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2023 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file in
  * the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  *
  ******************************************************************************
  */

/**
  ******************************************************************************
  * This file has been auto generated from the following DTDL Component:
  * dtmi:appconfig:steval_mkboxpro:FP_SNS_STBOX1_BLESensorPnPL:other:configuration:configuration;1
  *
  * Created by: DTDL2PnPL_cGen version 1.1.0
  *
  * WARNING! All changes made to this file will be lost if this is regenerated
  ******************************************************************************
  */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef _PNPL_CONFIGURATION_H_
#define _PNPL_CONFIGURATION_H_



#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "parson.h"
#include "IPnPLComponent.h"
#include "IPnPLComponent_vtbl.h"


/**
  * Create a type name for _Configuration_PnPL.
 */
typedef struct _Configuration_PnPL Configuration_PnPL;

/* Public API declaration ----------------------------------------------------*/

IPnPLComponent_t *Configuration_PnPLAlloc(void);

/**
  * Initialize the default parameters.
  *
 */
uint8_t Configuration_PnPLInit(IPnPLComponent_t *_this);


/**
  * Get the IPnPLComponent interface for the component.
  * @param _this [IN] specifies a pointer to a PnPL component.
  * @return a pointer to the generic object ::IPnPLComponent if success,
  * or NULL if out of memory error occurs.
 */
IPnPLComponent_t *Configuration_PnPLGetComponentIF(Configuration_PnPL *_this);

#ifdef __cplusplus
}
#endif

#endif /* _PNPL_CONFIGURATION_H_ */
