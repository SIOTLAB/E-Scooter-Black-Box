/**
  ******************************************************************************
  * @file    BLESensorsPnPL\Src\BLE_Implementation.c
  * @author  System Research & Applications Team - Agrate/Catania Lab.
  * @version V1.6.0
  * @date    20-Oct-2023
  * @brief   BLE Implementation file
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
#include <stdio.h>
#include "STBOX1_config.h"
#include "BLE_Manager.h"
#include "OTA.h"
#include "main.h"
#include "SensorTileBoxPro.h"
#include "SensorTileBoxPro_env_sensors.h"
    
#include "PnPLCompManager.h"
#include "IControl.h"
#include "IControl_vtbl.h"
#include "Configuration_PnPL.h"
#include "Control_PnPL.h"
#include "Environmental_PnPL.h"
#include "Inertial_PnPL.h"
#include "Deviceinformation_PnPL.h"

/* Exported Variables --------------------------------------------------------*/
volatile uint8_t  connected   = FALSE;
volatile uint32_t RebootBoard = 0;
volatile uint32_t SwapBanks   = 0;
uint32_t ConnectionBleStatus =0;

uint32_t SizeOfUpdateBlueFW=0;

int32_t CurrentEnvUpdateEnumValue= 1; //1Hz
int32_t CurrentInerUpdateEnumValue= 10; //10Hz

uint8_t TimerEnvIsRunning=0;
uint8_t TimerInerIsRunning=0;

/* Imported Variables --------------------------------------------------------*/
extern TIM_HandleTypeDef TimCCHandle;
extern uint8_t BatteryPresent;

/* Private variables ---------------------------------------------------------*/
static uint32_t NeedToRebootBoard=0;
static uint32_t NeedToSwapBanks=0;

static IPnPLComponent_t *pConfigurationPnPLObj = NULL;
static IPnPLComponent_t *pControlPnPLObj = NULL;
static IPnPLComponent_t *pEnvironmentalPnPLObj = NULL;
static IPnPLComponent_t *pInertialPnPLObj = NULL;
static IPnPLComponent_t *pDeviceInformationPnPLObj = NULL;

static IControl_t iControl;

/* Private functions ---------------------------------------------------------*/
static uint32_t DebugConsoleParsing(uint8_t * att_data, uint8_t data_length);
static void ReadRequestEnvFunction(int32_t *Press,uint16_t *Hum,int16_t *Temp1,int16_t *Temp2);
static void DisconnectionCompletedFunction(void);
static void ConnectionCompletedFunction(uint16_t ConnectionHandle, uint8_t Address_Type, uint8_t Addr[6]);
static uint32_t DebugConsoleCommandParsing(uint8_t * att_data, uint8_t data_length);

/**********************************************************************************************
 * Callback functions prototypes to manage the extended configuration characteristic commands *
 **********************************************************************************************/
static void ExtExtConfigUidCommandCallback(uint8_t **UID);
static void ExtConfigInfoCommandCallback(uint8_t *Answer);
static void ExtConfigHelpCommandCallback(uint8_t *Answer);
static void ExtConfigVersionFwCommandCallback(uint8_t *Answer);
void ExtConfigSetNameCommandCallback(uint8_t *NewName);
static void ExtConfigReadBanksFwIdCommandCallback (uint8_t *CurBank,uint16_t *FwId1,uint16_t *FwId2);
static void ExtConfigBanksSwapCommandCallback(void);

static void NotifyEventBattery(BLE_NotifyEvent_t Event);
static void NotifyEventEnv(BLE_NotifyEvent_t Event);
static void NotifyEventInertial(BLE_NotifyEvent_t Event);
static void NotifyEventPnPLike(BLE_NotifyEvent_t Event);

static void WriteRequestPnPLike(uint8_t* received_msg, uint8_t msg_length);

/** @brief Initialize the BlueNRG stack and services
  * @param  None
  * @retval None
  */
