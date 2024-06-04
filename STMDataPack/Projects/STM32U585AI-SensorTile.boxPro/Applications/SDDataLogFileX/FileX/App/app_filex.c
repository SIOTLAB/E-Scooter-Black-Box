/**
 ******************************************************************************
 * @file    SDDataLogFileX\FileX\App\app_filex.c
 * @author  System Research & Applications Team - Catania Lab.
 * @version V1.6.0
 * @date    20-Oct-2023
 * @brief   FileX applicative file
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
#include "app_filex.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>
#include "STBOX1_config.h"
#include "SensorTileBoxPro.h"
#include "SensorTileBoxPro_env_sensors.h"
#include "SensorTileBoxPro_motion_sensors.h"
#include "SensorTileBoxPro_audio.h"
#include "main.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
typedef struct
{
  UINT CommandType;
  ULONG MsgTime;
  float pressure;
  float temperature;
  BSP_MOTION_SENSOR_Axes_t acc;
  BSP_MOTION_SENSOR_Axes_t gyro;
  BSP_MOTION_SENSOR_Axes_t mag;
} MessageData_T;
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

#define DEFAULT_STACK_SIZE (2 * 1024)

/* fx_app_thread priority */
#define FX_APP_THREAD_PRIO 12

/* read_app_thread priority */
#define READ_APP_THREAD_PRIO 10

/* fx_app_thread preemption priority */
#define FX_APP_PREEMPTION_THRESHOLD FX_APP_THREAD_PRIO

/* read_app_thread preemption priority */
#define READ_APP_PREEMPTION_THRESHOLD READ_APP_THREAD_PRIO

#define MESSAGE_QUEUE_SIZE 100

#define COMMAND_STOP_LOG 0
#define COMMAND_START_LOG 1
#define COMMAND_SAVE_AUDIO 2
#define COMMAND_SAVE_SENSORS 3

#define STBOX1_AUDIO_DATA_NOT_READY 0xFFFFFF

#define PCM_AUDIO_IN_SAMPLES (AUDIO_IN_SAMPLING_FREQUENCY / 1000)
#define AUDIO_BUFF_SIZE (PCM_AUDIO_IN_SAMPLES * 1024)

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN PV */

/* Buffer for FileX FX_MEDIA sector cache. this should be 32-Bytes
aligned to avoid cache maintenance issues */
ALIGN_32BYTES(uint32_t media_memory[FX_STM32_SD_DEFAULT_SECTOR_SIZE / sizeof(uint32_t)]);

/* Define FileX global data structures.  */
FX_MEDIA sdio_disk;
FX_FILE SensorsFxFile;
FX_FILE AudioFxFile;
/* Define ThreadX global data structures.  */
TX_THREAD fx_app_thread;
TX_THREAD read_app_thread;
TX_QUEUE MessageQueue;

/* Timer for reading the sensor's Data*/
TX_TIMER ReadTimer;

/* Semaphore for controlling the Reading Thread */
TX_SEMAPHORE SemaphorePtr;

/* Array for Sensors Values */
static UCHAR SensorBuffer[MESSAGE_QUEUE_SIZE * sizeof(MessageData_T)];
static UCHAR *SensorBufferWritingPointer = SensorBuffer;
static UCHAR *SensorBufferEndPointer = SensorBuffer + MESSAGE_QUEUE_SIZE * sizeof(MessageData_T);

/* Output PCM buffer from Digital Microphone */
uint16_t OnBoard_PCM_Buffer[2 * PCM_AUDIO_IN_SAMPLES];

static uint16_t Audio_OUT_Buff[AUDIO_BUFF_SIZE];
static volatile uint32_t WriteIndexBufferAudio = 0;
static volatile uint32_t ReadIndexBufferAudio = STBOX1_AUDIO_DATA_NOT_READY;
static volatile int SkipFirst200mS;

/* Header for wav audio file */
static uint8_t pAudioHeader[44];

