/**
  ******************************************************************************
  * @file    Inertial_PnPL.c
  * @author  SRA
  * @brief   Inertial PnPL Component Manager
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
  * dtmi:appconfig:steval_mkboxpro:FP_SNS_STBOX1_BLESensorPnPL:other:inertial:inertial;1
  *
  * Created by: DTDL2PnPL_cGen version 1.1.0
  *
  * WARNING! All changes made to this file will be lost if this is regenerated
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include <string.h>
#include "App_model.h"
#include "IPnPLComponent.h"
#include "IPnPLComponent_vtbl.h"
#include "PnPLCompManager.h"

#include "Inertial_PnPL.h"
#include "Inertial_PnPL_vtbl.h"

static const IPnPLComponent_vtbl sInertial_PnPL_CompIF_vtbl =
{
  Inertial_PnPL_vtblGetKey,
  Inertial_PnPL_vtblGetNCommands,
  Inertial_PnPL_vtblGetCommandKey,
  Inertial_PnPL_vtblGetStatus,
  Inertial_PnPL_vtblSetProperty,
  Inertial_PnPL_vtblExecuteFunction
};

/**
  *  Inertial_PnPL internal structure.
  */
struct _Inertial_PnPL
{
  /**
    * Implements the IPnPLComponent interface.
    */
  IPnPLComponent_t component_if;

};

/* Objects instance ----------------------------------------------------------*/
static Inertial_PnPL sInertial_PnPL;

/* Public API definition -----------------------------------------------------*/
IPnPLComponent_t *Inertial_PnPLAlloc()
{
  IPnPLComponent_t *pxObj = (IPnPLComponent_t *) &sInertial_PnPL;
  if (pxObj != NULL)
  {
    pxObj->vptr = &sInertial_PnPL_CompIF_vtbl;
  }
  return pxObj;
}

uint8_t Inertial_PnPLInit(IPnPLComponent_t *_this)
{
  IPnPLComponent_t *component_if = _this;
  PnPLAddComponent(component_if);
  inertial_comp_init();
  return 0;
}


/* IPnPLComponent virtual functions definition -------------------------------*/
char *Inertial_PnPL_vtblGetKey(IPnPLComponent_t *_this)
{
  return inertial_get_key();
}

uint8_t Inertial_PnPL_vtblGetNCommands(IPnPLComponent_t *_this)
{
  return 0;
}

char *Inertial_PnPL_vtblGetCommandKey(IPnPLComponent_t *_this, uint8_t id)
{
  return "";
}

uint8_t Inertial_PnPL_vtblGetStatus(IPnPLComponent_t *_this, char **serializedJSON, uint32_t *size, uint8_t pretty)
{
  JSON_Value *tempJSON;
  JSON_Object *JSON_Status;

  tempJSON = json_value_init_object();
  JSON_Status = json_value_get_object(tempJSON);

  float temp_f = 0;
  inertial_get_samplerate(&temp_f);
  uint8_t enum_id = 0;
  if(temp_f == n10)
  {
    enum_id = 0;
  }
  else if(temp_f == n20)
  {
    enum_id = 1;
  }
  else if(temp_f == n30)
  {
    enum_id = 2;
  }
  json_object_dotset_number(JSON_Status, "inertial.samplerate", enum_id);
  inertial_get_change_acc(&temp_f);
  enum_id = 0;
  if(temp_f ==  LSM6DSV16X)
  {
    enum_id = 0;
  }
  else if(temp_f == LIS2DU12)
  {
    enum_id = 1;
  }
  json_object_dotset_number(JSON_Status, "inertial.change_acc", enum_id);
  json_object_dotset_number(JSON_Status, "inertial.c_type", COMP_TYPE_OTHER);

  if (pretty == 1)
  {
    *serializedJSON = json_serialize_to_string_pretty(tempJSON);
    *size = json_serialization_size_pretty(tempJSON);
  }
  else
  {
    *serializedJSON = json_serialize_to_string(tempJSON);
    *size = json_serialization_size(tempJSON);
  }

  /* No need to free temp_j as it is part of tempJSON */
  json_value_free(tempJSON);

  return 0;
}

uint8_t Inertial_PnPL_vtblSetProperty(IPnPLComponent_t *_this, char *serializedJSON)
{
  JSON_Value *tempJSON = json_parse_string(serializedJSON);
  JSON_Object *tempJSONObject = json_value_get_object(tempJSON);

  uint8_t ret = 0;
  if(json_object_dothas_value(tempJSONObject, "inertial.samplerate"))
  {
    int samplerate = (int)json_object_dotget_number(tempJSONObject, "inertial.samplerate");
    switch(samplerate)
    {
    case 0:
      inertial_set_samplerate(n10);
      break;
    case 1:
      inertial_set_samplerate(n20);
      break;
    case 2:
      inertial_set_samplerate(n30);
      break;
    }
  }
  if(json_object_dothas_value(tempJSONObject, "inertial.change_acc"))
  {
    int change_acc = (int)json_object_dotget_number(tempJSONObject, "inertial.change_acc");
    switch(change_acc)
    {
    case 0:
      inertial_set_change_acc(LSM6DSV16X);
      break;
    case 1:
      inertial_set_change_acc(LIS2DU12);
      break;
    }
  }
  json_value_free(tempJSON);
  return ret;
}

uint8_t Inertial_PnPL_vtblExecuteFunction(IPnPLComponent_t *_this, char *serializedJSON)
{
  return 1;
}