void BluetoothInit(void)
{
  /* BlueNRG stack setting */
  BLE_StackValue.ConfigValueOffsets                   = CONFIG_DATA_PUBADDR_OFFSET;
  BLE_StackValue.ConfigValuelength                    = CONFIG_DATA_PUBADDR_LEN;
  BLE_StackValue.GAP_Roles                            = GAP_PERIPHERAL_ROLE;
  BLE_StackValue.IO_capabilities                      = IO_CAP_DISPLAY_ONLY;
  BLE_StackValue.AuthenticationRequirements           = BONDING;
  BLE_StackValue.MITM_ProtectionRequirements          = MITM_PROTECTION_REQUIRED;
  BLE_StackValue.SecureConnectionSupportOptionCode    = SC_IS_SUPPORTED;
  BLE_StackValue.SecureConnectionKeypressNotification = KEYPRESS_IS_NOT_SUPPORTED;
  
  /* Use BLE Random Address */
  BLE_StackValue.OwnAddressType = RANDOM_ADDR;
  
  /* Set the Board Name */
  {
    uint8_t *BoardName = ReadFlashBoardName();

    if(BoardName!=NULL) {
      /* If there is Saved Board Name */
      memcpy(BLE_StackValue.BoardName,BoardName,8);
    } else {
      /* Use the Default Board Name */
      sprintf(BLE_StackValue.BoardName,"%s",BLE_FW_PACKAGENAME);
    }
  }

  /* To set the TX power level of the bluetooth device ( -2 dBm )*/
  BLE_StackValue.EnableHighPowerMode= 0; /*  Low Power */
  
  /* Values: 0x00 ... 0x31 - The value depends on the device */
  BLE_StackValue.PowerAmplifierOutputLevel =25;
  
  /* BlueNRG services setting */
  BLE_StackValue.EnableConfig    = 0;
  BLE_StackValue.EnableConsole   = 1;
  BLE_StackValue.EnableExtConfig = 1;
  
  /* For Enabling the Secure Connection */
  BLE_StackValue.EnableSecureConnection=0;
  
  if(BLE_StackValue.EnableSecureConnection) {
    /* Using the Secure Connection, the Rescan should be done by BLE chip */    
    BLE_StackValue.ForceRescan =0;
  } else {
    BLE_StackValue.ForceRescan =1;
  }
  
  if(FinishGood==FINISHA) {
    BLE_StackValue.BoardId = BLE_MANAGER_SENSOR_TILE_BOX_PRO_PLATFORM;
  } else {
     BLE_StackValue.BoardId = BLE_MANAGER_SENSOR_TILE_BOX_PRO_B_PLATFORM;
  }
  
  InitBleManager();
  
  /* PnP-L Components Allocation */
  pConfigurationPnPLObj = Configuration_PnPLAlloc();
  pControlPnPLObj = Control_PnPLAlloc();
  pEnvironmentalPnPLObj = Environmental_PnPLAlloc();
  pInertialPnPLObj = Inertial_PnPLAlloc();
  pDeviceInformationPnPLObj = Deviceinformation_PnPLAlloc();
  
  /* Init&Add PnP-L Components */
  Configuration_PnPLInit(pConfigurationPnPLObj);
  Control_PnPLInit(pControlPnPLObj, &iControl);
  Environmental_PnPLInit(pEnvironmentalPnPLObj);
  Inertial_PnPLInit(pInertialPnPLObj);
  Deviceinformation_PnPLInit(pDeviceInformationPnPLObj);
  
  if(FinishGood==FINISHA) {
    PnPLSetBOARDID(BLE_MANAGER_SENSOR_TILE_BOX_PRO_PLATFORM);
    PnPLSetFWID(STBOX1A_BLUEST_SDK_FW_ID);
  } else {
    PnPLSetBOARDID(BLE_MANAGER_SENSOR_TILE_BOX_PRO_B_PLATFORM);
    PnPLSetFWID(STBOX1B_BLUEST_SDK_FW_ID);
  }
}

void RestartBLE_Manager(void) {
  HAL_NVIC_DisableIRQ(EXTI11_IRQn);
  
  /* Stop the TIM Base generation in interrupt mode for Led Blinking*/
  if(HAL_TIM_OC_Stop_IT(&TimCCHandle, TIM_CHANNEL_1) != HAL_OK){
    /* Stopping Error */
    Error_Handler(STBOX1_ERROR_TIMER,__FILE__,__LINE__);
  }

  BSP_LED_Off(LED_GREEN);
  
  ResetBleManager();
  BluetoothInit();
  
  /* Make the device connectable again. */
  set_connectable = TRUE;
}

/**
 * @brief  Enable BlueNRG-LP Interrupt.
 * @param  None
 * @retval None
 */
void InitBLEIntForBlueNRGLP(void)
{
  HAL_EXTI_GetHandle(&hexti11, EXTI_LINE_11);
  HAL_EXTI_RegisterCallback(&hexti11, HAL_EXTI_COMMON_CB_ID, hci_tl_lowlevel_isr);
  HAL_NVIC_SetPriority(EXTI11_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI11_IRQn);
}

/**
 * @brief  Custom Service Initialization.
 * @param  None
 * @retval None
 */