static volatile CHAR SensorsFileOpen = 0;
static volatile CHAR AudioFileOpen = 0;

/* For Understanding Max Message Queues size */
INT MessagePushed = 0;
INT MessageRemoved = 0;
INT MessageMaxSize = 0;

/* USER CODE END PV */

static volatile uint32_t UserButtonPressed = 0;

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN PFP */

static void fx_thread_entry(ULONG thread_input);
static void read_thread_entry(ULONG thread_input);
static void ReadingTimerCallbackFunction(ULONG timer);
static void AudioProcess_SD_Recording(uint16_t *pInBuff, uint32_t len);
static uint32_t WavProcess_HeaderInit(void);
static uint32_t WavProcess_HeaderUpdate(uint32_t len);
/* USER CODE END PFP */

/**
 * @brief  Application FileX Initialization.
 * @param memory_ptr: memory pointer
 * @retval int
 */
UINT MX_FileX_Init(VOID *memory_ptr)
{
  UINT ret = FX_SUCCESS;
  TX_BYTE_POOL *byte_pool = (TX_BYTE_POOL *)memory_ptr;

  /* USER CODE BEGIN App_FileX_MEM_POOL */
  (void)byte_pool;
  /* USER CODE END App_FileX_MEM_POOL */

  /* USER CODE BEGIN MX_FileX_Init */

  VOID *pointer;

  /* Allocate memory for the writing thread's stack */
  ret = tx_byte_allocate(byte_pool, &pointer, DEFAULT_STACK_SIZE * 2, TX_NO_WAIT);

  if (ret != FX_SUCCESS)
  {
    /* Failed at allocating memory */
    Error_Handler(__FILE__, __LINE__);
  }

  /* Create the writing thread.  */
  tx_thread_create(&fx_app_thread, "FileX Writing App Thread", fx_thread_entry, 0, pointer, DEFAULT_STACK_SIZE * 2,
                   FX_APP_THREAD_PRIO, FX_APP_PREEMPTION_THRESHOLD, TX_NO_TIME_SLICE, TX_AUTO_START);

  /* Initialize FileX.  */
  fx_system_initialize();

  STBOX1_PRINTF("FileX Thread Created\r\n");

  /* Allocate memory for the reading thread's stack */
  ret = tx_byte_allocate(byte_pool, &pointer, DEFAULT_STACK_SIZE * 2, TX_NO_WAIT);

  if (ret != FX_SUCCESS)
  {
    /* Failed at allocating memory */
    Error_Handler(__FILE__, __LINE__);
  }

  /* Create the reading thread.  */
  tx_thread_create(&read_app_thread, "Reading Sensor App Thread", read_thread_entry, 0, pointer, DEFAULT_STACK_SIZE * 2,
                   READ_APP_THREAD_PRIO, READ_APP_PREEMPTION_THRESHOLD, TX_NO_TIME_SLICE, TX_AUTO_START);

  STBOX1_PRINTF("Read Sensor Thread Created\r\n");

  /* Allocate memory for MessageQueue */
  ret = tx_byte_allocate(byte_pool, &pointer, MESSAGE_QUEUE_SIZE * sizeof(MessageData_T *), TX_NO_WAIT);

  if (ret != FX_SUCCESS)
  {
    /* Failed at allocating memory */
    Error_Handler(__FILE__, __LINE__);
  }

  /* Create the MessageQueue shared by Reading and Writing Thread */
  if (tx_queue_create(&MessageQueue, "Message Queue", sizeof(MessageData_T *) / 4,
                      pointer, MESSAGE_QUEUE_SIZE * sizeof(MessageData_T *)) != TX_SUCCESS)
  {
    ret = TX_QUEUE_ERROR;
  }

  if (ret != FX_SUCCESS)
  {
    /* Failed at allocating Queue */
    Error_Handler(__FILE__, __LINE__);
  }

  STBOX1_PRINTF("MessageQueue Created\r\n");

  /* Create the tx_timer */
  if (tx_timer_create(
          &ReadTimer,
          "ReadingTimer",
          ReadingTimerCallbackFunction,
          (ULONG)TX_NULL,
          1 /* 10ms */,
          1 /* 10ms */,
          TX_NO_ACTIVATE) != TX_SUCCESS)
  {
    Error_Handler(__FILE__, __LINE__);
  }

  STBOX1_PRINTF("ReadingTimer Created:\r\n\tRead Sensors@100Hz\r\n\tAudio 48KHz\r\n");

  if (tx_semaphore_create(&SemaphorePtr, "Semaphore for Sensor Reading", 0 /* Intial value */) != TX_SUCCESS)
  {
    Error_Handler(__FILE__, __LINE__);
  }

  STBOX1_PRINTF("SemaphorePtr Created\r\n");

  STBOX1_PRINTF("System Ready...\r\n\tPress User Button\r\n\tfor Starting/Stopping SD Log\r\n");

  /* USER CODE END MX_FileX_Init */

  return ret;
}

