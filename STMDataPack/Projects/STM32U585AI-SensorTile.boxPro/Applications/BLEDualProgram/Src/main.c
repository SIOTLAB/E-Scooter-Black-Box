/**
  ******************************************************************************
  * @file    BLEDualProgram\Src\main.c
  * @author  System Research & Applications Team - Agrate/Catania Lab.
  * @version V1.6.0
  * @date    20-Oct-2023
  * @brief   Main program body
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

/**
 *
 * @page BLEDualProgram Secure BLE Firmware Over the Air Update
 *
 * @image html st_logo.png
 *
 * <b>Introduction</b>
 *
 * This firmware package includes Components Device Drivers, Board Support Package
 * and example application for the following STMicroelectronics elements:
 * - STEVAL-MKBOXPRO (SensorTile.box-Pro) evaluation board that contains the following components:
 *   - MEMS sensor devices: STTS22, LPS22DF, LSM6DSV16X, LIS2DU12, LIS2MDL
 *   - Gas Gouge device: STC3115
 *   - Digital Microphone: MP23db01HP
 *   - Dynamic NFC tag: ST25DV04K
 *   - BlueNRG-LP Bluetooth Low Energy System On Chip
 *
 * <b>Example Application</b>
 *
 * The Example application initializes all the Components and Library creating some Custom Bluetooth services
 * At the Beginning the application writes on NFC tag the URL of st.com, after pressing the user button, the application
 * changes the content of NFC adding a deep URI for automatic connection to the board
 *
 * The application allows connections only from bonded devices (PIN request) allowing them to updated its firwmare with 
 * one Over the Air update (FoTA)
 *
 *
 * This example must be used with the related ST BLE Sensor Android/iOS application available on Play/itune store
 * (Version 4.18.0 or higher), in order to read the sent information by Bluetooth Low Energy protocol
 *
 * 
 */

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "SensorTileBoxPro.h"
#include "bluenrg_conf.h"
#include "tagtype5_wrapper.h"
#include "lib_NDEF_URI.h"
#include "OTA.h"
#include "BLE_Manager.h"
#ifdef STBOX1_ENABLE_START_BIP
#include "note.h"
#endif /* STBOX1_ENABLE_START_BIP */

/* Exported variables --------------------------------------------------------*/
uint16_t ConnectionHandle = 0;

TIM_HandleTypeDef TimCCHandle;
int32_t CurrentActiveBank = 0;

/* Imported variables --------------------------------------------------------*/
extern volatile uint32_t hci_event;

FinishGood_TypeDef FinishGood;


/* Private includes ----------------------------------------------------------*/

/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/
#ifdef STBOX1_ENABLE_START_BIP
  #define SPKR_Pin GPIO_PIN_13
  #define SPKR_GPIO_Port GPIOE
#endif /* STBOX1_ENABLE_START_BIP*/

/* Private macro -------------------------------------------------------------*/
#ifdef STBOX1_ENABLE_START_BIP
  #define MCR_START_HEART_BIT()   \
  {                         \
    BSP_LED_On(LED_YELLOW); \
    BSP_LED_On(LED_GREEN);  \
    beep(NOTE_A6, 50);      \
    HAL_Delay(150);         \
    BSP_LED_Off(LED_YELLOW);\
    BSP_LED_Off(LED_GREEN); \
    HAL_Delay(400);         \
    BSP_LED_On(LED_YELLOW); \
    BSP_LED_On(LED_GREEN);  \
    beep(NOTE_G6, 50);      \
    HAL_Delay(150);         \
    BSP_LED_Off(LED_YELLOW);\
    BSP_LED_Off(LED_GREEN); \
    HAL_Delay(400);         \
    beep(NOTE_F6, 50);      \
  }

  #define MCR_START_HEART_BIT2()  \
  {                         \
    BSP_LED_On(LED_YELLOW); \
    BSP_LED_On(LED_RED);    \
    beep(NOTE_F6, 50);      \
    HAL_Delay(150);         \
    BSP_LED_Off(LED_YELLOW);\
    BSP_LED_Off(LED_RED);   \
    HAL_Delay(400);         \
    BSP_LED_On(LED_YELLOW); \
    BSP_LED_On(LED_RED);    \
    beep(NOTE_G6, 50);      \
    HAL_Delay(150);         \
    BSP_LED_Off(LED_YELLOW);\
    BSP_LED_Off(LED_RED);   \
    HAL_Delay(400);         \
    beep(NOTE_A6, 50);      \
  }
#else /* STBOX1_ENABLE_START_BIP */
  #define MCR_START_HEART_BIT MCR_HEART_BIT
  #define MCR_START_HEART_BIT2 MCR_HEART_BIT2
#endif /* STBOX1_ENABLE_START_BIP */
  
/* Private variables ---------------------------------------------------------*/
static volatile uint32_t user_button_pressed = 0;

