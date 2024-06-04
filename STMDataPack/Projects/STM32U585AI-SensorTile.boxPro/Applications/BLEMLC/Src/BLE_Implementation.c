/**
  ******************************************************************************
  * @file    BLEMLC\Src\BLE_Implementation.c
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
#include "SensorTileBoxPro_motion_sensors_ex.h"
#include "uzlib.h"

/* Exported Variables --------------------------------------------------------*/
volatile uint8_t  connected   = FALSE;
volatile uint32_t RebootBoard = 0;
volatile uint32_t SwapBanks   = 0;

uint32_t ConnectionBleStatus =0;
uint32_t SizeOfUpdateBlueFW=0;

/* Imported Variables --------------------------------------------------------*/
extern TIM_HandleTypeDef TimCCHandle;
extern BLE_AR_output_t ActivityCode;

/* Private variables ---------------------------------------------------------*/
static uint32_t NeedToRebootBoard=0;
static uint32_t NeedToSwapBanks=0;

/* Private functions ---------------------------------------------------------*/
static void DisconnectionCompletedFunction(void);
static void ConnectionCompletedFunction(uint16_t ConnectionHandle, uint8_t Address_Type, uint8_t Addr[6]);
static uint32_t DebugConsoleParsing(uint8_t * att_data, uint8_t data_length);
static uint32_t DebugConsoleCommandParsing(uint8_t * att_data, uint8_t data_length,uint32_t *DecodingOneStream,int32_t *StreamLength,uint8_t **CompressedData);
static void ReadRequestActRec(BLE_AR_output_t *ActCode, BLE_AR_algoIdx_t *Algorithm);
static void ReadMachineLearningCore(uint8_t *mlc_out, uint8_t *mlc_status_mainpage);
static void ReadFiniteStateMachine(uint8_t *fsm_out, uint8_t *fsm_status_a_mainpage,uint8_t *fsm_status_b_mainpage);

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

static void NotifyEventInertial(BLE_NotifyEvent_t Event);
static void NotifyEventActRec(BLE_NotifyEvent_t Event);
static void NotifyEventMachineLearningCore(BLE_NotifyEvent_t Event);
static void NotifyEventFiniteStateMachine(BLE_NotifyEvent_t Event);

/* Function for decompressing the MCL/FSM program */
static uint32_t GetUncompressedSize(uint8_t *compressed, uint32_t size);
static uint8_t *Decompress(uint8_t *compressed, uint32_t size,uint32_t *UnComSize);
static void FromHexToUCF(const char *In, uint32_t len, ucf_line_t *UCFProgram);

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
//  HAL_NVIC_DisableIRQ(EXTI11_IRQn);
//  ResetBleManager();
//  InitBleManager();
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
  
  /* Characteristc allocation for inertial features */
  /* BLE_InitInertialService(AccEnable,GyroEnable,MagEnabled) */
  if(BleManagerAddChar(BLE_InitInertialService(1,1,1))==0) {
    STBOX1_PRINTF("Error adding Inertial Service\r\n");
  }
  CustomNotifyEventInertial = NotifyEventInertial;
  
  /* Characteristic allocation for Activity Recognition */
  CustomReadRequestActRec = ReadRequestActRec;
  if(BleManagerAddChar(BLE_InitActRecService())==0) {
    STBOX1_PRINTF("Error adding Activity Recognition Service\r\n");
  }
  CustomNotifyEventActRec = NotifyEventActRec;
  
  /* Characteristic allocation for FSM 8 Registers */
  CustomReadFiniteStateMachine = ReadFiniteStateMachine;
  if(BleManagerAddChar(BLE_InitFiniteStateMachineService(BLE_FSM_8_REG))==0) {
    STBOX1_PRINTF("Error adding FSM Service\r\n");
  }
  CustomNotifyEventFiniteStateMachine = NotifyEventFiniteStateMachine;
  
  /* Characteristic allocation for MLC 4 Registers */
  CustomReadMachineLearningCore = ReadMachineLearningCore;
  if(BleManagerAddChar(BLE_InitMachineLearningCoreService(BLE_MLC_4_REG))==0) {
    STBOX1_PRINTF("Error adding MLC Service\r\n");
  }
  CustomNotifyEventMachineLearningCore = NotifyEventMachineLearningCore;
  
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
 * @brief  Read reaquest from Activity Recognition characteristic
 * @param  BLE_AR_output_t ActCode Activity Recognized
 * @param  BLE_AR_algoIdx_t Algorithm Code
 * @retval None
 */