/* USER CODE BEGIN 1 */

static void ReadingTimerCallbackFunction(ULONG timer)
{
  /* Release the Semaphore */
  tx_semaphore_put(&SemaphorePtr);
}

static void fx_thread_entry(ULONG thread_input)
{

  UINT status;
  MessageData_T *RMsg;
  CHAR header[] = "Time [mS], AccX [mg],AccY [mg],AccZ [mg],GyroX [mdps],GyroY [mdps],GyroZ [mdps],MagX [mgauss],MagY [mgauss],MagZ [mgauss]\r\n";

  SHORT SDCardCounter = 0;
  CHAR file_name[30];

  while (1)
  {
    /* Determine whether a message MessageQueue  is available */
    status = tx_queue_receive(&MessageQueue, &RMsg, TX_WAIT_FOREVER);
    MessageRemoved++;
    if (status == TX_SUCCESS)
    {
      switch (RMsg->CommandType)
      {
      case COMMAND_START_LOG:
      {
        BSP_LED_Off(LED_RED);

        /* Init the Queue Statics */
        MessagePushed = 0;
        MessageRemoved = 0;
        MessageMaxSize = 0;

        /* Reset the Sensors and Mic out Buffers */
        SensorBufferWritingPointer = SensorBuffer;
        WriteIndexBufferAudio = 0;
        ReadIndexBufferAudio = STBOX1_AUDIO_DATA_NOT_READY;
        SkipFirst200mS = 200;

        if (SensorsFileOpen == 0)
        {
          sprintf(file_name, "Sens%03d.csv", SDCardCounter);
          SDCardCounter++;

          /* Open the SD disk driver.  */
          status = fx_media_open(&sdio_disk, "STM32_SDIO_DISK", fx_stm32_sd_driver, 0, (VOID *)media_memory, sizeof(media_memory));

          /* Check the media open status.  */
          if (status != FX_SUCCESS)
          {
            STBOX1_PRINTF("Error opening SD Fx Media\r\n");
            Error_Handler(__FILE__, __LINE__);
          }

          STBOX1_PRINTF("SD FX Media Open\r\n");

          /* Create a file in the root directory.  */
          status = fx_file_create(&sdio_disk, file_name);

          /* Check the create status.  */
          if (status != FX_SUCCESS)
          {
            /* Check for an already created status */
            if (status != FX_ALREADY_CREATED)
            {
              /* Create error, call error handler.  */
              STBOX1_PRINTF("Error creating file %s\r\n", file_name);
              Error_Handler(__FILE__, __LINE__);
            }
            else
            {
              /* Searching one available File */
              while (status == FX_ALREADY_CREATED)
              {
                sprintf(file_name, "Sens%03d.csv", SDCardCounter);
                SDCardCounter++;
                status = fx_file_create(&sdio_disk, file_name);
                /* Check the create status.  */
                if (status != FX_SUCCESS)
                {
                  /* Check for an already created status */
                  if (status != FX_ALREADY_CREATED)
                  {
                    /* Create error, call error handler.  */
                    STBOX1_PRINTF("Error creating file %s\r\n", file_name);
                    Error_Handler(__FILE__, __LINE__);
                  }
                }
              }
            }
          }

          /* Open the test file.  */
          status = fx_file_open(&sdio_disk, &SensorsFxFile, file_name, FX_OPEN_FOR_WRITE);

          /* Check the file open status.  */
          if (status != FX_SUCCESS)
          {
            /* Error opening file, call error handler.  */
            STBOX1_PRINTF("Error opening SensXXX.csv\r\n");
            Error_Handler(__FILE__, __LINE__);
          }

          /* Seek to the beginning of the test file.  */
          status = fx_file_seek(&SensorsFxFile, 0);

          /* Check the file seek status.  */
          if (status != FX_SUCCESS)
          {
            /* Error performing file seek, call error handler.  */
            Error_Handler(__FILE__, __LINE__);
          }
          SensorsFileOpen = 1;
          STBOX1_PRINTF("File %s open\r\n", file_name);

          /* Write a string to the test file.  */
          status = fx_file_write(&SensorsFxFile, header, sizeof(header) - 1);

          /* Check the file write status.  */
          if (status != FX_SUCCESS)
          {
            /* Error writing to a file, call error handler.  */
            STBOX1_PRINTF("Error writing SensXXX.csv\r\n");
            Error_Handler(__FILE__, __LINE__);
          }

          if (tx_timer_activate(&ReadTimer) != TX_SUCCESS)
          {
            STBOX1_PRINTF("Error activating the TX Timer\r\n");
            Error_Handler(__FILE__, __LINE__);
          }
        }
        else
        {
          STBOX1_PRINTF("Error SensXXX.csv already opened\r\n");
        }

        if (AudioFileOpen == 0)
        {
          AudioFileOpen = 1;
        }
        else
        {
          STBOX1_PRINTF("Error MicXXX.wav already opened\r\n");
        }
      }
      break;
      case COMMAND_STOP_LOG:
      {
        if (SensorsFileOpen)
        {
          if (tx_timer_deactivate(&ReadTimer) != TX_SUCCESS)
          {
            Error_Handler(__FILE__, __LINE__);
          }

          /* Flush the Queue for Sensors Data */
          status = tx_queue_flush(&MessageQueue);

          if (status != FX_SUCCESS)
          {
            /* Error flushing the queue, call error handler.  */
            Error_Handler(__FILE__, __LINE__);
          }

          /* Close the test file.  */
          status = fx_file_close(&SensorsFxFile);

          /* Check the file close status.  */
          if (status != FX_SUCCESS)
          {
            /* Error closing the file, call error handler.  */
            STBOX1_PRINTF("Error closing SensXXX.csv\r\n");
            Error_Handler(__FILE__, __LINE__);
          }

          STBOX1_PRINTF("File SensXXX.csv closed\r\n");
          SensorsFileOpen = 0;
        }
        else
        {
          STBOX1_PRINTF("Error SensXXX.csv Not opened\r\n");
        }

        if (AudioFileOpen)
        {
          /* Flush the Queue for Sensors Data */
          status = tx_queue_flush(&MessageQueue);

          AudioFileOpen = 0;

          /* Close the media.  */
          status = fx_media_close(&sdio_disk);

          /* Check the media close status.  */
          if (status != FX_SUCCESS)
          {
            /* Error closing the media, call error handler.  */
            STBOX1_PRINTF("Error closing SD FX Media\r\n");
            Error_Handler(__FILE__, __LINE__);
          }

          STBOX1_PRINTF("SD FX Media Closed\r\n");

          STBOX1_PRINTF("|--------------------|\r\n");
          STBOX1_PRINTF("| Queues summary:    |\r\n");
          STBOX1_PRINTF("|--------------------|\r\n");
          STBOX1_PRINTF("|   Queue Max: %4d  |\r\n", MessageMaxSize);
          STBOX1_PRINTF("|--------------------|\r\n");
        }
        else
        {
          STBOX1_PRINTF("Error MicXXX.wav Not opened\r\n");
        }

        /* TODO: Add in some BLE code */
      }
      break;
      case COMMAND_SAVE_AUDIO:
      {
        if (AudioFileOpen == 1)
        {
          if (ReadIndexBufferAudio != STBOX1_AUDIO_DATA_NOT_READY)
          {
            status = fx_file_write(&AudioFxFile, (UCHAR *)(Audio_OUT_Buff + ReadIndexBufferAudio), AUDIO_BUFF_SIZE);
            /* Check the file write status.  */
            if (status != FX_SUCCESS)
            {
              /* Error writing to a file, call error handler.  */
              STBOX1_PRINTF("Error writing MicXXX.csv\r\n");
              Error_Handler(__FILE__, __LINE__);
            }
          }
        }
      }
      break;

      case COMMAND_SAVE_SENSORS:
      {
        if (SensorsFileOpen)
        {
          CHAR data_s[256];
          INT size;
          size = sprintf(data_s, "%ld, %d, %d, %d, %d, %d, %d, %d, %d, %d\r\n",
                         RMsg->MsgTime,
                         RMsg->acc.x, RMsg->acc.y, RMsg->acc.z,
                         RMsg->gyro.x, RMsg->gyro.y, RMsg->gyro.z,
                         RMsg->mag.x, RMsg->mag.y, RMsg->mag.z);

          /* Write a string to the test file.  */
          status = fx_file_write(&SensorsFxFile, data_s, size);

          /* Check the file write status.  */
          if (status != FX_SUCCESS)
          {
            /* Error writing to a file, call error handler.  */
            STBOX1_PRINTF("Error writing SensXXX.csv\r\n");
            Error_Handler(__FILE__, __LINE__);
          }
        }
        BSP_LED_Toggle(LED_GREEN);
      }
      break;

      default:
        STBOX1_PRINTF("Command =%d Not recognized\r\n", RMsg->CommandType);
        Error_Handler(__FILE__, __LINE__);
      }
    }
  }
}