void BLE_InitCustomService(void) {
  /* Custom Function for Debug Console Command parsing */
  CustomDebugConsoleParsingCallback = DebugConsoleParsing;
  
  /* Custom Function for Connection Completed */
  CustomConnectionCompleted = ConnectionCompletedFunction;
  
  /* Custom Function for Disconnection Completed */
  CustomDisconnectionCompleted = DisconnectionCompletedFunction;
  
  /* Characteristc allocation for environmental features */
  /* Reading Environmental data Custom Function */
  CustomReadRequestEnv = ReadRequestEnvFunction;
  /* BLE_InitEnvService(PressEnable,HumEnable,NumTempEnabled) */
  if(BleManagerAddChar(BLE_InitEnvService(1, 0, 1))==0) {
    STBOX1_PRINTF("Error adding Environmental Service\r\n");
  }
  CustomNotifyEventEnv = NotifyEventEnv;
  
  /* Characteristc allocation for inertial features */
  /* BLE_InitInertialService(AccEnable,GyroEnable,MagEnabled) */
  if(BleManagerAddChar(BLE_InitInertialService(1,1,1))==0) {
    STBOX1_PRINTF("Error adding Inertial Service\r\n");
  }
  CustomNotifyEventInertial = NotifyEventInertial;
  
  if(BatteryPresent) {
    /* Characteristc allocation for Battery feature */
    if(BleManagerAddChar(BLE_InitBatteryService())==0)  {
      STBOX1_PRINTF("Error adding Battery Service\r\n");
    }
    CustomNotifyEventBattery =NotifyEventBattery;
  }
  
  /* Characteristic allocation for PnPLike feature */
  CustomWriteRequestPnPLike = WriteRequestPnPLike;
  if(BleManagerAddChar(BLE_InitPnPLikeService())==0) {
     STBOX1_PRINTF("Error adding PnPLike Service\r\n");
  }
  CustomNotifyEventPnPLike =NotifyEventPnPLike;
  
  /***********************************************************************************
  * Callback functions to manage the extended configuration characteristic commands *
  ***********************************************************************************/
  CustomExtConfigUidCommandCallback       = ExtExtConfigUidCommandCallback;
  CustomExtConfigInfoCommandCallback      = ExtConfigInfoCommandCallback;
  CustomExtConfigHelpCommandCallback      = ExtConfigHelpCommandCallback;
  CustomExtConfigVersionFwCommandCallback = ExtConfigVersionFwCommandCallback;
  CustomExtConfigSetNameCommandCallback   = ExtConfigSetNameCommandCallback;

  CustomExtConfigReadBanksFwIdCommandCallback       = ExtConfigReadBanksFwIdCommandCallback;
  {
    uint16_t FwId1,FwId2;
    
    ReadFlashBanksFwId(&FwId1,&FwId2);
    if(FwId2!=OTA_OTA_FW_ID_NOT_VALID) {
      /* Enable the Banks Swap only if there is a valid fw on second bank */
      CustomExtConfigBanksSwapCommandCallback           = ExtConfigBanksSwapCommandCallback;
    }
  }
}


/**
 * @brief  Callback called when there is a write on PnPLike feature
 * @param  uint8_t* received_msg received message
 * @param  uint8_t msg_length message length
 * @retval None
 */
static void WriteRequestPnPLike(uint8_t* received_msg, uint8_t msg_length)
{
  PnPLCommand_t PnPLCommand;
  STBOX1_PRINTF("PnPMessage Received\r\n");
  STBOX1_PRINTF("\t<%.*s>\r\n",msg_length,received_msg);
  
  PnPLParseCommand((char *)received_msg,&PnPLCommand);
  
  if(PnPLCommand.comm_type == PNPL_CMD_GET) {
    char *SerializedJSON;
    uint32_t size;
    
    PnPLSerializeResponse(&PnPLCommand,&SerializedJSON,&size,0);
    
    STBOX1_PRINTF("--> <%.*s>\r\n",size,SerializedJSON);
    
    PnPLikeEncapsulateAndSend((uint8_t*) SerializedJSON,size);
    free(SerializedJSON);
  }
}

