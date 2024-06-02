/**
  ******************************************************************************
  * @file    BLEDualProgram\Src\BLE_Implementation.c
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
volatile uint8_t  paired      = FALSE;
volatile uint32_t RebootBoard = 0;
volatile uint32_t SwapBanks   = 0;
volatile uint32_t NeedToClearSecureDB=0;

uint32_t SizeOfUpdateBlueFW=0;
uint32_t ConnectionBleStatus =0;

/* Imported Variables --------------------------------------------------------*/
extern TIM_HandleTypeDef TimCCHandle;

/* Private variables ---------------------------------------------------------*/
static uint32_t NeedToRebootBoard=0;
static uint32_t NeedToSwapBanks=0;

/* Private functions ---------------------------------------------------------*/
static uint32_t DebugConsoleParsing(uint8_t * att_data, uint8_t data_length);
static void ReadRequestEnvFunction(int32_t *Press,uint16_t *Hum,int16_t *Temp1,int16_t *Temp2);
static void DisconnectionCompletedFunction(void);
static void ConnectionCompletedFunction(uint16_t ConnectionHandle, uint8_t Address_Type, uint8_t Addr[6]);
static void PairingCompletedFunction(uint8_t PairingStatus);

static uint32_t DebugConsoleCommandParsing(uint8_t * att_data, uint8_t data_length);

/**********************************************************************************************
 * Callback functions prototypes to manage the extended configuration characteristic commands *
 **********************************************************************************************/
static void ExtExtConfigUidCommandCallback(uint8_t **UID);
static void ExtConfigInfoCommandCallback(uint8_t *Answer);
static void ExtConfigHelpCommandCallback(uint8_t *Answer);
static void ExtConfigVersionFwCommandCallback(uint8_t *Answer);
static void ExtConfigClearDBCommandCallback(void);
static void ExtConfigSetNameCommandCallback(uint8_t *NewName);
static void ExtConfigReadBanksFwIdCommandCallback (uint8_t *CurBank,uint16_t *FwId1,uint16_t *FwId2);
static void ExtConfigBanksSwapCommandCallback(void);

static void NotifyEventSensorFusion(BLE_NotifyEvent_t Event);
static void NotifyEventEnv(BLE_NotifyEvent_t Event);
static void NotifyEventInertial(BLE_NotifyEvent_t Event);


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
#ifdef STBOX1_BLE_SECURE_CONNECTION
  BLE_StackValue.EnableSecureConnection=1;
  /* Default Secure PIN */
  BLE_StackValue.SecurePIN=123456;
  /* For creating a Random Secure PIN */
  BLE_StackValue.EnableRandomSecurePIN = 0;
#else /* STBOX1_BLE_SECURE_CONNECTION */
  BLE_StackValue.EnableSecureConnection=0;
#endif /* STBOX1_BLE_SECURE_CONNECTION */
 
  
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
  
  /* Custom Paring Complete Function */
  CustomPairingCompleted = PairingCompletedFunction;

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

  /* Characteristc allocation for Sensor Fusion feature */
  if(BleManagerAddChar(BLE_InitSensorFusionService(1))==0) {
    STBOX1_PRINTF("Error adding SensorFusion Service\r\n");
  }
  CustomNotifyEventSensorFusion = NotifyEventSensorFusion;
  
  /***********************************************************************************
  * Callback functions to manage the extended configuration characteristic commands *
  ***********************************************************************************/
  CustomExtConfigUidCommandCallback       = ExtExtConfigUidCommandCallback;
  CustomExtConfigInfoCommandCallback      = ExtConfigInfoCommandCallback;
  CustomExtConfigHelpCommandCallback      = ExtConfigHelpCommandCallback;
  CustomExtConfigVersionFwCommandCallback = ExtConfigVersionFwCommandCallback;
  CustomExtConfigClearDBCommandCallback   = ExtConfigClearDBCommandCallback;
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
 * @brief  Set Custom Avertise Data.
 * @param  uint8_t *manuf_data: Avertise Data
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
  manuf_data[BLE_MANAGER_CUSTOM_FIELD3]=0x00; /* Not Used */
  manuf_data[BLE_MANAGER_CUSTOM_FIELD4]=0x00; /* Not Used */
}

/**
 * @brief  Callback Function for Un/Subscription SensorFusion Feature
 * @param  BLE_NotifyEvent_t Event
 * @retval None
 */