/**
 * @brief  BSP Push Button callback
 *
 * @param  Button Specifies the pin connected EXTI line
 * @retval None
 */
void BSP_PB_Callback(Button_TypeDef Button)
{
  if (Button == BUTTON_USER)
  {
    /* Set the User Button flag */
    UserButtonPressed = SET;
    /* Release the Semaphore */
    tx_semaphore_put(&SemaphorePtr);
  }
}

static void read_thread_entry(ULONG thread_input)
{
  MessageData_T *Msg;
  INT LogCommandType = COMMAND_STOP_LOG;
  while (1)
  {
    tx_semaphore_get(&SemaphorePtr, TX_WAIT_FOREVER);

    Msg = (MessageData_T *)SensorBufferWritingPointer;
    SensorBufferWritingPointer += sizeof(MessageData_T);
    if (SensorBufferWritingPointer == SensorBufferEndPointer)
    {
      SensorBufferWritingPointer = SensorBuffer;
    }

    if (UserButtonPressed)
    {
      static ULONG ButtonPressedTime = 0;
      ULONG NewTime;
      UserButtonPressed = 0;

      NewTime = tx_time_get();
      /* For avoiding a double click */
      if ((NewTime - ButtonPressedTime) > 100)
      {
        ButtonPressedTime = NewTime;
        if (LogCommandType == COMMAND_STOP_LOG)
        {
          Msg->CommandType = LogCommandType = COMMAND_START_LOG;
        }
        else
        {
          Msg->CommandType = LogCommandType = COMMAND_STOP_LOG;
        }

        /* Send message to MessageQueue.  */
        if (tx_queue_send(&MessageQueue, &Msg, TX_WAIT_FOREVER) != TX_SUCCESS)
        {
          STBOX1_PRINTF("Error Queue Max: %4d\r\n", MessageMaxSize);
          Error_Handler(__FILE__, __LINE__);
        }
        MessagePushed++;
        if ((MessagePushed - MessageRemoved) > MessageMaxSize)
        {
          MessageMaxSize = MessagePushed - MessageRemoved;
        }
      }
    }
    else
    {
      if (SensorsFileOpen == 1)
      {
        /* Read Sensors' Value */
        Msg->CommandType = COMMAND_SAVE_SENSORS;
        Msg->MsgTime = tx_time_get();
        BSP_MOTION_SENSOR_GetAxes(LSM6DSV16X_0, MOTION_ACCELERO, &Msg->acc);
        BSP_MOTION_SENSOR_GetAxes(LSM6DSV16X_0, MOTION_GYRO, &Msg->gyro);
        BSP_MOTION_SENSOR_GetAxes(LIS2MDL_0, MOTION_MAGNETO, &Msg->mag);

        /* Send message to MessageQueue.  */
        if (tx_queue_send(&MessageQueue, &Msg, TX_WAIT_FOREVER) != TX_SUCCESS)
        {
          STBOX1_PRINTF("Error Queue Max: %4d\r\n", MessageMaxSize);
          Error_Handler(__FILE__, __LINE__);
        }
        MessagePushed++;
        if ((MessagePushed - MessageRemoved) > MessageMaxSize)
        {
          MessageMaxSize = MessagePushed - MessageRemoved;
        }
      }
    }
  }
}