/**
* @brief  Encapsulate and Send data to PnPLike Feature
* @param  uint8_t *data string to write
* @param  uint32_t lenght lengt of string to write
* @retval tBleStatus      Status
*/
tBleStatus PnPLikeEncapsulateAndSend(uint8_t *data,uint32_t length)
{
  uint32_t tot_len,j;
  uint8_t *JSON_string_command_wTP;
  uint32_t length_wTP;
  uint32_t len;
  
  if ((length % 19U) == 0U) {
    length_wTP = (length/19U)+length;
  } else {
    length_wTP = (length/19U)+1U+length;
  }
  
  JSON_string_command_wTP = malloc(sizeof(uint8_t) * length_wTP);
  
  if(JSON_string_command_wTP==NULL) {
    STBOX1_PRINTF("Error: Mem calloc error [%lu]: %d@%s\r\n",length,__LINE__,__FILE__);
    return BLE_STATUS_ERROR;
  } else {
    tot_len = BLE_Command_TP_Encapsulate(JSON_string_command_wTP, data, length,20);
    
    /* Data are sent as notifications*/
    j = 0;
    while (j < tot_len) {
      len = MIN(20U, (tot_len - j));
      if(BLE_PnPLikeUpdate(JSON_string_command_wTP+j,(uint8_t)len)!=(tBleStatus)BLE_STATUS_SUCCESS) {
        return BLE_STATUS_ERROR;
      }
      HAL_Delay(20);
      j += len;
    }
    free(JSON_string_command_wTP);
    return BLE_STATUS_SUCCESS;
  }
}



/**
 * @brief  Set Custom Advertise Data.
 * @param  uint8_t *manuf_data: Advertise Data
 * @retval None
 */
void BLE_SetCustomAdvertiseData(uint8_t *manuf_data)
{
  if(FinishGood==FINISHA) {
    manuf_data[BLE_MANAGER_CUSTOM_FIELD1]=STBOX1A_BLUEST_SDK_FW_ID;
  } else {
    manuf_data[BLE_MANAGER_CUSTOM_FIELD1]=STBOX1B_BLUEST_SDK_FW_ID;
  }
  manuf_data[BLE_MANAGER_CUSTOM_FIELD2]=CurrentActiveBank;
  manuf_data[BLE_MANAGER_CUSTOM_FIELD3]=0x00;//0xFF
  manuf_data[BLE_MANAGER_CUSTOM_FIELD4]=0x01;
}

/**
 * @brief  Callback Function for Un/Subscription Battery Feature
 * @param  BLE_NotifyEvent_t Event Sub/Unsub
 * @retval None
 */
static void NotifyEventBattery(BLE_NotifyEvent_t Event)
{
  if(Event == BLE_NOTIFY_SUB){
    uint32_t uhCapture = __HAL_TIM_GET_COUNTER(&TimCCHandle);
    
    W2ST_ON_CONNECTION(W2ST_CONNECT_BAT_EVENT);
    
    /* Start the TIM Base generation in interrupt mode */
    if(HAL_TIM_OC_Start_IT(&TimCCHandle, TIM_CHANNEL_1) != HAL_OK){
      /* Starting Error */
      Error_Handler(STBOX1_ERROR_TIMER,__FILE__,__LINE__);
    }
    
    /* Set the Capture Compare Register value */
    __HAL_TIM_SET_COMPARE(&TimCCHandle, TIM_CHANNEL_1, (uhCapture + STBOX1_UPDATE_LED_BATTERY));
    STBOX1_PRINTF("Start Battery\r\n");
  } else if(Event == BLE_NOTIFY_UNSUB) {
    W2ST_OFF_CONNECTION(W2ST_CONNECT_BAT_EVENT);
    
    /* Stop the TIM Base generation in interrupt mode */
    if(HAL_TIM_OC_Stop_IT(&TimCCHandle, TIM_CHANNEL_1) != HAL_OK){
      /* Stopping Error */
      Error_Handler(STBOX1_ERROR_TIMER,__FILE__,__LINE__);
    }
    STBOX1_PRINTF("Stop Battery\r\n");
  }
}

/**
 * @brief  Callback Function for Un/Subscription Environmental Feature
 * @param  BLE_NotifyEvent_t Event Sub/Unsub
 * @retval None
 */
static void NotifyEventEnv(BLE_NotifyEvent_t Event)
{
  if(Event == BLE_NOTIFY_SUB){
    
    if(TimerEnvIsRunning==0) {
      uint32_t uhCapture = __HAL_TIM_GET_COUNTER(&TimCCHandle);
      W2ST_ON_CONNECTION(W2ST_CONNECT_ENV);
      
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
    } else {
      STBOX1_PRINTF("Env Already Started\r\n");
    }
  } else if(Event == BLE_NOTIFY_UNSUB) {
    W2ST_OFF_CONNECTION(W2ST_CONNECT_ENV);
    if(TimerEnvIsRunning) {
      /* Stop the TIM Base generation in interrupt mode */
      if(HAL_TIM_OC_Stop_IT(&TimCCHandle, TIM_CHANNEL_2) != HAL_OK){
        /* Stopping Error */
        Error_Handler(STBOX1_ERROR_TIMER,__FILE__,__LINE__);
      }
      STBOX1_PRINTF("Stop Env\r\n");
      TimerEnvIsRunning=0;
    } else {
      STBOX1_PRINTF("Env Already Stopped\r\n");
    }
  }
}

