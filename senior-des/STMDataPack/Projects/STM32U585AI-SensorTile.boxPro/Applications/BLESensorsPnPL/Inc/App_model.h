/**
  ******************************************************************************
  * @file    App_model.h
  * @author  SRA
  * @brief   App Application Model and PnPL Components APIs
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
  * This file has been auto generated from the following Device Template Model:
  * dtmi:appconfig:steval_mkboxpro:FP_SNS_STBOX1_BLESensorPnPL;1
  *
  * Created by: DTDL2PnPL_cGen version 1.1.0
  *
  * WARNING! All changes made to this file will be lost if this is regenerated
  ******************************************************************************
  */

/**
  ******************************************************************************
  * Component APIs *************************************************************
  * - Component init function
  *    <comp_name>_comp_init(void)
  * - Component get_key function
  *    <comp_name>_get_key(void)
  * - Component GET/SET Properties APIs ****************************************
  *  - GET Functions
  *    uint8_t <comp_name>_get_<prop_name>(prop_type *value)
  *      if prop_type == char --> (char **value)
  *  - SET Functions
  *    uint8_t <comp_name>_set_<prop_name>(prop_type value)
  *      if prop_type == char --> (char *value)
  *  - Component COMMAND Reaction Functions
  *      uint8_t <comp_name>_<command_name>(I<Compname>_t * ifn,
  *                     field1_type field1_name, field2_type field2_name, ...,
  *                     fieldN_type fieldN_name); //ifn: Interface Functions
  *  - Component TELEMETRY Send Functions
  *      uint8_t <comp_name>_create_telemetry(tel1_type tel1_name,
  *                     tel2_type tel2_name, ..., telN_type telN_name,
  *                     char **telemetry, uint32_t *size)
  ******************************************************************************
  */

#ifndef APP_MODEL_H_
#define APP_MODEL_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "stdint.h"
#include "stdbool.h"
#include "PnPLCompManager.h"
#include "IControl.h"
#include "IControl_vtbl.h"
/* USER includes -------------------------------------------------------------*/

#define COMP_TYPE_SENSOR          0x00
#define COMP_TYPE_ALGORITHM       0x01
#define COMP_TYPE_OTHER           0x02
#define COMP_TYPE_ACTUATOR        0x03

#define LOG_CTRL_MODE_SD          0x00
#define LOG_CTRL_MODE_USB         0x01
#define LOG_CTRL_MODE_BLE         0x02

#define SENSOR_NUMBER             0
#define ALGORITHM_NUMBER	        0
#define ACTUATOR_NUMBER	          0
#define OTHER_COMP_NUMBER	        4

typedef struct _EnvironmentalModel_t
{
  char *comp_name;
  /* Environmental Component Model USER code */
} EnvironmentalModel_t;

typedef struct _InertialModel_t
{
  char *comp_name;
  /* Inertial Component Model USER code */
} InertialModel_t;

typedef struct _ControlModel_t
{
  char *comp_name;
  /* Control Component Model USER code */
} ControlModel_t;

typedef struct _ConfigurationModel_t
{
  char *comp_name;
  /* Configuration Component Model USER code */
} ConfigurationModel_t;

typedef struct _AppModel_t
{
  EnvironmentalModel_t environmental_model;
  InertialModel_t inertial_model;
  ControlModel_t control_model;
  ConfigurationModel_t configuration_model;
  /* Insert here your custom App Model code */
} AppModel_t;

AppModel_t *getAppModel(void);

/* Device Components APIs ----------------------------------------------------*/


/* Environmental PnPL Component ----------------------------------------------*/
uint8_t environmental_comp_init(void);
char* environmental_get_key(void);
uint8_t environmental_get_samplerate(float *value);
uint8_t environmental_set_samplerate(float value);

/* Inertial PnPL Component ---------------------------------------------------*/
uint8_t inertial_comp_init(void);
char* inertial_get_key(void);
uint8_t inertial_get_samplerate(float *value);
uint8_t inertial_get_change_acc(float *value);
uint8_t inertial_set_samplerate(float value);
uint8_t inertial_set_change_acc(float value);

/* Control PnPL Component ----------------------------------------------------*/
uint8_t control_comp_init(void);
char* control_get_key(void);
uint8_t control_get_fw_status(char **value);
uint8_t control_pause_resume(IControl_t *ifn);

/* Configuration PnPL Component ----------------------------------------------*/
uint8_t configuration_comp_init(void);
char* configuration_get_key(void);
uint8_t configuration_get_version_fw(char **value);
uint8_t configuration_get_board_name(char **value);
uint8_t configuration_set_board_name(const char *value);

/* Device Information PnPL Component -----------------------------------------*/
uint8_t DeviceInformation_comp_init(void);
char* DeviceInformation_get_key(void);
uint8_t DeviceInformation_get_manufacturer(char **value);
uint8_t DeviceInformation_get_model(char **value);
uint8_t DeviceInformation_get_swVersion(char **value);
uint8_t DeviceInformation_get_osName(char **value);
uint8_t DeviceInformation_get_processorArchitecture(char **value);
uint8_t DeviceInformation_get_processorManufacturer(char **value);
uint8_t DeviceInformation_get_totalStorage(float *value);
uint8_t DeviceInformation_get_totalMemory(float *value);

#ifdef __cplusplus
}
#endif

#endif /* APP_MODEL_H_ */
