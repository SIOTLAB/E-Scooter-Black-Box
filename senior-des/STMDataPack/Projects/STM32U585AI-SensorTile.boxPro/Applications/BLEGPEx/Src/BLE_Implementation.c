/**
  ******************************************************************************
  * @file    BLEGPEx\Src\BLE_Implementation.c
  * @author  System Research & Applications Team - Agrate/Catania Lab.
  * @version V1.6.0
  * @date    20-Oct-2023
  * @brief  Implementation of API called from BLE Manager
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
/* Exported Variables --------------------------------------------------------*/
volatile uint8_t  connected   = FALSE;
volatile uint32_t RebootBoard = 0;
volatile uint32_t SwapBanks   = 0;
uint32_t ConnectionBleStatus =0;

uint32_t SizeOfUpdateBlueFW=0;

/* Imported Variables --------------------------------------------------------*/
extern TIM_HandleTypeDef TimCCHandle;
extern uint8_t BatteryPresent;

/* Private variables ---------------------------------------------------------*/
static uint32_t NeedToRebootBoard=0;
static uint32_t NeedToSwapBanks=0;

static uint16_t CustomCommandPageLevel=0;

/* Private functions ---------------------------------------------------------*/
static uint32_t DebugConsoleParsing(uint8_t * att_data, uint8_t data_length);
static void DisconnectionCompletedFunction(void);
static void ConnectionCompletedFunction(uint16_t ConnectionHandle, uint8_t Address_Type, uint8_t Addr[6]);
static uint32_t DebugConsoleCommandParsing(uint8_t * att_data, uint8_t data_length);

static void NotifyEventBattery(BLE_NotifyEvent_t Event);

static void NotifyEventGeneralPurpose(uint8_t GP_CharNum, BLE_NotifyEvent_t Event);

/**********************************************************************************************
 * Callback functions prototypes to manage the extended configuration characteristic commands *
 **********************************************************************************************/
static void ExtExtConfigUidCommandCallback(uint8_t **UID);
static void ExtConfigInfoCommandCallback(uint8_t *Answer);
static void ExtConfigHelpCommandCallback(uint8_t *Answer);
static void ExtConfigVersionFwCommandCallback(uint8_t *Answer);
static void ExtConfigSetNameCommandCallback(uint8_t *NewName);

static void ExtConfigReadBanksFwIdCommandCallback (uint8_t *CurBank,uint16_t *FwId1,uint16_t *FwId2);
static void ExtConfigBanksSwapCommandCallback(void);

static void ExtConfigCustomCommandCallback(BLE_CustomCommadResult_t *CustomCommand);
static void ExtConfigReadCustomCommandsCallback(JSON_Array *JSON_SensorArray);

static void ExtConfigReadSensorConfigCommandCallback(JSON_Array *JSON_SensorArray);
static void ExtConfigSetSensorConfigCommandCallback(uint8_t *configuration);

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
  
  if(BatteryPresent) {
    /* Characteristc allocation for Battery feature */
    if(BleManagerAddChar(BLE_InitBatteryService())==0)  {
      STBOX1_PRINTF("Error adding Battery Service\r\n");
    }
    CustomNotifyEventBattery =NotifyEventBattery;
  }
  
  /* Allocate 3 General Purpose Characteristics */
  if(BleManagerAddChar(BLE_InitGeneralPurposeService(2+2))==0)  {
    STBOX1_PRINTF("Error adding General Purpose Service 1\r\n");
  }
  if(BleManagerAddChar(BLE_InitGeneralPurposeService(2+4))==0)  {
    STBOX1_PRINTF("Error adding General Purpose Service 2\r\n");
  }
  if(BleManagerAddChar(BLE_InitGeneralPurposeService(2+1+4))==0)  {
    STBOX1_PRINTF("Error adding General Purpose Service 3\r\n");
  }
  CustomNotifyEventGeneralPurpose = NotifyEventGeneralPurpose;
  
  /***********************************************************************************
  * Callback functions to manage the extended configuration characteristic commands *
  ***********************************************************************************/
  CustomExtConfigUidCommandCallback       = ExtExtConfigUidCommandCallback;
  CustomExtConfigInfoCommandCallback      = ExtConfigInfoCommandCallback;
  CustomExtConfigHelpCommandCallback      = ExtConfigHelpCommandCallback;
  CustomExtConfigVersionFwCommandCallback = ExtConfigVersionFwCommandCallback;
  CustomExtConfigSetNameCommandCallback   = ExtConfigSetNameCommandCallback;

  /* FOTA and Dual Banks Section */
  CustomExtConfigReadBanksFwIdCommandCallback       = ExtConfigReadBanksFwIdCommandCallback;
  {
    uint16_t FwId1,FwId2;
    
    ReadFlashBanksFwId(&FwId1,&FwId2);
    if(FwId2!=OTA_OTA_FW_ID_NOT_VALID) {
      /* Enable the Banks Swap only if there is a valid fw on second bank */
      CustomExtConfigBanksSwapCommandCallback           = ExtConfigBanksSwapCommandCallback;
    }
  }
  
  /* Custom Command Section */
  CustomExtConfigCustomCommandCallback = ExtConfigCustomCommandCallback;
  CustomExtConfigReadCustomCommandsCallback = ExtConfigReadCustomCommandsCallback;
  
  /* Sensor Configuration */
  CustomExtConfigReadSensorsConfigCommandsCallback = ExtConfigReadSensorConfigCommandCallback;
  CustomExtConfigSetSensorsConfigCommandsCallback =  ExtConfigSetSensorConfigCommandCallback;
}