/**
 * @brief  Callback Function for Un/Subscription PnPLike Feature
 * @param  BLE_NotifyEvent_t Event Sub/Unsub
 * @retval None
 */
static void NotifyEventPnPLike(BLE_NotifyEvent_t Event)
{
  if(Event == BLE_NOTIFY_SUB){
    W2ST_ON_CONNECTION(W2ST_CONNECT_PNPLIKE);
    STBOX1_PRINTF("PnPLike Subscribe\r\n");
  } else if(Event == BLE_NOTIFY_UNSUB) {
    W2ST_OFF_CONNECTION(W2ST_CONNECT_PNPLIKE);
    STBOX1_PRINTF("PnPLike Unsubscribe\r\n");
  }
}

/**
 * @brief  Callback Function for Un/Subscription Inertial Feature
 * @param  BLE_NotifyEvent_t Event Sub/Unsub
 * @retval None
 */
static void NotifyEventInertial(BLE_NotifyEvent_t Event)
{
  if(Event == BLE_NOTIFY_SUB){
    W2ST_ON_CONNECTION(W2ST_CONNECT_ACC_GYRO_MAG);
    if(TimerInerIsRunning==0) {
      uint32_t uhCapture = __HAL_TIM_GET_COUNTER(&TimCCHandle);
      
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
      TimerInerIsRunning=1;
    } else {
      STBOX1_PRINTF("Iner Already Started\r\n");
    }
  } else if(Event == BLE_NOTIFY_UNSUB) {
    W2ST_OFF_CONNECTION(W2ST_CONNECT_ACC_GYRO_MAG);
    
     if(TimerInerIsRunning) {
      /* Stop the TIM Base generation in interrupt mode */
      if(HAL_TIM_OC_Stop_IT(&TimCCHandle, TIM_CHANNEL_3) != HAL_OK){
        /* Stopping Error */
        Error_Handler(STBOX1_ERROR_TIMER,__FILE__,__LINE__);
      }
      STBOX1_PRINTF("Stop Iner\r\n");
      TimerInerIsRunning=0;
    } else {
      STBOX1_PRINTF("Iner Already Stopped\r\n");
    }
  }
}

/**
* @brief  This function makes the parsing of the Debug Console
* @param  uint8_t *att_data attribute data
* @param  uint8_t data_length length of the data
* @retval uint32_t SendBackData true/false
*/
static uint32_t DebugConsoleParsing(uint8_t * att_data, uint8_t data_length)
{
  /* By default Answer with the same message received */
  uint32_t SendBackData =1;
  
 if(SizeOfUpdateBlueFW!=0) {
    /* Firwmare update */
    int8_t RetValue = UpdateFWBlueMS(&SizeOfUpdateBlueFW,att_data, data_length,1);
    if(RetValue!=0) {
      Term_Update(((uint8_t *)&RetValue),1);
      if(RetValue==1) {
        /* if OTA checked */
        STBOX1_PRINTF("%s will restart after the disconnection\r\n",STBOX1_PACKAGENAME);
        HAL_Delay(1000);
        NeedToSwapBanks=1;
      }
    }
    SendBackData=0;
  } else {
    /* Received one write from Client on Terminal characteristc */
    SendBackData = DebugConsoleCommandParsing(att_data,data_length);
  }

  return SendBackData;
}

/**
 * @brief  This function makes the parsing of the Debug Console Commands
 * @param  uint8_t *att_data attribute data
 * @param  uint8_t data_length length of the data
 * @retval uint32_t SendBackData true/false
 */