static void NotifyEventSensorFusion(BLE_NotifyEvent_t Event)
{
  if(Event == BLE_NOTIFY_SUB){
    uint32_t uhCapture = __HAL_TIM_GET_COUNTER(&TimCCHandle);
    W2ST_ON_CONNECTION(W2ST_CONNECT_QUAT_EVENT);
    
    /* Start the TIM Base generation in interrupt mode */
    if(HAL_TIM_OC_Start_IT(&TimCCHandle, TIM_CHANNEL_3) != HAL_OK){
      /* Starting Error */
      Error_Handler(STBOX1_ERROR_TIMER,__FILE__,__LINE__);
    }
    
    /* Set the Capture Compare Register value */
    __HAL_TIM_SET_COMPARE(&TimCCHandle, TIM_CHANNEL_3, (uhCapture + STBOX1_UPDATE_INV));
     STBOX1_PRINTF("Start Fusion\r\n");
  } else if(Event == BLE_NOTIFY_UNSUB) {
    W2ST_OFF_CONNECTION(W2ST_CONNECT_QUAT_EVENT);
    
    /* Stop the TIM Base generation in interrupt mode */
    if(HAL_TIM_OC_Stop_IT(&TimCCHandle, TIM_CHANNEL_3) != HAL_OK){
      /* Stopping Error */
      Error_Handler(STBOX1_ERROR_TIMER,__FILE__,__LINE__);
    }
    STBOX1_PRINTF("Stop Fusion\r\n");
  }
}

/**
 * @brief  Callback Function for Un/Subscription Environmental Feature
 * @param  BLE_NotifyEvent_t Event
 * @retval None
 */
static void NotifyEventEnv(BLE_NotifyEvent_t Event)
{
  if(Event == BLE_NOTIFY_SUB){
    uint32_t uhCapture = __HAL_TIM_GET_COUNTER(&TimCCHandle);
    W2ST_ON_CONNECTION(W2ST_CONNECT_ENV);
    
    /* Start the TIM Base generation in interrupt mode */
    if(HAL_TIM_OC_Start_IT(&TimCCHandle, TIM_CHANNEL_2) != HAL_OK){
      /* Starting Error */
      Error_Handler(STBOX1_ERROR_TIMER,__FILE__,__LINE__);
    }
    
    /* Set the Capture Compare Register value */
    __HAL_TIM_SET_COMPARE(&TimCCHandle, TIM_CHANNEL_2, (uhCapture + STBOX1_UPDATE_ENV));
    STBOX1_PRINTF("Start Env\r\n");
  } else if(Event == BLE_NOTIFY_UNSUB) {
    W2ST_OFF_CONNECTION(W2ST_CONNECT_ENV);
    /* Stop the TIM Base generation in interrupt mode */
    if(HAL_TIM_OC_Stop_IT(&TimCCHandle, TIM_CHANNEL_2) != HAL_OK){
      /* Stopping Error */
      Error_Handler(STBOX1_ERROR_TIMER,__FILE__,__LINE__);
    }
    STBOX1_PRINTF("Stop Env\r\n");
  }
}

/**
 * @brief  Callback Function for Un/Subscription Inertial Feature
 * @param  BLE_NotifyEvent_t Event
 * @retval None
 */
static void NotifyEventInertial(BLE_NotifyEvent_t Event)
{
   if(Event == BLE_NOTIFY_SUB){
    uint32_t uhCapture = __HAL_TIM_GET_COUNTER(&TimCCHandle);
    W2ST_ON_CONNECTION(W2ST_CONNECT_ACC_GYRO_MAG);
    
    /* Start the TIM Base generation in interrupt mode */
    if(HAL_TIM_OC_Start_IT(&TimCCHandle, TIM_CHANNEL_3) != HAL_OK){
      /* Starting Error */
      Error_Handler(STBOX1_ERROR_TIMER,__FILE__,__LINE__);
    }
    
    /* Set the Capture Compare Register value */
    __HAL_TIM_SET_COMPARE(&TimCCHandle, TIM_CHANNEL_3, (uhCapture + STBOX1_UPDATE_INV));
    STBOX1_PRINTF("Start Iner\r\n");
  } else if(Event == BLE_NOTIFY_UNSUB) {
    W2ST_OFF_CONNECTION(W2ST_CONNECT_ACC_GYRO_MAG);
    
    /* Stop the TIM Base generation in interrupt mode */
    if(HAL_TIM_OC_Stop_IT(&TimCCHandle, TIM_CHANNEL_3) != HAL_OK){
      /* Stopping Error */
      Error_Handler(STBOX1_ERROR_TIMER,__FILE__,__LINE__);
    }
    STBOX1_PRINTF("Stop Iner\r\n");
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
                          "info\n"
                          "clearDB\n");
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
  } else  if(!strncmp("clearDB",(char *)(att_data),7)) {
    BytesToWrite =sprintf((char *)BufferToWrite,"\nThe Secure database will be cleared\n");
    Term_Update(BufferToWrite,BytesToWrite);
    NeedToClearSecureDB=1;
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
  float data_t;
  float data_p;
  /* Update emulated Environmental data */
  Set_Random_Environmental_Values(&data_t, &data_p);
  *Press = (int32_t)(data_p *100);
  *Temp1 = (int16_t)(data_t * 10);

}