static volatile uint32_t BlinkLed        = 0;
static volatile uint32_t SendEnv         = 0;
static volatile uint32_t SendAccGyroMag  = 0;
static volatile uint32_t SendQuat        = 0;

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_ICACHE_Init(void);
void NDEF_URI_Init(void);
static void User_Init(void);
static void Set_Random_Motion_Values(BLE_MANAGER_INERTIAL_Axes_t *x_axes, 
                                     BLE_MANAGER_INERTIAL_Axes_t *g_axes, 
                                     BLE_MANAGER_INERTIAL_Axes_t * m_axes, 
                                     BLE_MOTION_SENSOR_Axes_t *q_axes,
                                     uint32_t cnt);
static void Reset_Motion_Values(BLE_MANAGER_INERTIAL_Axes_t *x_axes, 
                                 BLE_MANAGER_INERTIAL_Axes_t *g_axes, 
                                 BLE_MANAGER_INERTIAL_Axes_t * m_axes, 
                                 BLE_MOTION_SENSOR_Axes_t *q_axes);

static void InitTimers(void);
static void PrintInfo(void);
static void ClearSecureDB(void);
static void BLE_Pairing_ST25DV(void);

#ifdef STBOX1_ENABLE_START_BIP
  static void TIM_Beep_MspPostInit(TIM_HandleTypeDef* htim);
  static void TIM_Beep_Init(void);
  static void TIM_Beep_DeInit(void);
  static void beep(uint32_t freq, uint16_t time);
#endif /* STBOX1_ENABLE_START_BIP */

/* Private user code ---------------------------------------------------------*/

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{ 
  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* Configure the system clock */
  SystemClock_Config();

  /* Initialize all configured peripherals */
  MX_ICACHE_Init();
  
  /* Set a random seed */
  srand(HAL_GetTick());
  
  User_Init();
  
  STBOX1_PRINTF("\033[2J"); /* serial console clear screen */
  STBOX1_PRINTF("\033[H");  /* serial console cursor to home */
  PrintInfo();
  
  BSP_LED_On(LED_BLUE);
  
  /* Init BLE */
  STBOX1_PRINTF("\r\nInitializing Bluetooth\r\n");
  BluetoothInit();
  
  BSP_LED_Off(LED_BLUE);
  BSP_LED_On(LED_GREEN);
  
  /* Short delay before starting the user application process */
  HAL_Delay(500);
  STBOX1_PRINTF("BLE Stack Initialized & Device Configured\r\n");
  
  //This is for Writing a URI..."st.com"
  NDEF_URI_Init();

  /* Infinite loop */
  while (1) {
    /* BLE Event */
    if(hci_event) {
      hci_event=0;
      hci_user_evt_proc();
    }
    
    /* Make the device discoverable */
    if(set_connectable)
    {
      uint32_t uhCapture = __HAL_TIM_GET_COUNTER(&TimCCHandle);
      /* Start the TIM Base generation in interrupt mode */
      if(HAL_TIM_OC_Start_IT(&TimCCHandle, TIM_CHANNEL_1) != HAL_OK){
        /* Starting Error */
        Error_Handler(STBOX1_ERROR_TIMER,__FILE__,__LINE__);
      }
      /* Set the Capture Compare Register value */
      __HAL_TIM_SET_COMPARE(&TimCCHandle, TIM_CHANNEL_1, (uhCapture + STBOX1_UPDATE_LED));
      
      setConnectable();
      /* If we need to clear the Secure DataBase */
      if(NeedToClearSecureDB) {
        NeedToClearSecureDB =0;
        ClearSecureDB();
      }
      set_connectable = FALSE;
    }
    
    /*  Update sensor value */
    if (SendEnv) {
      float data_t;
      float data_p;
      SendEnv=0;
      /* Update emulated Environmental data */
      Set_Random_Environmental_Values(&data_t, &data_p);
      
      if(paired == TRUE) {
        //Send Values only if we are paired with the Device */
        BLE_EnvironmentalUpdate((int32_t)(data_p *100),0,(int16_t)(data_t * 10),0);
      }
    }
    
    if ((SendAccGyroMag) || (SendQuat)) {
      static uint32_t counter = 0;
      static BLE_MANAGER_INERTIAL_Axes_t x_axes = {0,0,0};
      static BLE_MANAGER_INERTIAL_Axes_t g_axes = {0,0,0};
      static BLE_MANAGER_INERTIAL_Axes_t m_axes = {0,0,0};
      static BLE_MOTION_SENSOR_Axes_t q_axes = {0,0,0};
      /* Update emulated Acceleration, Gyroscope and Sensor Fusion data */
      Set_Random_Motion_Values(&x_axes,&g_axes,&m_axes,&q_axes,counter);
      if (SendAccGyroMag) {
        SendAccGyroMag=0;
        
        if(paired == TRUE) {
         //Send Values only if we are paired with the Device */
         BLE_AccGyroMagUpdate(&x_axes,&g_axes,&m_axes);
        }
      }
      if (SendQuat) {
        SendQuat=0;
        if(paired == TRUE) {
          //Send Values only if we are paired with the Device */
          BLE_SensorFusionUpdate(&q_axes, 1);
        }
      }
      counter++;
      if (counter == 40) {
        counter = 0;
        Reset_Motion_Values(&x_axes,&g_axes,&m_axes,&q_axes);
      }
    }
    
    /* Reboot the Board */
    if(RebootBoard) {
      RebootBoard=0;
      HAL_NVIC_SystemReset();
    }
    
    /* Swap the Flash Banks */
    if(SwapBanks) {
      EnableDisableDualBoot();
      SwapBanks=0;
    }
    
    /* Handle the user button */
    if(user_button_pressed) {
      static int32_t BleConnInfoWrittenOnNFC=0;
      user_button_pressed=0;
      STBOX1_PRINTF("User Button pressed...\r\n");
      if(!BleConnInfoWrittenOnNFC) {
        BleConnInfoWrittenOnNFC=1;
        BSP_LED_On(LED_YELLOW);
        /* Write BLE Pairing Information */
        BLE_Pairing_ST25DV();
        BSP_LED_Off(LED_YELLOW);
      } else {
        /* Clear the Secure DB if we are not connected */
        if(connected==FALSE) {
          ClearSecureDB();
          BleConnInfoWrittenOnNFC=0;
        }
      }
    }

    /* Blinking the Led */
    if(BlinkLed) {
      BlinkLed = 0;
      BSP_LED_Toggle(LED_GREEN);
    }

    /* Wait next event */
    __WFI();
  }
}