static uint32_t DebugConsoleCommandParsing(uint8_t * att_data, uint8_t data_length)
{
  uint32_t SendBackData = 1;

  /* Help Command */
  if(!strncmp("help",(char *)(att_data),4)) {
    /* Print Legend */
    SendBackData=0;

    BytesToWrite =sprintf((char *)BufferToWrite,
                          "info\n");
    Term_Update(BufferToWrite,BytesToWrite);
  } else if(!strncmp("versionFw",(char *)(att_data),9)) {
    BytesToWrite =sprintf((char *)BufferToWrite,"%s_%s_%c.%c.%c\r\n",
                          "U585",
                          STBOX1_PACKAGENAME,
                          STBOX1_VERSION_MAJOR,
                          STBOX1_VERSION_MINOR,
                          STBOX1_VERSION_PATCH);
    Term_Update(BufferToWrite,BytesToWrite);
    SendBackData=0;
  } else if(!strncmp("info",(char *)(att_data),4)) {
    SendBackData=0;

    BytesToWrite =sprintf((char *)BufferToWrite,"\r\nSTMicroelectronics %s:\n"
       "\tVersion %c.%c.%c\n"
      "\tSTM32U585AI-SensorTile.box-Pro  (%c) board"
        "\n",
        STBOX1_PACKAGENAME,
        STBOX1_VERSION_MAJOR,STBOX1_VERSION_MINOR,STBOX1_VERSION_PATCH,
        (FinishGood==FINISHA) ? 'A' : 'B');
    Term_Update(BufferToWrite,BytesToWrite);

    BytesToWrite =sprintf((char *)BufferToWrite,"\t(HAL %ld.%ld.%ld_%ld)\n"
      "\tCompiled %s %s"
#if defined (__IAR_SYSTEMS_ICC__)
      " (IAR)\r\n",
#elif defined (__ARMCC_VERSION)
      " (KEIL)\r\n",
#elif defined (__GNUC__)
      " (STM32CubeIDE)\r\n",
#endif
        HAL_GetHalVersion() >>24,
        (HAL_GetHalVersion() >>16)&0xFF,
        (HAL_GetHalVersion() >> 8)&0xFF,
         HAL_GetHalVersion()      &0xFF,
         __DATE__,__TIME__);
    Term_Update(BufferToWrite,BytesToWrite);
    BytesToWrite =sprintf((char *)BufferToWrite,"Current Bank =%ld\n",CurrentActiveBank);
    Term_Update(BufferToWrite,BytesToWrite);
  } else if(!strncmp("upgradeFw",(char *)(att_data),9)) {
    uint32_t uwCRCValue;
    uint8_t *PointerByte = (uint8_t*) &SizeOfUpdateBlueFW;

    PointerByte[0]=att_data[ 9];
    PointerByte[1]=att_data[10];
    PointerByte[2]=att_data[11];
    PointerByte[3]=att_data[12];

    /* Check the Maximum Possible OTA size */
    if(SizeOfUpdateBlueFW>OTA_MAX_PROG_SIZE) {
      STBOX1_PRINTF("OTA %s SIZE=%ld > %d Max Allowed\r\n",STBOX1_PACKAGENAME,SizeOfUpdateBlueFW, OTA_MAX_PROG_SIZE);
      /* Answer with a wrong CRC value for signaling the problem to BlueMS application */
      BufferToWrite[0]= att_data[13];
      BufferToWrite[1]=(att_data[14]!=0) ? 0 : 1;/* In order to be sure to have a wrong CRC */
      BufferToWrite[2]= att_data[15];
      BufferToWrite[3]= att_data[16];
      BytesToWrite = 4;
      Term_Update(BufferToWrite,BytesToWrite);
    } else {
      PointerByte = (uint8_t*) &uwCRCValue;
      PointerByte[0]=att_data[13];
      PointerByte[1]=att_data[14];
      PointerByte[2]=att_data[15];
      PointerByte[3]=att_data[16];

      STBOX1_PRINTF("OTA %s SIZE=%ld uwCRCValue=%lx\r\n",STBOX1_PACKAGENAME,SizeOfUpdateBlueFW,uwCRCValue);

      /* Reset the Flash */
      StartUpdateFWBlueMS(SizeOfUpdateBlueFW,uwCRCValue);

#if 0
      /* Reduce the connection interval */
      {
         tBleStatus ret = aci_l2cap_connection_parameter_update_req(
                                                      ConnectionHandle,
                                                      6 /* interval_min*/,
                                                      6 /* interval_max */,
                                                      0   /* slave_latency */,
                                                      400 /*timeout_multiplier*/);
        /* Go to infinite loop if there is one error */
        if (ret != BLE_STATUS_SUCCESS) {
          while (1) {
            STBOX1_PRINTF("Problem Changing the connection interval\r\n");
          }
        }
      }
#endif

      /* Signal that we are ready sending back the CRV value*/
      BufferToWrite[0] = PointerByte[0];
      BufferToWrite[1] = PointerByte[1];
      BufferToWrite[2] = PointerByte[2];
      BufferToWrite[3] = PointerByte[3];
      BytesToWrite = 4;
      Term_Update(BufferToWrite,BytesToWrite);
    }

    SendBackData=0;
  }  else if(!strncmp("uid",(char *)(att_data),3)) {
    /* Write back the STM32 UID */
    uint8_t *uid = (uint8_t *)STM32_UUID;
    uint32_t MCU_ID = STM32_MCU_ID[0]&0xFFF;
    BytesToWrite =sprintf((char *)BufferToWrite,"%.2X%.2X%.2X%.2X%.2X%.2X%.2X%.2X%.2X%.2X%.2X%.2X_%.3lX\n",
                          uid[ 3],uid[ 2],uid[ 1],uid[ 0],
                          uid[ 7],uid[ 6],uid[ 5],uid[ 4],
                          uid[11],uid[ 10],uid[9],uid[8],
                          MCU_ID);
    Term_Update(BufferToWrite,BytesToWrite);
    SendBackData=0;
  }

  return SendBackData;
}