static void ReadRequestActRec(BLE_AR_output_t *ActCode, BLE_AR_algoIdx_t *Algorithm)
{
  *ActCode=ActivityCode;
  *Algorithm = HAR_ALGO_IDX_NONE;
}

/**
 * @brief  Read reaquest from Machine Learning Core characteristic
 * @param  uint8_t *mlc_out output of the MLC
 * @param  uint8_t *mlc_status_mainpage pointer to the MLC status mainpage
 * @retval None
 */
static void ReadMachineLearningCore(uint8_t *mlc_out, uint8_t *mlc_status_mainpage)
{
  lsm6dsv16x_all_sources_t status;
  lsm6dsv16x_all_sources_get(LSM6DSV16X_Contex, &status);
  
  *mlc_status_mainpage = (status.mlc1) | (status.mlc2<<1) | (status.mlc3<<2) | (status.mlc4<<3);
  lsm6dsv16x_mlc_out_get(LSM6DSV16X_Contex, (lsm6dsv16x_mlc_out_t*) mlc_out);
}

/**
 * @brief  Read reaquest from Finite State Machine characteristic
 * @param  uint8_t *fsm_out output of the FSM
 * @param  uint8_t *fsm_status_a_mainpage pointer to the FSM status mainpage A
 * @param  uint8_t *fsm_status_b_mainpage pointer to the FSM status mainpage B
 * @retval None
 */
static void ReadFiniteStateMachine(uint8_t *fsm_out, uint8_t *fsm_status_a_mainpage,uint8_t *fsm_status_b_mainpage)
{
  lsm6dsv16x_all_sources_t      status;  
  lsm6dsv16x_all_sources_get(LSM6DSV16X_Contex, &status);
  
  *fsm_status_a_mainpage = ((status.fsm1) | (status.fsm2<<1) | (status.fsm3<<2) | (status.fsm4<<3) |
                         (status.fsm5<<4) | (status.fsm6<<5) | (status.fsm7<<6) | (status.fsm8<<7));
  
  *fsm_status_b_mainpage = 0; //Dummy

  lsm6dsv16x_fsm_out_get(LSM6DSV16X_Contex, (lsm6dsv16x_fsm_out_t*)fsm_out);
}

/**
 * @brief  Callback Function for Un/Subscription Inertial Feature
 * @param  BLE_NotifyEvent_t Event Sub/Unsub
 * @retval None
 */
static void NotifyEventInertial(BLE_NotifyEvent_t Event)
{
   if(Event == BLE_NOTIFY_SUB){
    uint32_t uhCapture = __HAL_TIM_GET_COUNTER(&TimCCHandle);
    W2ST_ON_CONNECTION(W2ST_CONNECT_ACC_GYRO_MAG);
    
    /* Initialise the Acc/Gyro no MLC o FSM */
    InitAcc();
    
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
    
    DeInit_Acc();
    
    /* Stop the TIM Base generation in interrupt mode */
    if(HAL_TIM_OC_Stop_IT(&TimCCHandle, TIM_CHANNEL_3) != HAL_OK){
      /* Stopping Error */
      Error_Handler(STBOX1_ERROR_TIMER,__FILE__,__LINE__);
    }
    STBOX1_PRINTF("Stop Iner\r\n");
  }
}