/**
 * @brief  This function is called when the peer device get disconnected.
 * @param  None 
 * @retval None
 */
static void DisconnectionCompletedFunction(void)
{
  connected = FALSE;
  paired    = FALSE;

  /* Make the device connectable again */
  
  /* Reset for any problem during FOTA update */
  SizeOfUpdateBlueFW = 0;
  
  /*Stop all the timers */
  if(W2ST_CHECK_CONNECTION(W2ST_CONNECT_QUAT_EVENT)) {
    if(HAL_TIM_OC_Stop_IT(&TimCCHandle, TIM_CHANNEL_3) != HAL_OK){
      /* Stopping Error */
      Error_Handler(STBOX1_ERROR_TIMER,__FILE__,__LINE__);
    }
    STBOX1_PRINTF("Stop Fusion\r\n");
  }
  
  if(W2ST_CHECK_CONNECTION(W2ST_CONNECT_ACC_GYRO_MAG)) {
    if(HAL_TIM_OC_Stop_IT(&TimCCHandle, TIM_CHANNEL_3) != HAL_OK){
      /* Stopping Error */
      Error_Handler(STBOX1_ERROR_TIMER,__FILE__,__LINE__);
    }
    STBOX1_PRINTF("Stop Iner\r\n");
  }
  
  if(W2ST_CHECK_CONNECTION(W2ST_CONNECT_ENV)) {
    if(HAL_TIM_OC_Stop_IT(&TimCCHandle, TIM_CHANNEL_2) != HAL_OK){
      /* Stopping Error */
      Error_Handler(STBOX1_ERROR_TIMER,__FILE__,__LINE__);
    }
    STBOX1_PRINTF("Stop Env\r\n");
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
  
#ifdef STBOX1_BLE_SECURE_CONNECTION
  /* Check if the device is already bonded */
  if( aci_gap_is_device_bonded(Address_Type,Addr) ==BLE_STATUS_SUCCESS) {
    STBOX1_PRINTF("Device already bounded\r\n");
    paired = TRUE;
  }
#else
  paired = TRUE;
#endif /* STBOX1_BLE_SECURE_CONNECTION */

  /* Stop the TIM Base generation in interrupt mode for Led Blinking*/
  if(HAL_TIM_OC_Stop_IT(&TimCCHandle, TIM_CHANNEL_1) != HAL_OK){
    /* Stopping Error */
    Error_Handler(STBOX1_ERROR_TIMER,__FILE__,__LINE__);
  }

  BSP_LED_Off(LED_GREEN);

  HAL_Delay(100);
}

/**
 * @brief  This function is called when there is a Pairing Complete event.
 * @param  uint8_t PairingStatus 
 * @retval None
 */
static void PairingCompletedFunction(uint8_t PairingStatus)
{
   /* Pairing Status:
    * 0x00 -> "Success",
    * 0x01 -> "Timeout",
    * 0x02 -> "Pairing Failed",
    * 0x03 -> "Encryption failed, LTK missing on local device",
    * 0x04 -> "Encryption failed, LTK missing on peer device",
    * 0x05 -> "Encryption not supported by remote device"
    */
  if(PairingStatus==0x00 /* Success */) {
    paired=TRUE;
  } else {
    paired=FALSE;
  }
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
 * @brief  Callback Function for answering to ClearDB Command
 * @param  None
 * @retval None
 */
static void ExtConfigClearDBCommandCallback(void) {
  NeedToClearSecureDB=1;
}

/**
 * @brief  Callback Function for answering to Info command
 * @param  uint8_t *Answer response to command
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