/** @brief Write the NDEF values for NFC BLE pairing
 * @param None
 * @retval None
 */
static void BLE_Pairing_ST25DV(void)
{

  sURI_Info URI;
  /* Prepare URI NDEF message content */
  strcpy( URI.protocol,"\0" );
  sprintf(URI.URI_Message,"stapplication://connect?Pin=%d&Add=%02x:%02x:%02x:%02x:%02x:%02x",
                       BLE_StackValue.SecurePIN,
                       BLE_StackValue.BleMacAddress[5],
                       BLE_StackValue.BleMacAddress[4],
                       BLE_StackValue.BleMacAddress[3],
                       BLE_StackValue.BleMacAddress[2],
                       BLE_StackValue.BleMacAddress[1],
                       BLE_StackValue.BleMacAddress[0]);
  strcpy( URI.Information,"\0" );
  /* Write NDEF to EEPROM */
  while( NDEF_WriteURI( &URI ) != NDEF_OK );
  STBOX1_PRINTF("ST25DV Bluetooth NDEF Table written\r\n");
}

/**
* @brief  Initialize Timers
*
* @param  None
* @retval None
*/
static void InitTimers(void)
{
  uint32_t uwPrescalerValue;
  
  /* Timer Output Compare Configuration Structure declaration */
  TIM_OC_InitTypeDef sConfig;
  
  /* Compute the prescaler value to counter clock equal to 10000 Hz */
  uwPrescalerValue = (uint32_t) ((SystemCoreClock / 10000) - 1);
  
  /* Set TIM1 instance */
  TimCCHandle.Instance = TIM1;
  TimCCHandle.Init.Period        = 65535;
  TimCCHandle.Init.Prescaler     = uwPrescalerValue;
  TimCCHandle.Init.ClockDivision = 0;
  TimCCHandle.Init.CounterMode   = TIM_COUNTERMODE_UP;
  
   if(HAL_TIM_OC_DeInit(&TimCCHandle) != HAL_OK) {
    /* Initialization Error */
    Error_Handler(STBOX1_ERROR_HW_INIT,__FILE__,__LINE__);
  }
  
  if(HAL_TIM_OC_Init(&TimCCHandle) != HAL_OK) {
    /* Initialization Error */
    Error_Handler(STBOX1_ERROR_HW_INIT,__FILE__,__LINE__);
  }
  
  /* Configure the Output Compare channels */
  
  /* Common configuration for all channels */
  sConfig.OCMode     = TIM_OCMODE_TOGGLE;
  sConfig.OCPolarity = TIM_OCPOLARITY_LOW;
  
  sConfig.Pulse = STBOX1_UPDATE_LED;
  if(HAL_TIM_OC_ConfigChannel(&TimCCHandle, &sConfig, TIM_CHANNEL_1) != HAL_OK) {
    /* Configuration Error */
    Error_Handler(STBOX1_ERROR_HW_INIT,__FILE__,__LINE__);
  }
  
  sConfig.Pulse = STBOX1_UPDATE_ENV;
  if(HAL_TIM_OC_ConfigChannel(&TimCCHandle, &sConfig, TIM_CHANNEL_2) != HAL_OK) {
    /* Configuration Error */
    Error_Handler(STBOX1_ERROR_HW_INIT,__FILE__,__LINE__);
  }
  
  sConfig.Pulse = STBOX1_UPDATE_INV;
  if(HAL_TIM_OC_ConfigChannel(&TimCCHandle, &sConfig, TIM_CHANNEL_3) != HAL_OK) {
    /* Configuration Error */
    Error_Handler(STBOX1_ERROR_HW_INIT,__FILE__,__LINE__);
  }
}

