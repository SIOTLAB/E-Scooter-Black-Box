/**
  ******************************************************************************
  * @file    Environmental_PnPL.h
  * @author  SRA
  * @brief   Environmental PnPL Component Manager
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
  * dtmi:appconfig:steval_mkboxpro:FP_SNS_STBOX1_BLESensorPnPL:other:environmental:environmental;1
  *
  * Created by: DTDL2PnPL_cGen version 1.1.0
  *
  * WARNING! All changes made to this file will be lost if this is regenerated
  ******************************************************************************
  */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef _PNPL_ENVIRONMENTAL_H_
#define _PNPL_ENVIRONMENTAL_H_

#ifdef __cplusplus
extern "C" {
#endif
  
#define n1 1
#define n10 10
#define n20 20

/* Includes ------------------------------------------------------------------*/
#include "parson.h"
#include "IPnPLComponent.h"
#include "IPnPLComponent_vtbl.h"


/**
  * Create a type name for _Environmental_PnPL.
 */
typedef struct _Environmental_PnPL Environmental_PnPL;

/* Public API declaration ----------------------------------------------------*/

IPnPLComponent_t *Environmental_PnPLAlloc(void);

/**
  * Initialize the default parameters.
  *
 */
uint8_t Environmental_PnPLInit(IPnPLComponent_t *_this);


/**
  * Get the IPnPLComponent interface for the component.
  * @param _this [IN] specifies a pointer to a PnPL component.
  * @return a pointer to the generic object ::IPnPLComponent if success,
  * or NULL if out of memory error occurs.
 */
IPnPLComponent_t *Environmental_PnPLGetComponentIF(Environmental_PnPL *_this);

#ifdef __cplusplus
}
#endif

#endif /* _PNPL_ENVIRONMENTAL_H_ */