/**
 * @brief  This function is called when there is a Bluetooth Read request.
 * @param  int32_t *Press Pressure Value
 * @param  uint16_t *Hum Humidity Value
 * @param  int16_t *Temp1 Temperature Number 1
 * @param  int16_t *Temp2 Temperature Number 2
 * @retval None
 */
static void ReadRequestEnvFunction(int32_t *Press,uint16_t *Hum,int16_t *Temp1,int16_t *Temp2)
{
  float Temperature1,Pressure;
  BSP_ENV_SENSOR_GetValue(STTS22H_0, ENV_TEMPERATURE, &Temperature1);
  BSP_ENV_SENSOR_GetValue(LPS22DF_0, ENV_PRESSURE, &Pressure);

  *Press = (int32_t)(Pressure *100);
  *Temp1 = (int16_t)(Temperature1 * 10);
}


/**
 * @brief  This function is called when the peer device get disconnected.
 * @param  None 
 * @retval None
 */
static void DisconnectionCompletedFunction(void)
{
  connected = FALSE;

  /* Make the device connectable again */

  /* Reset for any problem during FOTA update */
  SizeOfUpdateBlueFW = 0;
  
  /*Stop all the timers */
  if(W2ST_CHECK_CONNECTION(W2ST_CONNECT_ACC_GYRO_MAG)) {
    if(TimerInerIsRunning) {
      if(HAL_TIM_OC_Stop_IT(&TimCCHandle, TIM_CHANNEL_3) != HAL_OK){
        /* Stopping Error */
        Error_Handler(STBOX1_ERROR_TIMER,__FILE__,__LINE__);
      }
      STBOX1_PRINTF("Stop Iner\r\n");
      TimerInerIsRunning=0;
    }
  }
  
  if(W2ST_CHECK_CONNECTION(W2ST_CONNECT_ENV)) {
    if(TimerEnvIsRunning) {
      if(HAL_TIM_OC_Stop_IT(&TimCCHandle, TIM_CHANNEL_2) != HAL_OK){
        /* Stopping Error */
        Error_Handler(STBOX1_ERROR_TIMER,__FILE__,__LINE__);
      }
      STBOX1_PRINTF("Stop Env\r\n");
      TimerEnvIsRunning=0;
    }
  }
  
  if(W2ST_CHECK_CONNECTION(W2ST_CONNECT_BAT_EVENT)) {
    if(HAL_TIM_OC_Stop_IT(&TimCCHandle, TIM_CHANNEL_1) != HAL_OK){
      /* Stopping Error */
      Error_Handler(STBOX1_ERROR_TIMER,__FILE__,__LINE__);
    }
    STBOX1_PRINTF("Stop Battery\r\n");
  }
  
  /* Reset the BLE Connection Variable */
  ConnectionBleStatus=0;
  
  if(NeedToRebootBoard) {
    NeedToRebootBoard=0;
    RebootBoard=1;
  }
  
   if(NeedToSwapBanks) {
    NeedToSwapBanks=0;
    SwapBanks=1;
  }
  
  HAL_Delay(200);
}

/**
 * @brief  This function is called when there is a LE Connection Complete event.
 * @param  None 
 * @retval None
 */
static void ConnectionCompletedFunction(uint16_t ConnectionHandle, uint8_t Address_Type,uint8_t Addr[6])
{
  connected = TRUE;
  ConnectionBleStatus=0;

  /* Stop the TIM Base generation in interrupt mode for Led Blinking*/
  if(HAL_TIM_OC_Stop_IT(&TimCCHandle, TIM_CHANNEL_1) != HAL_OK){
    /* Stopping Error */
    Error_Handler(STBOX1_ERROR_TIMER,__FILE__,__LINE__);
  }

  BSP_LED_Off(LED_GREEN);

  HAL_Delay(100);
}


/**
 * @brief  Enable Disable the jump to second flash bank and reboot board
 * @param  None
 * @retval None
 */
void EnableDisableDualBoot(void)
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


/***********************************************************************************
 * Callback functions to manage the extended configuration characteristic commands *
 ***********************************************************************************/

/**
 * @brief  Callback Function for answering to the UID command
 * @param  uint8_t **UID STM32 UID Return value
 * @retval None
 */