/**
 * @brief  Callback Function for Un/Subscription Activity Rec Feature
 * @param  BLE_NotifyEvent_t Event Sub/Unsub
 * @retval None
 */
static void NotifyEventActRec(BLE_NotifyEvent_t Event)
{
   if(Event == BLE_NOTIFY_SUB){
    W2ST_ON_CONNECTION(W2ST_CONNECT_AR_EVENT);
    
    InitAcc_MLC(1);
    
    STBOX1_PRINTF("Start Activity Rec\r\n");
  } else if(Event == BLE_NOTIFY_UNSUB) {
    W2ST_OFF_CONNECTION(W2ST_CONNECT_AR_EVENT);
    
    DeInit_Acc();
    
    STBOX1_PRINTF("Stop Activity Rec\r\n");
  }
}

/**
 * @brief  Callback Function for Un/Subscription MLC Feature
 * @param  BLE_NotifyEvent_t Event Sub/Unsub
 * @retval None
 */
static void NotifyEventMachineLearningCore(BLE_NotifyEvent_t Event)
{
   if(Event == BLE_NOTIFY_SUB){
    W2ST_ON_CONNECTION(W2ST_CONNECT_MLC);
     
    InitAcc_MLC(1);
    
    STBOX1_PRINTF("Start MLC\r\n");
  } else if(Event == BLE_NOTIFY_UNSUB) {
    W2ST_OFF_CONNECTION(W2ST_CONNECT_MLC);
    
    DeInit_Acc();
      
    STBOX1_PRINTF("Stop MLC\r\n");
  }
}

/**
 * @brief  Callback Function for Un/Subscription FSM Feature
 * @param  BLE_NotifyEvent_t Event Sub/Unsub
 * @retval None
 */
static void NotifyEventFiniteStateMachine(BLE_NotifyEvent_t Event)
{
   if(Event == BLE_NOTIFY_SUB){
    W2ST_ON_CONNECTION(W2ST_CONNECT_FSM);
    InitAcc_FSM(0);
    
    STBOX1_PRINTF("Start FSM\r\n");
  } else if(Event == BLE_NOTIFY_UNSUB) {
    W2ST_OFF_CONNECTION(W2ST_CONNECT_FSM);
    STBOX1_PRINTF("Stop FSM\r\n");
  }
}


/**
 * @brief This function Decompresses one buffer
 * @param uint8_t *compressed buffer
 * @param uint32_t size dimension of the compressed buffer
 * @param uint32_t *UnComSize Size of uncompressed buffer
 * @retval uint8_t *Pointer to uncompressed buffer
 */
static uint8_t *Decompress(uint8_t *compressed, uint32_t size,uint32_t *UnComSize)
{
  struct uzlib_uncomp dest_struct;
  uint8_t* uncompressed;
  int res;
  uint32_t un_size;
  unsigned int chunk_len;

#define OUT_CHUNK_SIZE 1

  un_size = GetUncompressedSize(compressed,size);
  *UnComSize = un_size;
  STBOX1_PRINTF("Uncompressed Size =%ld\r\n",un_size);

  uncompressed = (uint8_t*)calloc(un_size, sizeof(uint8_t));
  if (uncompressed == NULL) {
    STBOX1_PRINTF("Error in memory allocation for decompression\r\n");
    return NULL;
  }
 
  uzlib_uncompress_init(&dest_struct,NULL,0);

  dest_struct.source = compressed;
  dest_struct.source_limit = compressed + size - 4;
  dest_struct.source_read_cb = NULL;
 
  res = uzlib_gzip_parse_header(&dest_struct);
 
  if (res != TINF_OK) {
    STBOX1_PRINTF("Error in decompressing regConfig\r\n");
    return NULL;
  }
  
  dest_struct.dest_start = dest_struct.dest = uncompressed;
  
  chunk_len = 0;
  
  while (un_size) {
    chunk_len = un_size < OUT_CHUNK_SIZE ? un_size : OUT_CHUNK_SIZE;
    dest_struct.dest_limit = dest_struct.dest + chunk_len;
    res = uzlib_uncompress_chksum(&dest_struct);
    un_size -= chunk_len;
    if (res != TINF_OK) {
      break;
    }
  }
  
  if (res != TINF_DONE) {
    STBOX1_PRINTF("Error in decompressing regConfig\r\n");
    return NULL;
  }
  return uncompressed;
}

