/**
  ******************************************************************************
  * @file    Configuration_PnPL.c
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

/* Includes ------------------------------------------------------------------*/
#include <string.h>
#include "App_model.h"
#include "IPnPLComponent.h"
#include "IPnPLComponent_vtbl.h"
#include "PnPLCompManager.h"

#include "Configuration_PnPL.h"
#include "Configuration_PnPL_vtbl.h"

static const IPnPLComponent_vtbl sConfiguration_PnPL_CompIF_vtbl =
{
  Configuration_PnPL_vtblGetKey,
  Configuration_PnPL_vtblGetNCommands,
  Configuration_PnPL_vtblGetCommandKey,
  Configuration_PnPL_vtblGetStatus,
  Configuration_PnPL_vtblSetProperty,
  Configuration_PnPL_vtblExecuteFunction
};

/**
  *  Configuration_PnPL internal structure.
  */
struct _Configuration_PnPL
{
  /**
    * Implements the IPnPLComponent interface.
    */
  IPnPLComponent_t component_if;

};

/* Objects instance ----------------------------------------------------------*/
static Configuration_PnPL sConfiguration_PnPL;

/* Public API definition -----------------------------------------------------*/
IPnPLComponent_t *Configuration_PnPLAlloc()
{
  IPnPLComponent_t *pxObj = (IPnPLComponent_t *) &sConfiguration_PnPL;
  if (pxObj != NULL)
  {
    pxObj->vptr = &sConfiguration_PnPL_CompIF_vtbl;
  }
  return pxObj;
}

uint8_t Configuration_PnPLInit(IPnPLComponent_t *_this)
{
  IPnPLComponent_t *component_if = _this;
  PnPLAddComponent(component_if);
  configuration_comp_init();
  return 0;
}


/* IPnPLComponent virtual functions definition -------------------------------*/
char *Configuration_PnPL_vtblGetKey(IPnPLComponent_t *_this)
{
  return configuration_get_key();
}

uint8_t Configuration_PnPL_vtblGetNCommands(IPnPLComponent_t *_this)
{
  return 0;
}

char *Configuration_PnPL_vtblGetCommandKey(IPnPLComponent_t *_this, uint8_t id)
{
  return "";
}

uint8_t Configuration_PnPL_vtblGetStatus(IPnPLComponent_t *_this, char **serializedJSON, uint32_t *size, uint8_t pretty)
{
  JSON_Value *tempJSON;
  JSON_Object *JSON_Status;

  tempJSON = json_value_init_object();
  JSON_Status = json_value_get_object(tempJSON);

  char *temp_s = "";
  configuration_get_version_fw(&temp_s);
  json_object_dotset_string(JSON_Status, "configuration.version_fw", temp_s);
  configuration_get_board_name(&temp_s);
  json_object_dotset_string(JSON_Status, "configuration.board_name", temp_s);
  json_object_dotset_number(JSON_Status, "configuration.c_type", COMP_TYPE_OTHER);

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

uint8_t Configuration_PnPL_vtblSetProperty(IPnPLComponent_t *_this, char *serializedJSON)
{
  JSON_Value *tempJSON = json_parse_string(serializedJSON);
  JSON_Object *tempJSONObject = json_value_get_object(tempJSON);

  uint8_t ret = 0;
  if (json_object_dothas_value(tempJSONObject, "configuration.board_name"))
  {
    const char *board_name = json_object_dotget_string(tempJSONObject, "configuration.board_name");
    configuration_set_board_name(board_name);
  }
  json_value_free(tempJSON);
  return ret;
}

uint8_t Configuration_PnPL_vtblExecuteFunction(IPnPLComponent_t *_this, char *serializedJSON)
{
  return 1;
}