static void ExtExtConfigUidCommandCallback(uint8_t **UID)
{
  *UID = (uint8_t *)STM32_UUID;
}

/**
 * @brief  Callback Function for answering to Info command
 * @param  uint8_t *Answer Return String
 * @retval None
 */
static void ExtConfigInfoCommandCallback(uint8_t *Answer)
{
  sprintf((char *)Answer,"\r\nSTMicroelectronics %s:\n"
       "\tVersion %c.%c.%c\n"
      "\tSTM32U585AI-SensorTile.box-Pro (%c) board"
      "\n\t(HAL %ld.%ld.%ld_%ld)\n"
      "\tCompiled %s %s"
#if defined (__IAR_SYSTEMS_ICC__)
      " (IAR)\n"
#elif defined (__ARMCC_VERSION)
       " (KEIL)\n"
#elif defined (__GNUC__)
       " (STM32CubeIDE)\n"
#endif
       "\tCurrent Bank =%ld\n",
        STBOX1_PACKAGENAME,
        STBOX1_VERSION_MAJOR,STBOX1_VERSION_MINOR,STBOX1_VERSION_PATCH,
        (FinishGood==FINISHA) ? 'A' : 'B',
        HAL_GetHalVersion() >>24,
        (HAL_GetHalVersion() >>16)&0xFF,
        (HAL_GetHalVersion() >> 8)&0xFF,
         HAL_GetHalVersion()      &0xFF,
         __DATE__,__TIME__,
         CurrentActiveBank);
}

/**
 * @brief  Callback Function for answering to SetName command
 * @param  uint8_t *NewName New Name
 * @retval None
 */
void ExtConfigSetNameCommandCallback(uint8_t *NewName)
{
  STBOX1_PRINTF("Received a new Board's Name=%s\r\n",NewName);
  /* Update the Board's name in flash */
  if(FinishGood==FINISHA) {
    UpdateCurrFlashBankFwIdBoardName(STBOX1A_BLUEST_SDK_FW_ID,NewName);
  } else {
    UpdateCurrFlashBankFwIdBoardName(STBOX1B_BLUEST_SDK_FW_ID,NewName);
  }

  /* Update the Name for BLE Advertise */
  sprintf(BLE_StackValue.BoardName,"%s",NewName);
}  

/**
 * @brief  Callback Function for answering to Help command
 * @param  uint8_t *Answer Return String
 * @retval None
 */
static void ExtConfigHelpCommandCallback(uint8_t *Answer)
{
  sprintf((char *)Answer,"Help Message.....");
}
 
/**
 * @brief  Callback Function for answering to VersionFw command
 * @param  uint8_t *Answer Return String
 * @retval None
 */
static void ExtConfigVersionFwCommandCallback(uint8_t *Answer)
{
  sprintf((char *)Answer,"%s_%s_%c.%c.%c\r\n",
                  "U585",
                  STBOX1_PACKAGENAME,
                  STBOX1_VERSION_MAJOR,
                  STBOX1_VERSION_MINOR,
                  STBOX1_VERSION_PATCH);
}

/**
 * @brief  Callback Function for answering to ReadBanksFwId command
 * @param  uint8_t *CurBank Number Current Bank
 * @param  uint16_t *FwId1 Bank1 Firmware Id
 * @param  uint16_t *FwId2 Bank2 Firmware Id
 * @retval None
 */
static void ExtConfigReadBanksFwIdCommandCallback (uint8_t *CurBank,uint16_t *FwId1,uint16_t *FwId2)
{
  ReadFlashBanksFwId(FwId1,FwId2);
  *CurBank=CurrentActiveBank;
}

/**
 * @brief  Callback Function for answering to BanksSwap command
 * @param  None
 * @retval None
 */
static void ExtConfigBanksSwapCommandCallback(void)
{
  uint16_t FwId1,FwId2;
    
  ReadFlashBanksFwId(&FwId1,&FwId2);
  if(FwId2!=OTA_OTA_FW_ID_NOT_VALID) {
    STBOX1_PRINTF("Swapping to Bank%d\n",(CurrentActiveBank==1) ? 0 : 1);
    STBOX1_PRINTF("%s will restart after the disconnection\r\n",STBOX1_PACKAGENAME);
    NeedToSwapBanks = 1;
  } else {
    STBOX1_PRINTF("Not Valid fw on Bank%d\n\tCommand Rejected\n",(CurrentActiveBank==1) ? 0 : 1);
    STBOX1_PRINTF("\tLoad a Firwmare on Bank%d\n",(CurrentActiveBank==1) ? 0 : 1);
  }
}