/**
 * @brief This function return the size of uncompressed buffer
 * @param uint8_t *compressed buffer
 * @param uint32_t size dimension of the compressed buffer
 * @retval uint32_t size of uncompressed buffer
 */
static uint32_t GetUncompressedSize(uint8_t *compressed, uint32_t size)
{
  uint32_t dlen =     compressed[size - 1];
  dlen = (256*dlen) + compressed[size - 2];
  dlen = (256*dlen) + compressed[size - 3];
  dlen = (256*dlen) + compressed[size - 4];
  return dlen+1;  
}

#ifdef STBOX1_ENABLE_PRINTF
  /* For searching how many bytes are allocated and if the code release all the memory */
  static int32_t malloc_count =0;
  static int32_t malloc_size=0;
  static void *counted_malloc(size_t size)
  {
      void *res = malloc(size);
      if (res != NULL) {
          malloc_count++;
          malloc_size+=size;
      }
      return res;
  }

  static void counted_free(void *ptr) 
  {
      if (ptr != NULL) {
          malloc_count--;
      }
      free(ptr);
  }
#endif /* STBOX1_ENABLE_PRINTF */
  
/**
 * @brief  This function Converts one string to UCF program
 * @param const char *In Input char string
 * @param uint32_t len lenght of the input char string
 * @param ucf_line_t *UCFProgram pointer to output UCF program
 * @retval None
 */