/**
 * @brief  Set Custom Advertise Data.
 * @param  uint8_t *manuf_data: Advertise Data
 * @retval None
 */
void BLE_SetCustomAdvertiseData(uint8_t *manuf_data)
{
  manuf_data[BLE_MANAGER_CUSTOM_FIELD1]=STBOX1_BLUEST_SDK_FW_ID;
  manuf_data[BLE_MANAGER_CUSTOM_FIELD2]=CurrentActiveBank;
  manuf_data[BLE_MANAGER_CUSTOM_FIELD3]=0x00; /* Not Used */
  manuf_data[BLE_MANAGER_CUSTOM_FIELD4]=0x00; /* Not Used */
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
 * @brief  Callback Function for Un/Subscription General Purpose Feature
 * @param uint8_t GP_CharNum General Purpouse Feature Number
 * @param  BLE_NotifyEvent_t Event Sub/Unsub
 * @retval None
 */
static void NotifyEventGeneralPurpose(uint8_t GP_CharNum, BLE_NotifyEvent_t Event)
{
  switch(GP_CharNum) {
    case 0:
      if(Event == BLE_NOTIFY_SUB){
        W2ST_ON_CONNECTION(W2ST_CONNECT_GP0);
      } else {
        W2ST_OFF_CONNECTION(W2ST_CONNECT_GP0);
      }
    break;
    case 1:
      if(Event == BLE_NOTIFY_SUB){
        W2ST_ON_CONNECTION(W2ST_CONNECT_GP1);
      } else {
        W2ST_OFF_CONNECTION(W2ST_CONNECT_GP1);
      }
    break;
    case 2:
      if(Event == BLE_NOTIFY_SUB){
        W2ST_ON_CONNECTION(W2ST_CONNECT_GP2);
      } else {
        W2ST_OFF_CONNECTION(W2ST_CONNECT_GP2);
      }
    break;
  }
  
  if(Event == BLE_NOTIFY_SUB){
    uint32_t uhCapture = __HAL_TIM_GET_COUNTER(&TimCCHandle);
    
    /* Start the TIM Base generation in interrupt mode */
    if(HAL_TIM_OC_Start_IT(&TimCCHandle, TIM_CHANNEL_2) != HAL_OK){
      /* Starting Error */
      Error_Handler(STBOX1_ERROR_TIMER,__FILE__,__LINE__);
    }
    
    /* Set the Capture Compare Register value */
    __HAL_TIM_SET_COMPARE(&TimCCHandle, TIM_CHANNEL_2, (uhCapture + STBOX1_UPDATE_GP));
    STBOX1_PRINTF("Start GP[%d]\r\n",GP_CharNum);
  } else if(Event == BLE_NOTIFY_UNSUB) {

    /* Stop the TIM Base generation in interrupt mode */
    if(HAL_TIM_OC_Stop_IT(&TimCCHandle, TIM_CHANNEL_2) != HAL_OK){
      /* Stopping Error */
      Error_Handler(STBOX1_ERROR_TIMER,__FILE__,__LINE__);
    }
    STBOX1_PRINTF("Stop GP[%d]\r\n",GP_CharNum);
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
  if((W2ST_CHECK_CONNECTION(W2ST_CONNECT_GP0)) |
     (W2ST_CHECK_CONNECTION(W2ST_CONNECT_GP1)) |
       (W2ST_CHECK_CONNECTION(W2ST_CONNECT_GP2))) {
    if(HAL_TIM_OC_Stop_IT(&TimCCHandle, TIM_CHANNEL_2) != HAL_OK){
      /* Stopping Error */
      Error_Handler(STBOX1_ERROR_TIMER,__FILE__,__LINE__);
    }
    STBOX1_PRINTF("Stop General Purpose\r\n");
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
  
  CustomCommandPageLevel=0;

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
static void ExtConfigSetNameCommandCallback(uint8_t *NewName)
{
  STBOX1_PRINTF("Received a new Board's Name=%s\r\n",NewName);
  /* Update the Board's name in flash */
  UpdateCurrFlashBankFwIdBoardName(STBOX1_BLUEST_SDK_FW_ID,NewName);

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

/**
 * @brief  Callback Function for managing the custom command
 * @param  BLE_CustomCommadResult_t *CustomCommand:
 * @param                            uint8_t *CommandName: Nome of the command
 * @param                            CustomCommand->CommandType: Type of the command
 * @param                            int32_t IntValue:    Integer or boolean parameter
 * @param                            uint8_t *StringValue: String parameter
 * @retval None
 */
static void  ExtConfigCustomCommandCallback(BLE_CustomCommadResult_t *CustomCommand)
{
  STBOX1_PRINTF("Received Custom Command:\r\n");
  STBOX1_PRINTF("\tCommand Name: <%s>\r\n", CustomCommand->CommandName);
  STBOX1_PRINTF("\tCommand Type: <%d>\r\n", CustomCommand->CommandType);
    
  switch(CustomCommand->CommandType) { 
    case BLE_CUSTOM_COMMAND_VOID:
     STBOX1_PRINTF("\tVoid\r\n");
    break;
    case BLE_CUSTOM_COMMAND_INTEGER:
      STBOX1_PRINTF("\tInt    Value: <%d>\r\n", CustomCommand->IntValue);
    break;
    case BLE_CUSTOM_COMMAND_ENUM_INTEGER:
      STBOX1_PRINTF("\tInt     Enum: <%d>\r\n", CustomCommand->IntValue);
    break;
    case BLE_CUSTOM_COMMAND_BOOLEAN:
      STBOX1_PRINTF("\tInt    Value: <%d>\r\n", CustomCommand->IntValue);
    break;
    case  BLE_CUSTOM_COMMAND_STRING:
      STBOX1_PRINTF("\tString Value: <%s>\r\n", CustomCommand->StringValue);
    break;
    case  BLE_CUSTOM_COMMAND_ENUM_STRING:
      STBOX1_PRINTF("\tString  Enum: <%s>\r\n", CustomCommand->StringValue);
    break;
  }
   if(!strncmp((char *)CustomCommand->CommandName,"ChangeCustomCommand",20)) {
     CustomCommandPageLevel=1;
     SendNewCustomCommandList();
   } else if(!strncmp((char *)CustomCommand->CommandName,"ComeBackCustomCommand",21)) {
     CustomCommandPageLevel=0;
     SendNewCustomCommandList();
   } else if(!strncmp((char *)CustomCommand->CommandName,"IntValue2",9)) {
     SendError("Example of Error");
   } else if(!strncmp((char *)CustomCommand->CommandName,"IntValue1",9)) {
     SendInfo("Example of Info");
   }
}

/**
 * @brief  Custom commands definition
 * @param  JSON_Array *JSON_SensorArray
 * @retval None
 */
static void ExtConfigReadCustomCommandsCallback(JSON_Array *JSON_SensorArray)
{
  /* Clear the previous Costom Command List */
  ClearCustomCommandsList();
  
  if(CustomCommandPageLevel==0) {
  
    /* Add all the custom Commands */
    if(AddCustomCommand("IntValue1", //Name
                        BLE_CUSTOM_COMMAND_INTEGER, //Type
                        100, //Default Value
                        -100, //MIN
                        200,  //MAX
                        NULL, //Enum Int
                        NULL, //Enum String
                        NULL, //Description
                        JSON_SensorArray)) {
      STBOX1_PRINTF("Added Command <%s>\r\n","IntValue1");
    } else {
       STBOX1_PRINTF("Error Adding Command <%s>\r\n","IntValue1");
       return;
    }
    
    if(AddCustomCommand("IntValue2", //Name
                        BLE_CUSTOM_COMMAND_INTEGER, //Type
                        BLE_MANAGER_CUSTOM_COMMAND_VALUE_NAN, //Default Value not important
                        10, //MIN
                        3000,  //MAX
                        NULL, //Enum Int
                        NULL, //Enum String
                        NULL, //Description
                        JSON_SensorArray)) {
      STBOX1_PRINTF("Added Command <%s>\r\n","IntValue2");
    } else {
       STBOX1_PRINTF("Error Adding Command <%s>\r\n","IntValue2");
       return;
    }

    if(AddCustomCommand("VoidCommand", //Name
                        BLE_CUSTOM_COMMAND_VOID, //Type
                        BLE_MANAGER_CUSTOM_COMMAND_VALUE_NAN, //Default Value
                        BLE_MANAGER_CUSTOM_COMMAND_VALUE_NAN, //MIN
                        BLE_MANAGER_CUSTOM_COMMAND_VALUE_NAN,  //MAX
                        NULL, //Enum Int
                        NULL, //Enum String
                        "Example Void Command", //Description
                        JSON_SensorArray)) {
      STBOX1_PRINTF("Added Command <%s>\r\n","Command1");
    } else {
       STBOX1_PRINTF("Error Adding Command <%s>\r\n","Command1");
       return;
    }
    
    if(AddCustomCommand("StringValue1", //Name
                        BLE_CUSTOM_COMMAND_STRING, //Type
                        BLE_MANAGER_CUSTOM_COMMAND_VALUE_NAN, //Default Value
                        BLE_MANAGER_CUSTOM_COMMAND_VALUE_NAN, //MIN
                        20,  //MAX
                        NULL, //Enum Int
                        NULL, //Enum String
                        NULL, //Description
                        JSON_SensorArray)) {
      STBOX1_PRINTF("Added Command <%s>\r\n","StringValue1");
    } else {
       STBOX1_PRINTF("Error Adding Command <%s>\r\n","StringValue1");
       return;
    }
    
    if(AddCustomCommand("BooleanValue", //Name
                        BLE_CUSTOM_COMMAND_BOOLEAN, //Type
                        1, //Default Value
                        BLE_MANAGER_CUSTOM_COMMAND_VALUE_NAN, //MIN
                        BLE_MANAGER_CUSTOM_COMMAND_VALUE_NAN,  //MAX
                        NULL, //Enum Int
                        NULL, //Enum String
                        "Example for Boolean", //Description
                        JSON_SensorArray)) {
      STBOX1_PRINTF("Added Command <%s>\r\n","BooleanValue");
    } else {
       STBOX1_PRINTF("Error Adding Command <%s>\r\n","BooleanValue");
       return;
    }
       
    if(AddCustomCommand("StringValue2", //Name
                        BLE_CUSTOM_COMMAND_STRING, //Type
                        BLE_MANAGER_CUSTOM_COMMAND_VALUE_NAN, //Default Value
                        4, //MIN
                        10,  //MAX
                        NULL, //Enum Int
                        NULL, //Enum String
                        "It's possible to add a  very very very very very very long description", //Description
                        JSON_SensorArray)) {
      STBOX1_PRINTF("Added Command <%s>\r\n","StringValue2");
    } else {
      STBOX1_PRINTF("Error Adding Command <%s>\r\n","StringValue2");
      return;
    }
    
    //Example of Enum String Custom Command
    {
      //The Last value should be NULL
      char *ValidStringValues[]={"Ciao", "Buona","Giornata",NULL};
      if(AddCustomCommand("StringEnum", //Name
                          BLE_CUSTOM_COMMAND_ENUM_STRING, //Type
                          1, //Default Value
                          BLE_MANAGER_CUSTOM_COMMAND_VALUE_NAN, //MIN
                          BLE_MANAGER_CUSTOM_COMMAND_VALUE_NAN,  //MAX
                          NULL, //Enum Int
                          (void *)ValidStringValues, //Enum String
                          "Example of Enum String", //Description
                          JSON_SensorArray)) {
        STBOX1_PRINTF("Added Command <%s>\r\n","StringEnum");
      } else {
        STBOX1_PRINTF("Error Adding Command <%s>\r\n","StringEnum");
        return;
      }
    } 
    
    //Example of Enum Int Custom Command
    {
      //The Last value should be BLE_MANAGER_CUSTOM_COMMAND_VALUE_NAN
      int32_t ValidIntValues[]={-1,12,123,321,BLE_MANAGER_CUSTOM_COMMAND_VALUE_NAN};
      if(AddCustomCommand("IntEnum", //Name
                          BLE_CUSTOM_COMMAND_ENUM_INTEGER, //Type
                          3, //Default Value
                          BLE_MANAGER_CUSTOM_COMMAND_VALUE_NAN, //MIN
                          BLE_MANAGER_CUSTOM_COMMAND_VALUE_NAN,  //MAX
                          (void *) ValidIntValues, //Enum Int
                          NULL, //Enum String
                          "Example of Enum Integer", //Description
                          JSON_SensorArray)) {
        STBOX1_PRINTF("Added Command <%s>\r\n","IntEnum");
      } else {
        STBOX1_PRINTF("Error Adding Command <%s>\r\n","IntEnum");
        return;
      }
    }
    
     if(AddCustomCommand("ChangeCustomCommand", //Name
                        BLE_CUSTOM_COMMAND_VOID, //Type
                        BLE_MANAGER_CUSTOM_COMMAND_VALUE_NAN, //Default Value
                        BLE_MANAGER_CUSTOM_COMMAND_VALUE_NAN, //MIN
                        BLE_MANAGER_CUSTOM_COMMAND_VALUE_NAN,  //MAX
                        NULL, //Enum Int
                        NULL, //Enum String
                        "Change the Custom Commands", //Description
                        JSON_SensorArray)) {
      STBOX1_PRINTF("Added Command <%s>\r\n","Command1");
    } else {
       STBOX1_PRINTF("Error Adding Command <%s>\r\n","Command1");
       return;
    }
    
    //Just one Example of one Invalid Command
    STBOX1_PRINTF("Example of trying to add one Invalid Custom Command\r\n");
    if(AddCustomCommand("ReadCert", //Name
                        BLE_CUSTOM_COMMAND_STRING, //Type
                        BLE_MANAGER_CUSTOM_COMMAND_VALUE_NAN, //Default Value
                        4, //MIN
                        10,  //MAX
                        NULL, //Enum Int
                        NULL, //Enum String
                        "Invalid Command...", //Description
                        JSON_SensorArray)) {
      STBOX1_PRINTF("Added Command <%s>\r\n","ReadCert");
    } else {
      STBOX1_PRINTF("Error Adding Command <%s>\r\n","ReadCert");
      return;//not mandatory... it's the last one
    }
  } else if(CustomCommandPageLevel==1) {
     if(AddCustomCommand("ComeBackCustomCommand", //Name
                        BLE_CUSTOM_COMMAND_VOID, //Type
                        BLE_MANAGER_CUSTOM_COMMAND_VALUE_NAN, //Default Value
                        BLE_MANAGER_CUSTOM_COMMAND_VALUE_NAN, //MIN
                        BLE_MANAGER_CUSTOM_COMMAND_VALUE_NAN,  //MAX
                        NULL, //Enum Int
                        NULL, //Enum String
                        "Come back to previous Custom Commands", //Description
                        JSON_SensorArray)) {
      STBOX1_PRINTF("Added Command <%s>\r\n","Command1");
    } else {
       STBOX1_PRINTF("Error Adding Command <%s>\r\n","Command1");
       return;
    }
    
    if(AddCustomCommand("StringValueNewLevel", //Name
                        BLE_CUSTOM_COMMAND_STRING, //Type
                        BLE_MANAGER_CUSTOM_COMMAND_VALUE_NAN, //Default Value
                        BLE_MANAGER_CUSTOM_COMMAND_VALUE_NAN, //MIN
                        20,  //MAX
                        NULL, //Enum Int
                        NULL, //Enum String
                        NULL, //Description
                        JSON_SensorArray)) {
      STBOX1_PRINTF("Added Command <%s>\r\n","StringValueNewLevel");
    } else {
       STBOX1_PRINTF("Error Adding Command <%s>\r\n","StringValue1");
       return;
    }
  }
}


/**
 * @brief  Reads Sensor Config
 * @param  JSON_Array *JSON_SensorArray
 * @retval None
 */
static void ExtConfigReadSensorConfigCommandCallback(JSON_Array *JSON_SensorArray)
{
  
#define WRITE_BUFFER_SIZE_LPS22DF_P       (uint32_t)(1024)
#define WRITE_BUFFER_SIZE_LPS22DF_T       (uint32_t)(1024)
#define WRITE_BUFFER_SIZE_LSM6DSV16X_A   (uint32_t)(16384)
#define WRITE_BUFFER_SIZE_LSM6DSV16X_G   (uint32_t)(16384)
#define WRITE_BUFFER_SIZE_LSM6DSV16X_MLC (uint32_t)(1024)
  
  COM_Sensor_t SensorLPS22DF,SensorLSM6DSV16X;
  JSON_Value *tempJSON1;
  STBOX1_PRINTF("Received the Read Sensors Config Command\r\n");
  
  /* LPS22DF SENSOR DESCRIPTOR */
  strcpy(SensorLPS22DF.sensorDescriptor.name, "LPS22DF");
  SensorLPS22DF.sensorDescriptor.nSubSensors = 2;
  SensorLPS22DF.sensorDescriptor.id=0;
  
  /* SUBSENSOR 0 DESCRIPTOR */
  SensorLPS22DF.sensorDescriptor.subSensorDescriptor[0].id = 0;
  SensorLPS22DF.sensorDescriptor.subSensorDescriptor[0].sensorType = COM_TYPE_PRESS;
  SensorLPS22DF.sensorDescriptor.subSensorDescriptor[0].dimensions = 1;
  strcpy(SensorLPS22DF.sensorDescriptor.subSensorDescriptor[0].dimensionsLabel[0], "prs");
  SensorLPS22DF.sensorDescriptor.subSensorDescriptor[0].dataType = DATA_TYPE_FLOAT;
  SensorLPS22DF.sensorDescriptor.subSensorDescriptor[0].ODR[0] = 1.0f;
  SensorLPS22DF.sensorDescriptor.subSensorDescriptor[0].ODR[1] = 4.0f;
  SensorLPS22DF.sensorDescriptor.subSensorDescriptor[0].ODR[2] = 10.0f;
  SensorLPS22DF.sensorDescriptor.subSensorDescriptor[0].ODR[3] = 25.0f;
  SensorLPS22DF.sensorDescriptor.subSensorDescriptor[0].ODR[4] = 50.0f;
  SensorLPS22DF.sensorDescriptor.subSensorDescriptor[0].ODR[5] = 75.0f;
  SensorLPS22DF.sensorDescriptor.subSensorDescriptor[0].ODR[6] = 100.0f;
  SensorLPS22DF.sensorDescriptor.subSensorDescriptor[0].ODR[7] = COM_END_OF_LIST_FLOAT;
  SensorLPS22DF.sensorDescriptor.subSensorDescriptor[0].samplesPerTimestamp[0] = 0;
  SensorLPS22DF.sensorDescriptor.subSensorDescriptor[0].samplesPerTimestamp[1] = 1000;
  strcpy(SensorLPS22DF.sensorDescriptor.subSensorDescriptor[0].unit, "hPa");
  SensorLPS22DF.sensorDescriptor.subSensorDescriptor[0].FS[0] = 1260.0f;
  SensorLPS22DF.sensorDescriptor.subSensorDescriptor[0].FS[1] = COM_END_OF_LIST_FLOAT;
  
  /* SUBSENSOR 0 STATUS */
  SensorLPS22DF.sensorStatus.subSensorStatus[0].isActive = 1;
  SensorLPS22DF.sensorStatus.subSensorStatus[0].FS = 1260.0f;
  SensorLPS22DF.sensorStatus.subSensorStatus[0].sensitivity = 1.0f;
  SensorLPS22DF.sensorStatus.subSensorStatus[0].ODR = 100.0f;
  SensorLPS22DF.sensorStatus.subSensorStatus[0].measuredODR = 0.0f;
  SensorLPS22DF.sensorStatus.subSensorStatus[0].initialOffset = 0.0f;
  SensorLPS22DF.sensorStatus.subSensorStatus[0].samplesPerTimestamp = 100;
  SensorLPS22DF.sensorStatus.subSensorStatus[0].usbDataPacketSize = 1600;
  SensorLPS22DF.sensorStatus.subSensorStatus[0].sdWriteBufferSize = WRITE_BUFFER_SIZE_LPS22DF_P;
  SensorLPS22DF.sensorStatus.subSensorStatus[0].comChannelNumber = -1;
  SensorLPS22DF.sensorStatus.subSensorStatus[0].ucfLoaded = 0;
  
  /* SUBSENSOR 1 DESCRIPTOR */
  SensorLPS22DF.sensorDescriptor.subSensorDescriptor[1].id = 1;
  SensorLPS22DF.sensorDescriptor.subSensorDescriptor[1].sensorType = COM_TYPE_TEMP;
  SensorLPS22DF.sensorDescriptor.subSensorDescriptor[1].dimensions = 1;
  strcpy(SensorLPS22DF.sensorDescriptor.subSensorDescriptor[1].dimensionsLabel[0], "tem");
  SensorLPS22DF.sensorDescriptor.subSensorDescriptor[1].dataType = DATA_TYPE_FLOAT;
  SensorLPS22DF.sensorDescriptor.subSensorDescriptor[1].ODR[0] = 1.0f;
  SensorLPS22DF.sensorDescriptor.subSensorDescriptor[1].ODR[1] = 4.0f;
  SensorLPS22DF.sensorDescriptor.subSensorDescriptor[1].ODR[2] = 10.0f;
  SensorLPS22DF.sensorDescriptor.subSensorDescriptor[1].ODR[3] = 25.0f;
  SensorLPS22DF.sensorDescriptor.subSensorDescriptor[1].ODR[4] = 50.0f;
  SensorLPS22DF.sensorDescriptor.subSensorDescriptor[1].ODR[5] = 75.0f;
  SensorLPS22DF.sensorDescriptor.subSensorDescriptor[1].ODR[6] = 100.0f;
  SensorLPS22DF.sensorDescriptor.subSensorDescriptor[1].ODR[7] = COM_END_OF_LIST_FLOAT;
  SensorLPS22DF.sensorDescriptor.subSensorDescriptor[1].samplesPerTimestamp[0] = 0;
  SensorLPS22DF.sensorDescriptor.subSensorDescriptor[1].samplesPerTimestamp[1] = 1000;
  strcpy(SensorLPS22DF.sensorDescriptor.subSensorDescriptor[1].unit, "Celsius");
  SensorLPS22DF.sensorDescriptor.subSensorDescriptor[1].FS[0] = 85.0f;
  SensorLPS22DF.sensorDescriptor.subSensorDescriptor[1].FS[1] = COM_END_OF_LIST_FLOAT;
  
  /* SUBSENSOR 1 STATUS */
  SensorLPS22DF.sensorStatus.subSensorStatus[1].isActive = 1;
  SensorLPS22DF.sensorStatus.subSensorStatus[1].FS = 85.0f;
  SensorLPS22DF.sensorStatus.subSensorStatus[1].sensitivity = 1.0f;
  SensorLPS22DF.sensorStatus.subSensorStatus[1].ODR = 100.0f;
  SensorLPS22DF.sensorStatus.subSensorStatus[1].measuredODR = 0.0f;
  SensorLPS22DF.sensorStatus.subSensorStatus[1].initialOffset = 0.0f;
  SensorLPS22DF.sensorStatus.subSensorStatus[1].samplesPerTimestamp = 100;
  SensorLPS22DF.sensorStatus.subSensorStatus[1].usbDataPacketSize = 1600;
  SensorLPS22DF.sensorStatus.subSensorStatus[1].sdWriteBufferSize = WRITE_BUFFER_SIZE_LPS22DF_T;
  SensorLPS22DF.sensorStatus.subSensorStatus[1].comChannelNumber = -1;
  SensorLPS22DF.sensorStatus.subSensorStatus[1].ucfLoaded = 0;
  
  
  /* SENSOR DESCRIPTOR */
  strcpy(SensorLSM6DSV16X.sensorDescriptor.name, "LSM6DSV16X");
  SensorLSM6DSV16X.sensorDescriptor.nSubSensors = 3;
  SensorLSM6DSV16X.sensorDescriptor.id=1;

  /* SUBSENSOR 0 DESCRIPTOR */
  SensorLSM6DSV16X.sensorDescriptor.subSensorDescriptor[0].id = 0;
  SensorLSM6DSV16X.sensorDescriptor.subSensorDescriptor[0].sensorType = COM_TYPE_ACC;
  SensorLSM6DSV16X.sensorDescriptor.subSensorDescriptor[0].dimensions = 3;
  strcpy(SensorLSM6DSV16X.sensorDescriptor.subSensorDescriptor[0].dimensionsLabel[0], "x");
  strcpy(SensorLSM6DSV16X.sensorDescriptor.subSensorDescriptor[0].dimensionsLabel[1], "y");
  strcpy(SensorLSM6DSV16X.sensorDescriptor.subSensorDescriptor[0].dimensionsLabel[2], "z");
  SensorLSM6DSV16X.sensorDescriptor.subSensorDescriptor[0].dataType = DATA_TYPE_INT16;
  SensorLSM6DSV16X.sensorDescriptor.subSensorDescriptor[0].ODR[0] = 1.875f;
  SensorLSM6DSV16X.sensorDescriptor.subSensorDescriptor[0].ODR[1] = 7.5f;
  SensorLSM6DSV16X.sensorDescriptor.subSensorDescriptor[0].ODR[2] = 15.0f;
  SensorLSM6DSV16X.sensorDescriptor.subSensorDescriptor[0].ODR[3] = 30.0f;
  SensorLSM6DSV16X.sensorDescriptor.subSensorDescriptor[0].ODR[4] = 60.0f;
  SensorLSM6DSV16X.sensorDescriptor.subSensorDescriptor[0].ODR[5] = 120.0f;
  SensorLSM6DSV16X.sensorDescriptor.subSensorDescriptor[0].ODR[6] = 240.0f;
  SensorLSM6DSV16X.sensorDescriptor.subSensorDescriptor[0].ODR[7] = 480.0f;
  SensorLSM6DSV16X.sensorDescriptor.subSensorDescriptor[0].ODR[8] = 960.0f;
  SensorLSM6DSV16X.sensorDescriptor.subSensorDescriptor[0].ODR[9] = 1920.0f;
  SensorLSM6DSV16X.sensorDescriptor.subSensorDescriptor[0].ODR[10] = 3840.0f;
  SensorLSM6DSV16X.sensorDescriptor.subSensorDescriptor[0].ODR[11] = COM_END_OF_LIST_FLOAT;
  SensorLSM6DSV16X.sensorDescriptor.subSensorDescriptor[0].samplesPerTimestamp[0] = 0;
  SensorLSM6DSV16X.sensorDescriptor.subSensorDescriptor[0].samplesPerTimestamp[1] = 1000;
  strcpy(SensorLSM6DSV16X.sensorDescriptor.subSensorDescriptor[0].unit, "g");
  SensorLSM6DSV16X.sensorDescriptor.subSensorDescriptor[0].FS[0] = 2.0f;
  SensorLSM6DSV16X.sensorDescriptor.subSensorDescriptor[0].FS[1] = 4.0f;
  SensorLSM6DSV16X.sensorDescriptor.subSensorDescriptor[0].FS[2] = 8.0f;
  SensorLSM6DSV16X.sensorDescriptor.subSensorDescriptor[0].FS[3] = 16.0f;
  SensorLSM6DSV16X.sensorDescriptor.subSensorDescriptor[0].FS[4] = COM_END_OF_LIST_FLOAT;

  /* SUBSENSOR 0 STATUS */
  SensorLSM6DSV16X.sensorStatus.subSensorStatus[0].isActive = 1;
  SensorLSM6DSV16X.sensorStatus.subSensorStatus[0].FS = 16.0f;
  SensorLSM6DSV16X.sensorStatus.subSensorStatus[0].sensitivity = 0.0000305f * SensorLSM6DSV16X.sensorStatus.subSensorStatus[0].FS;
  SensorLSM6DSV16X.sensorStatus.subSensorStatus[0].ODR = 1920.0f;
  SensorLSM6DSV16X.sensorStatus.subSensorStatus[0].measuredODR = 0.0f;
  SensorLSM6DSV16X.sensorStatus.subSensorStatus[0].initialOffset = 0.0f;
  SensorLSM6DSV16X.sensorStatus.subSensorStatus[0].samplesPerTimestamp = 1000;
  SensorLSM6DSV16X.sensorStatus.subSensorStatus[0].usbDataPacketSize = 2048;
  SensorLSM6DSV16X.sensorStatus.subSensorStatus[0].sdWriteBufferSize = WRITE_BUFFER_SIZE_LSM6DSV16X_A;
  SensorLSM6DSV16X.sensorStatus.subSensorStatus[0].comChannelNumber = -1;
  SensorLSM6DSV16X.sensorStatus.subSensorStatus[0].ucfLoaded = 0;

    /* SUBSENSOR 1 DESCRIPTOR */
  SensorLSM6DSV16X.sensorDescriptor.subSensorDescriptor[1].id = 1;
  SensorLSM6DSV16X.sensorDescriptor.subSensorDescriptor[1].sensorType = COM_TYPE_GYRO;
  SensorLSM6DSV16X.sensorDescriptor.subSensorDescriptor[1].dimensions = 3;
  strcpy(SensorLSM6DSV16X.sensorDescriptor.subSensorDescriptor[1].dimensionsLabel[0], "x");
  strcpy(SensorLSM6DSV16X.sensorDescriptor.subSensorDescriptor[1].dimensionsLabel[1], "y");
  strcpy(SensorLSM6DSV16X.sensorDescriptor.subSensorDescriptor[1].dimensionsLabel[2], "z");
  SensorLSM6DSV16X.sensorDescriptor.subSensorDescriptor[1].dataType = DATA_TYPE_INT16;
  SensorLSM6DSV16X.sensorDescriptor.subSensorDescriptor[1].ODR[0] = 7.5f;
  SensorLSM6DSV16X.sensorDescriptor.subSensorDescriptor[1].ODR[1] = 15.0f;
  SensorLSM6DSV16X.sensorDescriptor.subSensorDescriptor[1].ODR[2] = 30.0f;
  SensorLSM6DSV16X.sensorDescriptor.subSensorDescriptor[1].ODR[3] = 60.0f;
  SensorLSM6DSV16X.sensorDescriptor.subSensorDescriptor[1].ODR[4] = 120.0f;
  SensorLSM6DSV16X.sensorDescriptor.subSensorDescriptor[1].ODR[5] = 240.0f;
  SensorLSM6DSV16X.sensorDescriptor.subSensorDescriptor[1].ODR[6] = 480.0f;
  SensorLSM6DSV16X.sensorDescriptor.subSensorDescriptor[1].ODR[7] = 960.0f;
  SensorLSM6DSV16X.sensorDescriptor.subSensorDescriptor[1].ODR[8] = 1920.0f;
  SensorLSM6DSV16X.sensorDescriptor.subSensorDescriptor[1].ODR[9] = 3840.0f;
  SensorLSM6DSV16X.sensorDescriptor.subSensorDescriptor[1].ODR[10] = COM_END_OF_LIST_FLOAT;
  SensorLSM6DSV16X.sensorDescriptor.subSensorDescriptor[1].samplesPerTimestamp[0] = 0;
  SensorLSM6DSV16X.sensorDescriptor.subSensorDescriptor[1].samplesPerTimestamp[1] = 1000;
  strcpy(SensorLSM6DSV16X.sensorDescriptor.subSensorDescriptor[1].unit, "mdps");
  SensorLSM6DSV16X.sensorDescriptor.subSensorDescriptor[1].FS[0] = 125.0f;
  SensorLSM6DSV16X.sensorDescriptor.subSensorDescriptor[1].FS[1] = 250.0f;
  SensorLSM6DSV16X.sensorDescriptor.subSensorDescriptor[1].FS[2] = 500.0f;
  SensorLSM6DSV16X.sensorDescriptor.subSensorDescriptor[1].FS[3] = 1000.0f;
  SensorLSM6DSV16X.sensorDescriptor.subSensorDescriptor[1].FS[4] = 2000.0f;
  SensorLSM6DSV16X.sensorDescriptor.subSensorDescriptor[1].FS[5] = COM_END_OF_LIST_FLOAT;

  /* SUBSENSOR 1 STATUS */
  SensorLSM6DSV16X.sensorStatus.subSensorStatus[1].isActive = 1;
  SensorLSM6DSV16X.sensorStatus.subSensorStatus[1].FS = 2000.0f;
  SensorLSM6DSV16X.sensorStatus.subSensorStatus[1].sensitivity = 0.035f * SensorLSM6DSV16X.sensorStatus.subSensorStatus[1].FS;
  SensorLSM6DSV16X.sensorStatus.subSensorStatus[1].ODR = 3840.0f;
  SensorLSM6DSV16X.sensorStatus.subSensorStatus[1].measuredODR = 0.0f;
  SensorLSM6DSV16X.sensorStatus.subSensorStatus[1].initialOffset = 0.0f;
  SensorLSM6DSV16X.sensorStatus.subSensorStatus[1].samplesPerTimestamp = 1000;
  SensorLSM6DSV16X.sensorStatus.subSensorStatus[1].usbDataPacketSize = 2048;
  SensorLSM6DSV16X.sensorStatus.subSensorStatus[1].sdWriteBufferSize = WRITE_BUFFER_SIZE_LSM6DSV16X_G;
  SensorLSM6DSV16X.sensorStatus.subSensorStatus[1].comChannelNumber = -1;
  SensorLSM6DSV16X.sensorStatus.subSensorStatus[1].ucfLoaded = 0;

    /* SUBSENSOR 2 DESCRIPTOR */
  SensorLSM6DSV16X.sensorDescriptor.subSensorDescriptor[2].id = 2;
  SensorLSM6DSV16X.sensorDescriptor.subSensorDescriptor[2].sensorType = COM_TYPE_MLC;
  SensorLSM6DSV16X.sensorDescriptor.subSensorDescriptor[2].dimensions = 4;
  strcpy(SensorLSM6DSV16X.sensorDescriptor.subSensorDescriptor[2].dimensionsLabel[0], "1");
  strcpy(SensorLSM6DSV16X.sensorDescriptor.subSensorDescriptor[2].dimensionsLabel[1], "2");
  strcpy(SensorLSM6DSV16X.sensorDescriptor.subSensorDescriptor[2].dimensionsLabel[2], "3");
  strcpy(SensorLSM6DSV16X.sensorDescriptor.subSensorDescriptor[2].dimensionsLabel[3], "4");
  SensorLSM6DSV16X.sensorDescriptor.subSensorDescriptor[2].dataType = DATA_TYPE_INT8;
  SensorLSM6DSV16X.sensorDescriptor.subSensorDescriptor[2].ODR[0] = 1.875f;
  SensorLSM6DSV16X.sensorDescriptor.subSensorDescriptor[2].ODR[1] = 7.5f;
  SensorLSM6DSV16X.sensorDescriptor.subSensorDescriptor[2].ODR[2] = 15.0f;
  SensorLSM6DSV16X.sensorDescriptor.subSensorDescriptor[2].ODR[3] = 30.0f;
  SensorLSM6DSV16X.sensorDescriptor.subSensorDescriptor[2].ODR[4] = COM_END_OF_LIST_FLOAT;
  SensorLSM6DSV16X.sensorDescriptor.subSensorDescriptor[2].samplesPerTimestamp[0] = 0;
  SensorLSM6DSV16X.sensorDescriptor.subSensorDescriptor[2].samplesPerTimestamp[1] = 1000;
  strcpy(SensorLSM6DSV16X.sensorDescriptor.subSensorDescriptor[2].unit, "out");

  /* SUBSENSOR 2 STATUS */
  SensorLSM6DSV16X.sensorStatus.subSensorStatus[2].isActive = 0;
  SensorLSM6DSV16X.sensorStatus.subSensorStatus[2].FS = 0.0f;
  SensorLSM6DSV16X.sensorStatus.subSensorStatus[2].sensitivity = 1.0f;
  SensorLSM6DSV16X.sensorStatus.subSensorStatus[2].ODR = 0.0f;
  SensorLSM6DSV16X.sensorStatus.subSensorStatus[2].measuredODR = 0.0f;
  SensorLSM6DSV16X.sensorStatus.subSensorStatus[2].initialOffset = 0.0f;
  SensorLSM6DSV16X.sensorStatus.subSensorStatus[2].samplesPerTimestamp = 1;
  SensorLSM6DSV16X.sensorStatus.subSensorStatus[2].usbDataPacketSize = 16;
  SensorLSM6DSV16X.sensorStatus.subSensorStatus[2].sdWriteBufferSize = WRITE_BUFFER_SIZE_LSM6DSV16X_MLC;
  SensorLSM6DSV16X.sensorStatus.subSensorStatus[2].comChannelNumber = -1;
  SensorLSM6DSV16X.sensorStatus.subSensorStatus[2].ucfLoaded = 0;
  
  //Add the sensors to the Array
  tempJSON1 = json_value_init_object();
  create_JSON_Sensor(&SensorLPS22DF, tempJSON1);
  json_array_append_value(JSON_SensorArray,tempJSON1);
  tempJSON1 = json_value_init_object();
  create_JSON_Sensor(&SensorLSM6DSV16X, tempJSON1);
  json_array_append_value(JSON_SensorArray,tempJSON1);
  
}

/**
 * @brief  Callback Function called at Sensor Configuration
 * @param  uint8_t *configuration configuration received
 * @retval None
 */
static void ExtConfigSetSensorConfigCommandCallback(uint8_t *configuration)
{
  STBOX1_PRINTF("New Sensor Configuration = <%s>\r\n",configuration);
}