/**
* @brief  Initialize User process
*
* @param  None
* @retval None
*/
static void User_Init(void)
{
  /* Enable Button in Interrupt mode */
  BSP_PB_Init(BUTTON_KEY, BUTTON_MODE_EXTI);
  
  /* Init the Led */
  BSP_LED_Init(LED_GREEN);
  BSP_LED_Init(LED_RED);
  BSP_LED_Init(LED_YELLOW);
  BSP_LED_Init(LED_BLUE);
  
  /* why RED is activated by default? */
  BSP_LED_Off(LED_RED);
  
#ifdef STBOX1_ENABLE_START_BIP
  TIM_Beep_Init();
#endif /* STBOX1_ENABLE_START_BIP */
  
  /* Check if we are running from Bank1 or Bank2 */
  {
    FLASH_OBProgramInitTypeDef    OBInit;
    /* Allow Access to Flash control registers and user Flash */
    HAL_FLASH_Unlock();
    /* Allow Access to option bytes sector */ 
    HAL_FLASH_OB_Unlock();
    /* Get the Dual boot configuration status */
    HAL_FLASHEx_OBGetConfig(&OBInit);
    if (((OBInit.USERConfig) & (OB_SWAP_BANK_ENABLE)) == OB_SWAP_BANK_ENABLE) {
      CurrentActiveBank= 2;
      MCR_START_HEART_BIT2();
    } else {
      CurrentActiveBank= 1;
      MCR_START_HEART_BIT();
    }
    HAL_FLASH_OB_Lock();
    HAL_FLASH_Lock();
  }

#ifdef STBOX1_ENABLE_START_BIP
  TIM_Beep_DeInit();
#endif /* STBOX1_ENABLE_START_BIP */

  BSP_COM_Init(COM1);
  
  /* Check the board Type */
  FinishGood = BSP_CheckFinishGood();
  
  //Update the Current Fw ID saved in flash if it's neceessary
  if(FinishGood==FINISHA) {
    UpdateCurrFlashBankFwIdBoardName(STBOX1A_BLUEST_SDK_FW_ID,NULL);
  } else {
    UpdateCurrFlashBankFwIdBoardName(STBOX1B_BLUEST_SDK_FW_ID,NULL);
  }
  
  InitTimers();
}