static void FromHexToUCF(const char *In, uint32_t len, ucf_line_t *UCFProgram)
{
  uint32_t i, AH,AL,DH,DL;
  
  for (i = 0; i < len; i+=4) {
    char In1 = *In++;
    char In2 = *In++;
    char In3 = *In++;
    char In4 = *In++;
    AH = (In1 > '9') ? (In1 - 'A' + 10) : (In1 - '0');
    AL = (In2 > '9') ? (In2 - 'A' + 10) : (In2 - '0');
    DH = (In3 > '9') ? (In3 - 'A' + 10) : (In3 - '0');
    DL = (In4 > '9') ? (In4 - 'A' + 10) : (In4 - '0');
    UCFProgram->address = (AH << 4 ) | AL;
    UCFProgram->data    = (DH << 4 ) | DL;
    UCFProgram++;
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
  
  static uint32_t DecodingOneStream = 0;
  static int32_t StreamLength = -1; /* Nothing to Decode */
  static uint8_t *CompressedData=NULL;
  static int32_t PointerToCompressData=0;
  static uint8_t *DeCompressedData=NULL;
  
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
    if(DecodingOneStream) {
      /* If we are decoding one stream */
      /* we receive packets of 20 bytes */
      //STBOX1_PRINTF("Stream StreamLength=%d packet=%d\r\n",StreamLength,data_length);
      memcpy(CompressedData+PointerToCompressData,att_data,data_length);
      StreamLength -=data_length;
      PointerToCompressData+=data_length;
      /* Return Message */
      Term_Update(att_data,data_length);
      SendBackData=0;
    } else {
      /* Received one write from Client on Terminal characteristc */
      SendBackData = DebugConsoleCommandParsing(att_data,data_length,&DecodingOneStream,&StreamLength,&CompressedData);
    }
  }
 
  /* Decode the Stream full received */
  if(StreamLength==0) {
    /* Decode the stream */
    uint32_t UnComSize;
    STBOX1_PRINTF("End of Stream\r\n");
    /* Set Nothing to Receive */
    StreamLength = -1;
    DecodingOneStream =0;
    /* Decompressed the Data */
    STBOX1_PRINTF("--- Json Start Decompression ---\r\n");
    DeCompressedData = Decompress(CompressedData,PointerToCompressData,&UnComSize);
    STBOX1_PRINTF("--- Json End Decompression ---\r\n");
    /* Free Memory For Compressed Data*/
    free(CompressedData);
    CompressedData = NULL;
    PointerToCompressData=0;
      
    if(DeCompressedData==NULL) {
      /* Return Message */
      BytesToWrite =sprintf((char *)BufferToWrite,"Flow_parse_ko");
      Term_Update(BufferToWrite,BytesToWrite);
    } else {
      JSON_Value *root_value = NULL;
      STBOX1_PRINTF("--- Json Start Parsing ---\r\n");
#ifdef STBOX1_ENABLE_PRINTF
      json_set_allocation_functions(counted_malloc, counted_free);
#endif /* STBOX1_ENABLE_PRINTF */
      /* Parse the Decompressed Data */
      root_value = json_parse_string((char *)DeCompressedData);
       /* Free Memory For Decompressed Data*/
      free(DeCompressedData);
      DeCompressedData = NULL;
      if(json_value_get_type(root_value) == JSONArray){
        STBOX1_PRINTF("root_value==ARRAY\r\n");
      }
      JSON_Array *flows;
      int32_t NumFlows;
      flows = json_value_get_array(root_value);
      STBOX1_PRINTF("Num flows=%d\r\n",json_array_get_count(flows));
      for(NumFlows=0; NumFlows<json_array_get_count(flows);NumFlows++) {
        JSON_Object *flow;
        int32_t NumElementFlow;
        flow = json_array_get_object(flows, NumFlows);
        STBOX1_PRINTF("\tNumElementFlow=%d\r\n",json_object_get_count(flow));
        for(NumElementFlow=0;NumElementFlow<json_object_get_count(flow);NumElementFlow++) {
          STBOX1_PRINTF("\tElementName=%s\r\n",json_object_get_name(flow,NumElementFlow));
           if(!strncmp("sensors",json_object_get_name(flow,NumElementFlow),7)) {
             JSON_Array  *SensorsArray;
             int32_t NumSensors;
             SensorsArray = json_object_get_array  (flow, "sensors");
             STBOX1_PRINTF("\t\tNumSensors=%d\r\n",json_array_get_count(SensorsArray));
             for(NumSensors=0;NumSensors<json_array_get_count(SensorsArray);NumSensors++) {
               JSON_Object *Sensor;
               Sensor = json_array_get_object(SensorsArray, NumSensors);
               if(!strncmp("S12",json_object_get_string(Sensor,"id"),3)) {
                 /* Check if we have one program for MLC */
                 const char *regConfig;
                 const char *mlcLabels;
                 STBOX1_PRINTF("\t\t\tMLC Sensor ID found\r\n");
                 regConfig = json_object_dotget_string(Sensor,"configuration.regConfig");
                 if(regConfig!=NULL) {
                   uint32_t Length = strlen(regConfig);
                   STBOX1_PRINTF("\t\t\tMLC Reg Config [%ld] found\r\n",Length);
                   /* Allocate the Memory for the CustomUCF Program for MLC */ 
                   /* Length should be a multiple of 4 */
                   if(Length&0x3) {
                     /* Error */
                     STBOX1_PRINTF("Error Reg Config length not multiple of 4\r\n");
                   } else {
                     if(MLCCustomUCFFile!=NULL) {
                       /* if there is already one MLCCustomUCFFile...Release the Memory before */
                       free(MLCCustomUCFFile);
                       MLCCustomUCFFile = NULL;
                       MLCCustomUCFFileLength=0;
                     }
                     MLCCustomUCFFile = (ucf_line_t*)calloc(Length>>2, sizeof(ucf_line_t));
                     if (MLCCustomUCFFile == NULL) {
                       STBOX1_PRINTF("Error in memory allocation MLCCustomUCFFile\r\n");
                     } else {
                       MLCCustomUCFFileLength = Length>>2;
                       FromHexToUCF(regConfig, Length, MLCCustomUCFFile);
                     }
                   }
                 }
                 mlcLabels = json_object_dotget_string(Sensor,"configuration.mlcLabels");
                 if(mlcLabels!=NULL) {
                   uint32_t Length = strlen(mlcLabels);
                   STBOX1_PRINTF("\t\t\tMLC Labels [%ld] found\r\n",Length);
                   /* Allocate the Memory for the Custom Labels for MLC */ 
                    extern char *MLCCustomLabels;
                    if(MLCCustomLabels!=NULL) {
                      /* if there is already one MLCCustomLabels...Release the Memory before */
                      free(MLCCustomLabels);
                      MLCCustomLabels = NULL;
                    }
                    MLCCustomLabels = (char *)calloc(Length+1, sizeof(char));
                    if (MLCCustomLabels == NULL) {
                      STBOX1_PRINTF("Error in memory allocation MLCCustomLabels\r\n");
                    } else {
                      MLCCustomLabelsLength = Length+1;
                      memcpy(MLCCustomLabels,mlcLabels,Length);
                      /* Put Termination */
                      MLCCustomLabels[Length]='\n';
                    }
                 }
               } else if(!strncmp("S13",json_object_get_string(Sensor,"id"),3)) {
                 /* Check if we have one program for FSM */
                 const char *regConfig;
                 const char *fsmLabels;
                 STBOX1_PRINTF("\t\t\tFSM Sensor ID found\r\n");
                 regConfig = json_object_dotget_string(Sensor,"configuration.regConfig");
                 if(regConfig!=NULL) {
                   uint32_t Length = strlen(regConfig);
                   STBOX1_PRINTF("\t\t\tFSM Reg Config [%ld] found\r\n",Length);
                   /* Allocate the Memory for the CustomUCF Program for FSM */ 
                   /* Length should be a multiple of 4 */
                   if(Length&0x3) {
                     /* Error */
                     STBOX1_PRINTF("Error Reg Config length not multiple of 4\r\n");
                   } else {
                     if(FSMCustomUCFFile!=NULL) {
                       /* if there is already one FSMCustomUCFFile...Release the Memory before */
                       free(FSMCustomUCFFile);
                       FSMCustomUCFFile = NULL;
                       FSMCustomUCFFileLength=0;
                     }
                     FSMCustomUCFFile = (ucf_line_t*)calloc(Length>>2, sizeof(ucf_line_t));
                     if (FSMCustomUCFFile == NULL) {
                       STBOX1_PRINTF("Error in memory allocation FSMCustomUCFFile\r\n");
                     } else {
                       FSMCustomUCFFileLength = Length>>2;
                       FromHexToUCF(regConfig, Length, FSMCustomUCFFile);
                     }
                   }
                 }
                 fsmLabels = json_object_dotget_string(Sensor,"configuration.fsmLabels");
                 if(fsmLabels!=NULL) {
                   uint32_t Length = strlen(fsmLabels);
                   STBOX1_PRINTF("\t\t\tFSM Labels [%ld] found\r\n",Length);
                   /* Allocate the Memory for the Custom Labels for FSM */ 
                    extern char *FSMCustomLabels;
                    if(FSMCustomLabels!=NULL) {
                      /* if there is already one FSMCustomLabels...Release the Memory before */
                      free(FSMCustomLabels);
                      FSMCustomLabels = NULL;
                    }
                    FSMCustomLabels = (char *)calloc(Length+1, sizeof(char));
                    if (FSMCustomLabels == NULL) {
                      STBOX1_PRINTF("Error in memory allocation FSMCustomLabels\r\n");
                    } else {
                      FSMCustomLabelsLength = Length+1;
                      memcpy(FSMCustomLabels,fsmLabels,Length);
                      /* Put Termination */
                      FSMCustomLabels[Length]='\n';
                    }
                 }
               }
             }
           }
        }
      }
      json_value_free(root_value);
      STBOX1_PRINTF("%ld Alloc Not Released\r\nTotal Mem Used=%ld\r\n",malloc_count,malloc_size);
      STBOX1_PRINTF("--- Json End Parsing ---\r\n");
      /* Return Message */
      BytesToWrite =sprintf((char *)BufferToWrite,"Flow_parse_ok");
      Term_Update(BufferToWrite,BytesToWrite);
    }
  }

  return SendBackData;
}

