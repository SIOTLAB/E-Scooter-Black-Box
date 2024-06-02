/**
  ******************************************************************************
  * @file    App_Model.c
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

#include "App_model.h"
#include <string.h>
#include <stdio.h>

#include "STBOX1_config.h"
#include "main.h"

/* USER private function prototypes ------------------------------------------*/

/* Imported Variables --------------------------------------------------------*/
extern TIM_HandleTypeDef TimCCHandle;
extern uint8_t TimerEnvIsRunning;
extern uint8_t TimerInerIsRunning;
extern int32_t CurrentEnvUpdateEnumValue;
extern int32_t CurrentInerUpdateEnumValue;

/* Imported Functions --------------------------------------------------------*/
extern tBleStatus PnPLikeEncapsulateAndSend(uint8_t *data,uint32_t length);
extern void ExtConfigSetNameCommandCallback(uint8_t *NewName);
/* USER private functions prototypes -----------------------------------------*/

AppModel_t app_model;

AppModel_t* getAppModel(void)
{
  return &app_model;
}

/* Device Components APIs ----------------------------------------------------*/


/* Environmental PnPL Component ----------------------------------------------*/
uint8_t environmental_comp_init(void)
{
  app_model.environmental_model.comp_name = environmental_get_key();

  /* USER Component initialization code */
  return 0;
}
char* environmental_get_key(void)
{
  return "environmental";
}

uint8_t environmental_get_samplerate(float *value)
{
  /* USER Code */
  *value = CurrentEnvUpdateEnumValue;
  return 0;
}
uint8_t environmental_set_samplerate(float value)
{
  /* USER Code */
  CurrentEnvUpdateEnumValue = (int32_t) value;
  //if the Env are running, stop and restart with the new sample rate
  if(TimerEnvIsRunning) {
    uint32_t uhCapture;
    /* Stop the TIM Base generation in interrupt mode */
    if(HAL_TIM_OC_Stop_IT(&TimCCHandle, TIM_CHANNEL_2) != HAL_OK){
      /* Stopping Error */
      Error_Handler(STBOX1_ERROR_TIMER,__FILE__,__LINE__);
    }

    uhCapture = __HAL_TIM_GET_COUNTER(&TimCCHandle);    
    /* Start the TIM Base generation in interrupt mode */
    if(HAL_TIM_OC_Start_IT(&TimCCHandle, TIM_CHANNEL_2) != HAL_OK){
       /* Starting Error */
       Error_Handler(STBOX1_ERROR_TIMER,__FILE__,__LINE__);
    }

    /* Set the Capture Compare Register value */
    switch(CurrentEnvUpdateEnumValue) {
      case 1:
        __HAL_TIM_SET_COMPARE(&TimCCHandle, TIM_CHANNEL_2, (uhCapture + 10000));
      break;
      case 10:
        __HAL_TIM_SET_COMPARE(&TimCCHandle, TIM_CHANNEL_2, (uhCapture + 1000));
      break;
      case 20:
        __HAL_TIM_SET_COMPARE(&TimCCHandle, TIM_CHANNEL_2, (uhCapture + 500));
      break;
    }
    STBOX1_PRINTF("Start Env@%dHz\r\n",CurrentEnvUpdateEnumValue);
  }
  return 0;
}

/* Inertial PnPL Component ---------------------------------------------------*/
uint8_t inertial_comp_init(void)
{
  app_model.inertial_model.comp_name = inertial_get_key();

  /* USER Component initialization code */
  return 0;
}
char* inertial_get_key(void)
{
  return "inertial";
}