#ifdef STBOX1_ENABLE_START_BIP
  static void TIM_Beep_MspPostInit(TIM_HandleTypeDef* htim)
  {
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    if(htim->Instance==TIM1)
    {

    /* USER CODE BEGIN TIM1_MspPostInit 0 */
      __HAL_RCC_GPIOE_CLK_ENABLE();
      
    /* USER CODE END TIM1_MspPostInit 0 */
      __HAL_RCC_TIM1_CLK_ENABLE();
      /**TIM1 GPIO Configuration
      PE13     ------> TIM1_CH3
      */
      GPIO_InitStruct.Pin = SPKR_Pin;
      GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
      GPIO_InitStruct.Pull = GPIO_NOPULL;
      GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
      GPIO_InitStruct.Alternate = GPIO_AF1_TIM1;
      HAL_GPIO_Init(SPKR_GPIO_Port, &GPIO_InitStruct);

    /* USER CODE BEGIN TIM1_MspPostInit 1 */

    /* USER CODE END TIM1_MspPostInit 1 */
    }

  }

  /**
  * @brief TIM1 DeInitialization Function
  * @param None
  * @retval None
  */
  void TIM_Beep_DeInit(void)
  {
    /* Reset the Timer Used for making the Beep*/
    HAL_GPIO_DeInit(SPKR_GPIO_Port,SPKR_Pin);
    __HAL_RCC_TIM1_FORCE_RESET();
    HAL_Delay(10);
    __HAL_RCC_TIM1_RELEASE_RESET();
    HAL_Delay(10);
  }

  /**
  * @brief TIM1 Initialization Function
  * @param None
  * @retval None
  */
  static void TIM_Beep_Init(void)
  {
    
    /* USER CODE BEGIN TIM1_Init 0 */
    
    /* USER CODE END TIM1_Init 0 */
    
    TIM_ClockConfigTypeDef sClockSourceConfig = {0};
    TIM_MasterConfigTypeDef sMasterConfig = {0};
    TIM_OC_InitTypeDef sConfigOC = {0};
    TIM_BreakDeadTimeConfigTypeDef sBreakDeadTimeConfig = {0};
    
    /* USER CODE BEGIN TIM1_Init 1 */
    
    /* USER CODE END TIM1_Init 1 */
    TimCCHandle.Instance = TIM1;
    TimCCHandle.Init.Prescaler = 1000;
    TimCCHandle.Init.CounterMode = TIM_COUNTERMODE_UP;
    TimCCHandle.Init.Period = 65535;
    TimCCHandle.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    TimCCHandle.Init.RepetitionCounter = 0;
    TimCCHandle.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
    if (HAL_TIM_Base_Init(&TimCCHandle) != HAL_OK)
    {
      while(1);
    }
    sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
    if (HAL_TIM_ConfigClockSource(&TimCCHandle, &sClockSourceConfig) != HAL_OK)
    {
      while(1);
    }
    if (HAL_TIM_PWM_Init(&TimCCHandle) != HAL_OK)
    {
      while(1);
    }
    sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
    sMasterConfig.MasterOutputTrigger2 = TIM_TRGO2_RESET;
    sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
    if (HAL_TIMEx_MasterConfigSynchronization(&TimCCHandle, &sMasterConfig) != HAL_OK)
    {
      while(1);
    }
    sConfigOC.OCMode = TIM_OCMODE_PWM1;
    sConfigOC.Pulse = 0;
    sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
    sConfigOC.OCNPolarity = TIM_OCNPOLARITY_HIGH;
    sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
    sConfigOC.OCIdleState = TIM_OCIDLESTATE_RESET;
    sConfigOC.OCNIdleState = TIM_OCNIDLESTATE_RESET;
    if (HAL_TIM_PWM_ConfigChannel(&TimCCHandle, &sConfigOC, TIM_CHANNEL_3) != HAL_OK)
    {
      while(1);
    }
    sBreakDeadTimeConfig.OffStateRunMode = TIM_OSSR_DISABLE;
    sBreakDeadTimeConfig.OffStateIDLEMode = TIM_OSSI_DISABLE;
    sBreakDeadTimeConfig.LockLevel = TIM_LOCKLEVEL_OFF;
    sBreakDeadTimeConfig.DeadTime = 0;
    sBreakDeadTimeConfig.BreakState = TIM_BREAK_DISABLE;
    sBreakDeadTimeConfig.BreakPolarity = TIM_BREAKPOLARITY_HIGH;
    sBreakDeadTimeConfig.BreakFilter = 0;
    sBreakDeadTimeConfig.BreakAFMode = TIM_BREAK_AFMODE_INPUT;
    sBreakDeadTimeConfig.Break2State = TIM_BREAK2_DISABLE;
    sBreakDeadTimeConfig.Break2Polarity = TIM_BREAK2POLARITY_HIGH;
    sBreakDeadTimeConfig.Break2Filter = 0;
    sBreakDeadTimeConfig.Break2AFMode = TIM_BREAK_AFMODE_INPUT;
    sBreakDeadTimeConfig.AutomaticOutput = TIM_AUTOMATICOUTPUT_DISABLE;
    if (HAL_TIMEx_ConfigBreakDeadTime(&TimCCHandle, &sBreakDeadTimeConfig) != HAL_OK)
    {
     while(1);
    }
    /* USER CODE BEGIN TIM1_Init 2 */
    
    /* USER CODE END TIM1_Init 2 */
    TIM_Beep_MspPostInit(&TimCCHandle);
    
  }

  static void beep(uint32_t freq, uint16_t time)
  {
    TIM_OC_InitTypeDef sConfigOC = {0};
    uint16_t period = (SystemCoreClock/100)/freq-1;
    uint16_t DutyCycle = period/2;
    
    if(freq==0){ 
      HAL_TIM_PWM_Stop(&TimCCHandle, TIM_CHANNEL_3); 
      return; //speaker off
    }

    TimCCHandle.Instance = TIM1;
    TimCCHandle.Init.Prescaler = 63; //because 64MHz of timer clock, to have 1MHz of clock freq
    TimCCHandle.Init.CounterMode = TIM_COUNTERMODE_UP;
    TimCCHandle.Init.Period = period;
    TimCCHandle.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    TimCCHandle.Init.RepetitionCounter = 0;
    TimCCHandle.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
    if (HAL_TIM_PWM_Init(&TimCCHandle) != HAL_OK)
    {
     while(1);
    }
    
    sConfigOC.OCMode = TIM_OCMODE_PWM1;
    sConfigOC.Pulse = DutyCycle;
    sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
    sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
    HAL_TIM_PWM_ConfigChannel(&TimCCHandle, &sConfigOC, TIM_CHANNEL_3);
    HAL_TIM_PWM_Start(&TimCCHandle, TIM_CHANNEL_3);     
    if(time){ //don't stop beep if time=0
      HAL_Delay(time); 
      HAL_TIM_PWM_Stop(&TimCCHandle, TIM_CHANNEL_3);
    }
  }
#endif /* STBOX1_ENABLE_START_BIP */

/**
* @brief  Output Compare callback in non blocking mode
* @param  TIM_HandleTypeDef *htim TIM OC handle
* @retval None
*/
void HAL_TIM_OC_DelayElapsedCallback(TIM_HandleTypeDef *htim)
{
  uint32_t uhCapture=0;
  /* TIM1_CH1 toggling with frequency = 0.5Hz */
  if(htim->Channel == HAL_TIM_ACTIVE_CHANNEL_1) {
    uhCapture = HAL_TIM_ReadCapturedValue(htim, TIM_CHANNEL_1);
    /* Set the Capture Compare Register value */
    __HAL_TIM_SET_COMPARE(&TimCCHandle, TIM_CHANNEL_1, (uhCapture + STBOX1_UPDATE_LED));
    BlinkLed =1;
  }
  
  /* TIM1_CH2 toggling with frequency = 1Hz */
  if(htim->Channel == HAL_TIM_ACTIVE_CHANNEL_2) {
    uhCapture = HAL_TIM_ReadCapturedValue(htim, TIM_CHANNEL_2);
    /* Set the Capture Compare Register value */
    __HAL_TIM_SET_COMPARE(&TimCCHandle, TIM_CHANNEL_2, (uhCapture + STBOX1_UPDATE_ENV));
    SendEnv=1;
  }
  
  /* TIM1_CH2 toggling with frequency = 1Hz */
  if(htim->Channel == HAL_TIM_ACTIVE_CHANNEL_3) {
    uhCapture = HAL_TIM_ReadCapturedValue(htim, TIM_CHANNEL_3);
    /* Set the Capture Compare Register value */
    __HAL_TIM_SET_COMPARE(&TimCCHandle, TIM_CHANNEL_3, (uhCapture + STBOX1_UPDATE_INV));
    if(W2ST_CHECK_CONNECTION(W2ST_CONNECT_QUAT_EVENT)) {
      SendQuat=1;
    } else {
      SendAccGyroMag=1;
    }
  }
}