/**
 * @brief Half Transfer user callback, called by BSP functions.
 * @param None
 * @retval None
 */
void BSP_AUDIO_IN_HalfTransfer_CallBack(uint32_t Instance)
{
  UNUSED(Instance);
  if (AudioFileOpen)
  {
    AudioProcess_SD_Recording(OnBoard_PCM_Buffer, PCM_AUDIO_IN_SAMPLES);
  }
}

/**
 * @brief Transfer Complete user callback, called by BSP functions.
 * @param None
 * @retval None
 */
void BSP_AUDIO_IN_TransferComplete_CallBack(uint32_t Instance)
{
  UNUSED(Instance);
  if (AudioFileOpen)
  {
    AudioProcess_SD_Recording(OnBoard_PCM_Buffer, PCM_AUDIO_IN_SAMPLES);
  }
}

/**
 * @brief  Management of the audio data logging
 * @param  pInBuff     points to the audio buffer.
 * @param  len         number of samples to process.
 * @retval None
 */
static void AudioProcess_SD_Recording(uint16_t *pInBuff, uint32_t len)
{
  if (SkipFirst200mS > 0)
  {
    /* Tmp workaround for Initial Mic glitch */
    SkipFirst200mS--;
    return;
  }
  /* Accumulate audio buffer into local ping-pong buffer before writing */
  memcpy(Audio_OUT_Buff + WriteIndexBufferAudio, pInBuff, len * sizeof(uint16_t));
  WriteIndexBufferAudio += len;

  // WriteIndexBufferAudio &=AUDIO_BUFF_SIZE_MASK;
  WriteIndexBufferAudio %= AUDIO_BUFF_SIZE;

  if (WriteIndexBufferAudio == (AUDIO_BUFF_SIZE / 2))
  {
    MessageData_T *Msg;
    Msg = (MessageData_T *)SensorBufferWritingPointer;
    SensorBufferWritingPointer += sizeof(MessageData_T);
    if (SensorBufferWritingPointer == SensorBufferEndPointer)
    {
      SensorBufferWritingPointer = SensorBuffer;
    }
    Msg->CommandType = COMMAND_SAVE_AUDIO;
    /* first half */
    ReadIndexBufferAudio = 0;

    /* Send te Message for writing out the 1/2 buffer */
    if (tx_queue_send(&MessageQueue, &Msg, TX_NO_WAIT) != TX_SUCCESS)
    {
      STBOX1_PRINTF("Error Queue Max: %4d\r\n", MessageMaxSize);
      Error_Handler(__FILE__, __LINE__);
    }
    MessagePushed++;
    if ((MessagePushed - MessageRemoved) > MessageMaxSize)
    {
      MessageMaxSize = MessagePushed - MessageRemoved;
    }
  }
  else if (WriteIndexBufferAudio == 0)
  {
    MessageData_T *Msg;
    Msg = (MessageData_T *)SensorBufferWritingPointer;
    SensorBufferWritingPointer += sizeof(MessageData_T);
    if (SensorBufferWritingPointer == SensorBufferEndPointer)
    {
      SensorBufferWritingPointer = SensorBuffer;
    }
    Msg->CommandType = COMMAND_SAVE_AUDIO;
    /* second half */
    ReadIndexBufferAudio = AUDIO_BUFF_SIZE / 2;
    /* Send te Message for writing out the 1/2 buffer */
    if (tx_queue_send(&MessageQueue, &Msg, TX_NO_WAIT) != TX_SUCCESS)
    {
      STBOX1_PRINTF("Error Queue Max: %4d\r\n", MessageMaxSize);
      Error_Handler(__FILE__, __LINE__);
    }
    MessagePushed++;
    if ((MessagePushed - MessageRemoved) > MessageMaxSize)
    {
      MessageMaxSize = MessagePushed - MessageRemoved;
    }
  }

  /* Control section */
  if (ReadIndexBufferAudio == 0)
  {
    /* We need to read the First half */
    if (WriteIndexBufferAudio < (AUDIO_BUFF_SIZE / 2))
    {
      BSP_LED_Toggle(LED_RED);
    }
  }
  else if (ReadIndexBufferAudio == (AUDIO_BUFF_SIZE / 2))
  {
    /* We need to Read the Second half */
    if (WriteIndexBufferAudio > (AUDIO_BUFF_SIZE / 2))
    {
      BSP_LED_Toggle(LED_RED);
    }
  }
}