uint8_t inertial_get_samplerate(float *value)
{
  /* USER Code */
  *value =CurrentInerUpdateEnumValue;
  return 0;
}
uint8_t inertial_get_change_acc(float *value)
{
  /* USER Code */
  if( CurrentAccType == STBOX1_ACC_LSM6DSV16X) {
    *value = 0;
  } else {
     *value = 1;
  }
  return 0;
}
uint8_t inertial_set_samplerate(float value)
{
  /* USER Code */
  CurrentInerUpdateEnumValue = (int32_t) value;
  //if the Inertial are running, stop and restart with the new sample rate
  if(TimerInerIsRunning) {
    uint32_t uhCapture;
    /* Stop the TIM Base generation in interrupt mode */
    if(HAL_TIM_OC_Stop_IT(&TimCCHandle, TIM_CHANNEL_3) != HAL_OK){
      /* Stopping Error */
      Error_Handler(STBOX1_ERROR_TIMER,__FILE__,__LINE__);
    }

    uhCapture = __HAL_TIM_GET_COUNTER(&TimCCHandle);    
    /* Start the TIM Base generation in interrupt mode */
    if(HAL_TIM_OC_Start_IT(&TimCCHandle, TIM_CHANNEL_3) != HAL_OK){
       /* Starting Error */
       Error_Handler(STBOX1_ERROR_TIMER,__FILE__,__LINE__);
    }

    /* Set the Capture Compare Register value */
    switch(CurrentInerUpdateEnumValue) {
      case 10:
        __HAL_TIM_SET_COMPARE(&TimCCHandle, TIM_CHANNEL_3, (uhCapture + 1000));
      break;
      case 20:
        __HAL_TIM_SET_COMPARE(&TimCCHandle, TIM_CHANNEL_3, (uhCapture + 500));
      break;
      case 30:
        __HAL_TIM_SET_COMPARE(&TimCCHandle, TIM_CHANNEL_3, (uhCapture + 333));
      break;
    }
    STBOX1_PRINTF("Start Iner@%dHz\r\n",CurrentInerUpdateEnumValue);
  }
  return 0;
}
uint8_t inertial_set_change_acc(float value)
{
  /* USER Code */
  if(value==0.0) {
    CurrentAccType = STBOX1_ACC_LSM6DSV16X;
    STBOX1_PRINTF("Set LSM6DSV16X Like Current Acc\r\n");
  } else {
    CurrentAccType = STBOX1_ACC_LIS2DU12;
    STBOX1_PRINTF("Set LIS2DU12 Like Current Acc\r\n");
  }
  return 0;
}

/* Control PnPL Component ----------------------------------------------------*/
uint8_t control_comp_init(void)
{
  app_model.control_model.comp_name = control_get_key();

  /* USER Component initialization code */
  return 0;
}
char* control_get_key(void)
{
  return "control";
}

uint8_t control_get_fw_status(char **value)
{
  /* USER Code */
  static char FwName[8];
  
  if(TimerEnvIsRunning | TimerInerIsRunning) {
    sprintf(FwName,"Running");
  } else {
     sprintf(FwName,"Paused");
  }
  *value = FwName;
  return 0;
}
uint8_t control_pause_resume(IControl_t *ifn)
{
  //IControl_pause_resume(ifn);
  if(W2ST_CHECK_CONNECTION(W2ST_CONNECT_ENV))  {
    if(TimerEnvIsRunning) {
      /* Stop the TIM Base generation in interrupt mode */
      if(HAL_TIM_OC_Stop_IT(&TimCCHandle, TIM_CHANNEL_2) != HAL_OK){
        /* Stopping Error */
        Error_Handler(STBOX1_ERROR_TIMER,__FILE__,__LINE__);
      }
      STBOX1_PRINTF("Stop Env\r\n");
      TimerEnvIsRunning=0;
    } else {
      uint32_t uhCapture = __HAL_TIM_GET_COUNTER(&TimCCHandle);    
      /* Start the TIM Base generation in interrupt mode */
      if(HAL_TIM_OC_Start_IT(&TimCCHandle, TIM_CHANNEL_2) != HAL_OK){
        /* Starting Error */
        Error_Handler(STBOX1_ERROR_TIMER,__FILE__,__LINE__);
      }

      /* Set the Capture Compare Register value */
      switch(CurrentEnvUpdateEnumValue) {
        case 1:
          __HAL_TIM_SET_COMPARE(&TimCCHandle, TIM_CHANNEL_2, (uhCapture + 10000));
        break;
        case 10:
          __HAL_TIM_SET_COMPARE(&TimCCHandle, TIM_CHANNEL_2, (uhCapture + 1000));
        break;
        case 20:
          __HAL_TIM_SET_COMPARE(&TimCCHandle, TIM_CHANNEL_2, (uhCapture + 500));
        break;
      }
      STBOX1_PRINTF("Start Env@%dHz\r\n",CurrentEnvUpdateEnumValue);
      TimerEnvIsRunning=1;
    }
    
    {
        PnPLCommand_t PnPLCommand;
        char *SerializedJSON;
        uint32_t size;
        
        //sprintf(PnPLCommand.comp_name,"%s",Control_get_key());
        sprintf(PnPLCommand.comp_name,"%s","all");
        PnPLCommand.comm_type = PNPL_CMD_GET;
        
        PnPLSerializeResponse(&PnPLCommand,&SerializedJSON,&size,0);

        STBOX1_PRINTF("--> <%.*s>\r\n",size,SerializedJSON);

        PnPLikeEncapsulateAndSend((uint8_t*) SerializedJSON,size);
      }
    
  } else if(W2ST_CHECK_CONNECTION(W2ST_CONNECT_ACC_GYRO_MAG)) {
    STBOX1_PRINTF("The pause/resume command has not effect in this situation\r\n");
  }
  return 0;
}