/**
* @brief  Set random values for all environmental sensor data
*
* @param  float pointer to temperature data
* @param  float pointer to pressure data
* @retval None
*/
void Set_Random_Environmental_Values(float *data_t, float *data_p)
{
  *data_t = 27.0 + ((uint64_t)rand()*5)/RAND_MAX;     /* T sensor emulation */
  *data_p = 1000.0 + ((uint64_t)rand()*80)/RAND_MAX;  /* P sensor emulation */
}

/**
* @brief  Set random values for all motion sensor data
*
* @param  BLE_MANAGER_INERTIAL_Axes_t *x_axes Acc Value
* @param  BLE_MANAGER_INERTIAL_Axes_t *g_axes Gyro Value
* @param  BLE_MANAGER_INERTIAL_Axes_t * m_axes Mag Value
* @param  BLE_MOTION_SENSOR_Axes_t *q_axes  Quaternion Value
* @param  uint32_t counter for changing the rotation direction
* @retval None
*/
static void Set_Random_Motion_Values(BLE_MANAGER_INERTIAL_Axes_t *x_axes, 
                                     BLE_MANAGER_INERTIAL_Axes_t *g_axes, 
                                     BLE_MANAGER_INERTIAL_Axes_t * m_axes, 
                                     BLE_MOTION_SENSOR_Axes_t *q_axes,
                                     uint32_t cnt)
{
  /* Update Acceleration, Gyroscope and Sensor Fusion data */
  if (cnt < 20) {
    x_axes->Axis_x +=  (10  + ((uint64_t)rand()*3*cnt)/RAND_MAX);
    x_axes->Axis_y += -(10  + ((uint64_t)rand()*5*cnt)/RAND_MAX);
    x_axes->Axis_z +=  (10  + ((uint64_t)rand()*7*cnt)/RAND_MAX);
    g_axes->Axis_x +=  (100 + ((uint64_t)rand()*2*cnt)/RAND_MAX);
    g_axes->Axis_y += -(100 + ((uint64_t)rand()*4*cnt)/RAND_MAX);
    g_axes->Axis_z +=  (100 + ((uint64_t)rand()*6*cnt)/RAND_MAX);
    m_axes->Axis_x +=  (3  + ((uint64_t)rand()*3*cnt)/RAND_MAX);
    m_axes->Axis_y += -(3  + ((uint64_t)rand()*4*cnt)/RAND_MAX);
    m_axes->Axis_z +=  (3  + ((uint64_t)rand()*5*cnt)/RAND_MAX);
    
    q_axes->Axis_x -= (100  + ((uint64_t)rand()*3*cnt)/RAND_MAX);
    q_axes->Axis_y += (100  + ((uint64_t)rand()*5*cnt)/RAND_MAX);
    q_axes->Axis_z -= (100  + ((uint64_t)rand()*7*cnt)/RAND_MAX);
  } else {
    x_axes->Axis_x += -(10  + ((uint64_t)rand()*3*cnt)/RAND_MAX);
    x_axes->Axis_y +=  (10  + ((uint64_t)rand()*5*cnt)/RAND_MAX);
    x_axes->Axis_z += -(10  + ((uint64_t)rand()*7*cnt)/RAND_MAX);
    g_axes->Axis_x += -(100 + ((uint64_t)rand()*2*cnt)/RAND_MAX);
    g_axes->Axis_y +=  (100 + ((uint64_t)rand()*4*cnt)/RAND_MAX);
    g_axes->Axis_z += -(100 + ((uint64_t)rand()*6*cnt)/RAND_MAX);
    m_axes->Axis_x += -(3  + ((uint64_t)rand()*7*cnt)/RAND_MAX);
    m_axes->Axis_y +=  (3  + ((uint64_t)rand()*9*cnt)/RAND_MAX);
    m_axes->Axis_z += -(3  + ((uint64_t)rand()*3*cnt)/RAND_MAX);
    
    q_axes->Axis_x += (200 + ((uint64_t)rand()*7*cnt)/RAND_MAX);
    q_axes->Axis_y -= (150 + ((uint64_t)rand()*3*cnt)/RAND_MAX);
    q_axes->Axis_z += (10  + ((uint64_t)rand()*5*cnt)/RAND_MAX);
  }
}