/**
 * @brief  Initialize the wave header file
 * @param  pHeader: Header Buffer to be filled
 * @param  pWaveFormatStruct: Pointer to the wave structure to be filled.
 * @retval 0 if passed, !0 if failed.
 */
static uint32_t WavProcess_HeaderInit(void)
{
  uint16_t BitPerSample = 16;
  uint32_t ByteRate = AUDIO_IN_SAMPLING_FREQUENCY * (BitPerSample / 8);

  uint32_t SampleRate = AUDIO_IN_SAMPLING_FREQUENCY;
  uint16_t BlockAlign = BitPerSample / 8;

  /* Write chunkID, must be 'RIFF'  ------------------------------------------*/
  pAudioHeader[0] = 'R';
  pAudioHeader[1] = 'I';
  pAudioHeader[2] = 'F';
  pAudioHeader[3] = 'F';

  /* Write the file length ----------------------------------------------------*/
  /* The sampling time: this value will be be written back at the end of the
  recording opearation.  Example: 661500 Btyes = 0x000A17FC, byte[7]=0x00, byte[4]=0xFC */
  pAudioHeader[4] = 0x00;
  pAudioHeader[5] = 0x4C;
  pAudioHeader[6] = 0x1D;
  pAudioHeader[7] = 0x00;

  /* Write the file format, must be 'WAVE' -----------------------------------*/
  pAudioHeader[8] = 'W';
  pAudioHeader[9] = 'A';
  pAudioHeader[10] = 'V';
  pAudioHeader[11] = 'E';

  /* Write the format chunk, must be'fmt ' -----------------------------------*/
  pAudioHeader[12] = 'f';
  pAudioHeader[13] = 'm';
  pAudioHeader[14] = 't';
  pAudioHeader[15] = ' ';

  /* Write the length of the 'fmt' data, must be 0x10 ------------------------*/
  pAudioHeader[16] = 0x10;
  pAudioHeader[17] = 0x00;
  pAudioHeader[18] = 0x00;
  pAudioHeader[19] = 0x00;

  /* Write the audio format, must be 0x01 (PCM) ------------------------------*/
  pAudioHeader[20] = 0x01;
  pAudioHeader[21] = 0x00;

  /* Write the number of channels, ie. 0x01 (Mono) ---------------------------*/
  pAudioHeader[22] = 1;
  pAudioHeader[23] = 0x00;

  /* Write the Sample Rate in Hz ---------------------------------------------*/
  /* Write Little Endian ie. 8000 = 0x00001F40 => byte[24]=0x40, byte[27]=0x00*/
  pAudioHeader[24] = (uint8_t)((SampleRate & 0xFF));
  pAudioHeader[25] = (uint8_t)((SampleRate >> 8) & 0xFF);
  pAudioHeader[26] = (uint8_t)((SampleRate >> 16) & 0xFF);
  pAudioHeader[27] = (uint8_t)((SampleRate >> 24) & 0xFF);

  /* Write the Byte Rate -----------------------------------------------------*/
  pAudioHeader[28] = (uint8_t)((ByteRate & 0xFF));
  pAudioHeader[29] = (uint8_t)((ByteRate >> 8) & 0xFF);
  pAudioHeader[30] = (uint8_t)((ByteRate >> 16) & 0xFF);
  pAudioHeader[31] = (uint8_t)((ByteRate >> 24) & 0xFF);

  /* Write the block alignment -----------------------------------------------*/
  pAudioHeader[32] = BlockAlign;
  pAudioHeader[33] = 0x00;

  /* Write the number of bits per sample -------------------------------------*/
  pAudioHeader[34] = BitPerSample;
  pAudioHeader[35] = 0x00;

  /* Write the Data chunk, must be 'data' ------------------------------------*/
  pAudioHeader[36] = 'd';
  pAudioHeader[37] = 'a';
  pAudioHeader[38] = 't';
  pAudioHeader[39] = 'a';

  /* Write the number of sample data -----------------------------------------*/
  /* This variable will be written back at the end of the recording operation */
  pAudioHeader[40] = 0x00;
  pAudioHeader[41] = 0x4C;
  pAudioHeader[42] = 0x1D;
  pAudioHeader[43] = 0x00;

  /* Return 0 if all operations are OK */
  return 0;
}