/**
 * @brief  This function makes the parsing of the Debug Console Commands
 * @param  uint8_t *att_data attribute data
 * @param  uint8_t data_length length of the data
 * @param uint32_t *DecodingOneStream Flag for understanding when we are decoding one Stream
 * @param int32_t *StreamLength Length of the application stream
 * @param uint8_t **CompressedData pointer to buffer for storing the compressed json
 * @retval uint32_t SendBackData true/false
 */
static uint32_t DebugConsoleCommandParsing(uint8_t * att_data, uint8_t data_length,uint32_t *DecodingOneStream,int32_t *StreamLength,uint8_t **CompressedData)
{
  uint32_t SendBackData = 1;

  /* Help Command */
  if(!strncmp("help",(char *)(att_data),4)) {
    /* Print Legend */
    SendBackData=0;

    BytesToWrite =sprintf((char *)BufferToWrite,
         "info\n"
         "getMLCLabels\n"
         "delMLCCustom\n"
         "delFSMCustom\n"
         "getFSMLabels\n");
    Term_Update(BufferToWrite,BytesToWrite);
  } else if(!strncmp("versionFw",(char *)(att_data),9)) {
    BytesToWrite =sprintf((char *)BufferToWrite,"%s_%s_%c.%c.%c\r\n",
                          "U585",
                          STBOX1_PACKAGENAME,
                          STBOX1_VERSION_MAJOR,
                          STBOX1_VERSION_MINOR,
                          STBOX1_VERSION_PATCH);
    Term_Update(BufferToWrite,BytesToWrite);
    BytesToWrite =sprintf((char *)BufferToWrite,"Current Bank =%ld\n",CurrentActiveBank);
    Term_Update(BufferToWrite,BytesToWrite);
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
  } else if(!strncmp("getFSMLabels",(char *)(att_data),12)) {
      if(FSMCustomLabels==NULL) {
        BytesToWrite =sprintf((char *)BufferToWrite,"<FSM_OUTS1>4D PosRec,0='N/A',16='Portrait Down',32='Portrait Up',64='Landscape Right',128='Landscape Left';\n");
        Term_Update(BufferToWrite,BytesToWrite);
      } else {
        uint32_t Counter;
        for(Counter=0;Counter<FSMCustomLabelsLength;Counter+=MaxBleCharStdOutLen) {
          uint32_t MinSize;
          MinSize = FSMCustomLabelsLength-Counter;
          MinSize = (MinSize>MaxBleCharStdOutLen) ?  MaxBleCharStdOutLen : MinSize;
          Term_Update((uint8_t *) (FSMCustomLabels+Counter),MinSize);
        }
      }
      SendBackData=0;
   } else if(!strncmp("getMLCLabels",(char *)(att_data),12)) {
      if(MLCCustomLabels==NULL) {
        BytesToWrite =sprintf((char *)BufferToWrite,"<MLC0>Activity Rec,1='Other',4='Walking',8='Running',12='Driving';\n");
        Term_Update(BufferToWrite,BytesToWrite);
      } else {
        uint32_t Counter;
        for(Counter=0;Counter<MLCCustomLabelsLength;Counter+=MaxBleCharStdOutLen) {
          uint32_t MinSize;
          MinSize = MLCCustomLabelsLength-Counter;
          MinSize = (MinSize>MaxBleCharStdOutLen) ?  MaxBleCharStdOutLen : MinSize;
          Term_Update((uint8_t *) (MLCCustomLabels+Counter),MinSize);
        }
      }
      SendBackData=0;
    } else if(!strncmp("delMLCCustom",(char *)(att_data),12)) {
      if(MLCCustomLabels!=NULL) {
        free(MLCCustomLabels);
        MLCCustomLabels=NULL;
        MLCCustomLabelsLength =0;
        BytesToWrite =sprintf((char *)BufferToWrite,"MLCCustomLabels Deleted\n");
        Term_Update(BufferToWrite,BytesToWrite);
      }
      if(MLCCustomUCFFile!=NULL) {
        free(MLCCustomUCFFile);
        MLCCustomUCFFile=NULL;
        MLCCustomUCFFileLength =0;
        BytesToWrite =sprintf((char *)BufferToWrite,"MLCCustomUCFFile Deleted\n");
        Term_Update(BufferToWrite,BytesToWrite);
      }
       SendBackData=0;
    } else if(!strncmp("delFSMCustom",(char *)(att_data),12)) {
      if(FSMCustomLabels!=NULL) {
        free(FSMCustomLabels);
        FSMCustomLabels=NULL;
        FSMCustomLabelsLength =0;
        BytesToWrite =sprintf((char *)BufferToWrite,"FSMCustomLabels Deleted\n");
        Term_Update(BufferToWrite,BytesToWrite);
      }
      if(FSMCustomUCFFile!=NULL) {
        free(FSMCustomUCFFile);
        FSMCustomUCFFile=NULL;
        FSMCustomUCFFileLength =0;
        BytesToWrite =sprintf((char *)BufferToWrite,"FSMCustomUCFFile Deleted\n");
        Term_Update(BufferToWrite,BytesToWrite);
      }
      SendBackData=0;
    } else if(!strncmp("SF",(char *)(att_data),2)) {
      uint32_t TimeStamp;
      uint8_t *PointerByte = (uint8_t*) StreamLength;
      PointerByte[0]=att_data[5];
      PointerByte[1]=att_data[4];
      PointerByte[2]=att_data[3];
      PointerByte[3]=att_data[2];
      
      PointerByte = (uint8_t*) &TimeStamp;
      PointerByte[0]=att_data[9];
      PointerByte[1]=att_data[8];
      PointerByte[2]=att_data[7];
      PointerByte[3]=att_data[6];

      /* Debug Message */      
      STBOX1_PRINTF("SF command Length=%ld, TS=%lu\r\n",*StreamLength,TimeStamp);

      /* Alloc buffer for storing compressed json */
      *CompressedData = (uint8_t*) malloc((*StreamLength)*sizeof(uint8_t));
      if((*CompressedData)==NULL) {
        STBOX1_PRINTF("Memory Allocation error for CompressedData\r\n");
        /* Return Message */
        BytesToWrite =sprintf((char *)BufferToWrite,"Allocation Error");
        Term_Update(BufferToWrite,BytesToWrite);
      } else {
         /* Return Message */
        BytesToWrite =sprintf((char *)BufferToWrite,"Flow_Req_Received");
        Term_Update(BufferToWrite,BytesToWrite);
      }

      SendBackData = 0;
      *DecodingOneStream =1;
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
  if(W2ST_CHECK_CONNECTION(W2ST_CONNECT_ACC_GYRO_MAG)) {
    if(HAL_TIM_OC_Stop_IT(&TimCCHandle, TIM_CHANNEL_3) != HAL_OK){
      /* Stopping Error */
      Error_Handler(STBOX1_ERROR_TIMER,__FILE__,__LINE__);
    }
    STBOX1_PRINTF("Stop Iner\r\n");
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