/**
* @brief  Reset values for all motion sensor data
*
* @param  BLE_MANAGER_INERTIAL_Axes_t *x_axes Acc Value
* @param  BLE_MANAGER_INERTIAL_Axes_t *g_axes Gyro Value
* @param  BLE_MANAGER_INERTIAL_Axes_t * m_axes Mag Value
* @param  BLE_MOTION_SENSOR_Axes_t *q_axes  Quaternion Value
* @retval None
*/
static void Reset_Motion_Values(BLE_MANAGER_INERTIAL_Axes_t *x_axes, 
                               BLE_MANAGER_INERTIAL_Axes_t *g_axes, 
                               BLE_MANAGER_INERTIAL_Axes_t * m_axes, 
                               BLE_MOTION_SENSOR_Axes_t *q_axes)
{
  x_axes->Axis_x = (x_axes->Axis_x)%2000 == 0 ? -x_axes->Axis_x : 10;
  x_axes->Axis_y = (x_axes->Axis_y)%2000 == 0 ? -x_axes->Axis_y : -10;
  x_axes->Axis_z = (x_axes->Axis_z)%2000 == 0 ? -x_axes->Axis_z : 10;
  g_axes->Axis_x = (g_axes->Axis_x)%2000 == 0 ? -g_axes->Axis_x : 100;
  g_axes->Axis_y = (g_axes->Axis_y)%2000 == 0 ? -g_axes->Axis_y : -100;
  g_axes->Axis_z = (g_axes->Axis_z)%2000 == 0 ? -g_axes->Axis_z : 100;
  m_axes->Axis_x = (g_axes->Axis_x)%2000 == 0 ? -m_axes->Axis_x : 3;
  m_axes->Axis_y = (g_axes->Axis_y)%2000 == 0 ? -m_axes->Axis_y : -3;
  m_axes->Axis_z = (g_axes->Axis_z)%2000 == 0 ? -m_axes->Axis_z : 3;
  q_axes->Axis_x = -q_axes->Axis_x;
  q_axes->Axis_y = -q_axes->Axis_y;
  q_axes->Axis_z = -q_axes->Axis_z;
}

/**
* @brief  BSP Push Button callback
*
* @param  Button Specifies the pin connected EXTI line
* @retval None
*/
void BSP_PB_Callback(Button_TypeDef Button)
{
  /* Set the User Button flag */
  user_button_pressed = 1;
}

/**
* @brief  Clear Secure Database
* @param  None
* @retval None
*/
static void ClearSecureDB(void)
{
  tBleStatus ret = aci_gap_clear_security_db();
  
  if (ret != BLE_STATUS_SUCCESS) {
    STBOX1_PRINTF("aci_gap_clear_security_db failed:0x%02x\r\n", ret);
  }  else {
    STBOX1_PRINTF("aci_gap_clear_security_db\r\n");
  }
  
  /* Led Blinking */
  if (CurrentActiveBank==2) {
    MCR_HEART_BIT2();
  } else {
    MCR_HEART_BIT();
  }
}