/* Configuration PnPL Component ----------------------------------------------*/
uint8_t configuration_comp_init(void)
{
  app_model.configuration_model.comp_name = configuration_get_key();

  /* USER Component initialization code */
  return 0;
}
char* configuration_get_key(void)
{
  return "configuration";
}

uint8_t configuration_get_version_fw(char **value)
{
  /* USER Code */
  static char FwName[32];
   sprintf(FwName,"%s_%s_%c.%c.%c",
                  "U585",
                  STBOX1_PACKAGENAME,
                  STBOX1_VERSION_MAJOR,
                  STBOX1_VERSION_MINOR,
                  STBOX1_VERSION_PATCH);
   *value = FwName;
  return 0;
}
uint8_t configuration_get_board_name(char **value)
{
  /* USER Code */
  static char PackageName[16];
  sprintf(PackageName,"%s",BLE_StackValue.BoardName);
  *value = PackageName;
  return 0;
}
uint8_t configuration_set_board_name(const char *value)
{
  /* USER Code */
  char PackageName[16];
  int NumberChar =  sprintf(PackageName,"%s",value);
  //Fill with spaces...
  for(;NumberChar<7;NumberChar++) {
    PackageName[NumberChar]=' ';
  }
  //Termination
  PackageName[7] ='\0';
  
  ExtConfigSetNameCommandCallback((uint8_t*) PackageName);
  return 0;
}

/* Device Information PnPL Component -----------------------------------------*/
uint8_t DeviceInformation_comp_init(void)
{

  /* USER Component initialization code */
  return 0;
}
char* DeviceInformation_get_key(void)
{
  return "DeviceInformation";
}

uint8_t DeviceInformation_get_manufacturer(char **value)
{
  /* USER Code */
  static char StringValue[24];
  sprintf(StringValue,"STMicroelectronics");
  *value = StringValue;
  return 0;
}
uint8_t DeviceInformation_get_model(char **value)
{
  /* USER Code */
  static char StringValue[24];
  sprintf(StringValue,"steval_stbox_pro");
  *value = StringValue;
  return 0;
}
uint8_t DeviceInformation_get_swVersion(char **value)
{
  /* USER Code */
  static char FwName[32];
  sprintf(FwName,"%s_%s_%c.%c.%c",
                "U585",
                STBOX1_PACKAGENAME,
                STBOX1_VERSION_MAJOR,
                STBOX1_VERSION_MINOR,
                STBOX1_VERSION_PATCH);
  *value = FwName;
  return 0;
}
uint8_t DeviceInformation_get_osName(char **value)
{
  /* USER Code */
  static char StringValue[8];
  sprintf(StringValue,"None");
  *value = StringValue;
  return 0;
}
uint8_t DeviceInformation_get_processorArchitecture(char **value)
{
  /* USER Code */
  static char StringValue[4];
  sprintf(StringValue,"ARM");
  *value = StringValue;
  return 0;
}
uint8_t DeviceInformation_get_processorManufacturer(char **value)
{
  /* USER Code */
  static char StringValue[4];
  sprintf(StringValue,"ARM");
  *value = StringValue;
  return 0;
}
uint8_t DeviceInformation_get_totalStorage(float *value)
{
  /* USER Code */
  *value =2097151;
  return 0;
}
uint8_t DeviceInformation_get_totalMemory(float *value)
{
  /* USER Code */
  *value=786431;
  return 0;
}