/**
 * @brief  Initialize the wave header file
 * @param  pHeader: Header Buffer to be filled
 * @param  pWaveFormatStruct: Pointer to the wave structure to be filled.
 * @retval 0 if passed, !0 if failed.
 */
static uint32_t WavProcess_HeaderUpdate(uint32_t len)
{
  /* Write the file length ----------------------------------------------------*/
  /* The sampling time: this value will be be written back at the end of the
  recording opearation.  Example: 661500 Btyes = 0x000A17FC, byte[7]=0x00, byte[4]=0xFC */
  pAudioHeader[4] = (uint8_t)(len);
  pAudioHeader[5] = (uint8_t)(len >> 8);
  pAudioHeader[6] = (uint8_t)(len >> 16);
  pAudioHeader[7] = (uint8_t)(len >> 24);
  /* Write the number of sample data -----------------------------------------*/
  /* This variable will be written back at the end of the recording operation */
  len -= 44;
  pAudioHeader[40] = (uint8_t)(len);
  pAudioHeader[41] = (uint8_t)(len >> 8);
  pAudioHeader[42] = (uint8_t)(len >> 16);
  pAudioHeader[43] = (uint8_t)(len >> 24);
  /* Return 0 if all operations are OK */
  return 0;
}

/* USER CODE END 1 */