/**
* @brief  Print Bunner
* @param  None
* @retval None
*/
static void PrintInfo(void)
{
  if(FinishGood==FINISH_ERROR) {
   Error_Handler(STBOX1_ERROR_HW_INIT,__FILE__,__LINE__);
  }
  
  STBOX1_PRINTF("\r\nSTMicroelectronics %s:\r\n"
            "\tVersion %c.%c.%c\r\n"
              "\tSTM32U585AI-SensorTile.box-Pro (%c) board"
                "\r\n",
                STBOX1_PACKAGENAME,
                STBOX1_VERSION_MAJOR,STBOX1_VERSION_MINOR,STBOX1_VERSION_PATCH,
                  (FinishGood==FINISHA) ? 'A' : 'B');
  STBOX1_PRINTF("\t(HAL %ld.%ld.%ld_%ld)\r\n"
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
   STBOX1_PRINTF("Current Bank =%ld\r\n",CurrentActiveBank);
}


/**
* @brief  write one URI on NDEF
* @param  Nne
* @retval None
*/
void NDEF_URI_Init(void)
{
  sURI_Info URI;

  /* Init ST25DV driver */
  while( BSP_NFCTAG_Init(BSP_NFCTAG_INSTANCE) != NFCTAG_OK );

  /* Reset Mailbox enable to allow write to EEPROM */
  BSP_NFCTAG_ResetMBEN_Dyn(BSP_NFCTAG_INSTANCE);

  NfcTag_SelectProtocol(NFCTAG_TYPE5);

  /* Check if no NDEF detected, init mem in Tag Type 5 */
  if( NfcType5_NDEFDetection( ) != NDEF_OK )
  {
    CCFileStruct.MagicNumber = NFCT5_MAGICNUMBER_E1_CCFILE;
    CCFileStruct.Version = NFCT5_VERSION_V1_0;
    CCFileStruct.MemorySize = ( ST25DV_MAX_SIZE / 8 ) & 0xFF;
    CCFileStruct.TT5Tag = 0x05;
    /* Init of the Type Tag 5 component (M24LR) */
    while( NfcType5_TT5Init( ) != NFCTAG_OK );
  }

  /* Prepare URI NDEF message content */
  strcpy( URI.protocol,URI_ID_0x01_STRING );
  strcpy( URI.URI_Message,"st.com" );
  strcpy( URI.Information,"\0" );

  /* Write NDEF to EEPROM */
  while( NDEF_WriteURI( &URI ) != NDEF_OK );

  STBOX1_PRINTF("Written on NFC Uri=st.com\r\n");
}

/**
  * @brief System Clock Configuration 128MHz
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

  /** Configure the main internal regulator output voltage
  */
  if (HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1) != HAL_OK)
  {
    Error_Handler(STBOX1_ERROR_HW_INIT,__FILE__,__LINE__);
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI48|RCC_OSCILLATORTYPE_HSI
                              |RCC_OSCILLATORTYPE_LSI|RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSI48State = RCC_HSI48_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.LSIState = RCC_LSI_ON;
  RCC_OscInitStruct.LSIDiv = RCC_LSI_DIV1;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMBOOST = RCC_PLLMBOOST_DIV1;
  RCC_OscInitStruct.PLL.PLLM = 1;
  RCC_OscInitStruct.PLL.PLLN = 10;
  RCC_OscInitStruct.PLL.PLLP = 1;
  RCC_OscInitStruct.PLL.PLLQ = 2;
  RCC_OscInitStruct.PLL.PLLR = 1;
  RCC_OscInitStruct.PLL.PLLRGE = RCC_PLLVCIRANGE_1;
  RCC_OscInitStruct.PLL.PLLFRACN = 0;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler(STBOX1_ERROR_HW_INIT,__FILE__,__LINE__);
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2
                              |RCC_CLOCKTYPE_PCLK3;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB3CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4) != HAL_OK)
  {
    Error_Handler(STBOX1_ERROR_HW_INIT,__FILE__,__LINE__);
  }
  
    /** Initializes the common periph clock
  */
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_MDF1|RCC_PERIPHCLK_ADF1
                              |RCC_PERIPHCLK_ADCDAC;
  PeriphClkInit.Mdf1ClockSelection = RCC_MDF1CLKSOURCE_PLL3;
  PeriphClkInit.Adf1ClockSelection = RCC_ADF1CLKSOURCE_PLL3;
  PeriphClkInit.AdcDacClockSelection = RCC_ADCDACCLKSOURCE_PLL2;
  PeriphClkInit.PLL3.PLL3Source = RCC_PLLSOURCE_HSE;
  PeriphClkInit.PLL3.PLL3M = 2;
  PeriphClkInit.PLL3.PLL3N = 48;
  PeriphClkInit.PLL3.PLL3P = 2;
  PeriphClkInit.PLL3.PLL3Q = 25;
  PeriphClkInit.PLL3.PLL3R = 2;
  PeriphClkInit.PLL3.PLL3RGE = RCC_PLLVCIRANGE_1;
  PeriphClkInit.PLL3.PLL3FRACN = 0;
  PeriphClkInit.PLL3.PLL3ClockOut = RCC_PLL3_DIVQ;
  PeriphClkInit.PLL2.PLL2Source = RCC_PLLSOURCE_HSE;
  PeriphClkInit.PLL2.PLL2M = 2;
  PeriphClkInit.PLL2.PLL2N = 48;
  PeriphClkInit.PLL2.PLL2P = 2;
  PeriphClkInit.PLL2.PLL2Q = 7;
  PeriphClkInit.PLL2.PLL2R = 25;
  PeriphClkInit.PLL2.PLL2RGE = RCC_PLLVCIRANGE_1;
  PeriphClkInit.PLL2.PLL2FRACN = 0;
  PeriphClkInit.PLL2.PLL2ClockOut = RCC_PLL2_DIVR;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Error_Handler(STBOX1_ERROR_HW_INIT,__FILE__,__LINE__);
  }
}

/**
  * @brief ICACHE Initialization Function
  * @param None
  * @retval None
  */
static void MX_ICACHE_Init(void)
{
  /** Enable instruction cache in 1-way (direct mapped cache)
  */
  if (HAL_ICACHE_ConfigAssociativityMode(ICACHE_1WAY) != HAL_OK){
    Error_Handler(STBOX1_ERROR_HW_INIT,__FILE__,__LINE__);
  }
  if (HAL_ICACHE_Enable() != HAL_OK){
    Error_Handler(STBOX1_ERROR_HW_INIT,__FILE__,__LINE__);
  }

}

/**
  * @brief  This function is executed in case of error occurrence.
  * @param int32_t ErrorCode Error Code
  * @retval None
  */
void Error_Handler(int32_t ErrorCode,char *File,int32_t Line)
{
  /* User can add his own implementation to report the HAL error return state */
  BSP_LED_Off(LED_RED);
  STBOX1_PRINTF("Error at %d at %s\r\n",Line,File);
  while (1){
    int count;
    for(count=0;count<ErrorCode;count++) {
      BSP_LED_On(LED_RED);
      HAL_Delay(500);
      BSP_LED_Off(LED_RED);
      HAL_Delay(2000);
    }
    BSP_LED_On(LED_GREEN);
    BSP_LED_On(LED_YELLOW);
    HAL_Delay(2000);
    BSP_LED_Off(LED_GREEN);
    BSP_LED_Off(LED_YELLOW);
  }
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
}
#endif /* USE_FULL_ASSERT */

