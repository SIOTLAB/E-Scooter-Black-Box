/**
  ******************************************************************************
  * @file    IControl.h
  * @author  SRA
  * @brief
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
  * dtmi:appconfig:steval_mkboxpro:FP_SNS_STBOX1_BLESensorPnPL:other:control:control;1
  *
  * Created by: DTDL2PnPL_cGen version 1.1.0
  *
  * WARNING! All changes made to this file will be lost if this is regenerated
  ******************************************************************************
  */

#ifndef INCLUDE_ICONTROL_H_
#define INCLUDE_ICONTROL_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "stdint.h"

/**
  * Create  type name for IControl.
  */
typedef struct _IControl_t IControl_t;

/* Public API declarations ---------------------------------------------------*/

/* Public interface */
inline uint8_t IControl_pause_resume(IControl_t *_this);

#ifdef __cplusplus
}
#endif

#endif /* INCLUDE_ICONTROL_H_ */
