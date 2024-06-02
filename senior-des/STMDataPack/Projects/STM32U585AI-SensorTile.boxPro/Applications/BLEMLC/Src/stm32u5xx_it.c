/**
  ******************************************************************************
  * @file    BLEMLC\Src\stm32u5xx_it.c
  * @author  System Research & Applications Team - Agrate/Catania Lab.
  * @version V1.6.0
  * @date    20-Oct-2023
  * @brief   Interrupt Service Routines
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
#include "main.h"
#include "stm32u5xx_it.h"
#include "hci_tl_interface.h"
/* Private includes ----------------------------------------------------------*/

/* Private typedef -----------------------------------------------------------*/

/* Private define ------------------------------------------------------------*/

/* Private macro -------------------------------------------------------------*/

/* Private variables ---------------------------------------------------------*/
extern TIM_HandleTypeDef TimCCHandle;

/* Private function prototypes -----------------------------------------------*/

/* Private user code ---------------------------------------------------------*/

/* External variables --------------------------------------------------------*/

/******************************************************************************/
/*           Cortex Processor Interruption and Exception Handlers          */
/******************************************************************************/
/**
  * @brief This function handles Non maskable interrupt.
  */
void NMI_Handler(void)
{
  while (1)
  {
  }
}

/**
  * @brief This function handles Hard fault interrupt.
  */
void HardFault_Handler(void)
{
  while (1)
  {
  }
}

/**
  * @brief This function handles Memory management fault.
  */
void MemManage_Handler(void)
{
  while (1)
  {
  }
}

/**
  * @brief This function handles Prefetch fault, memory access fault.
  */
void BusFault_Handler(void)
{
  while (1)
  {
  }
}

/**
  * @brief This function handles Undefined instruction or illegal state.
  */
void UsageFault_Handler(void)
{
  while (1)
  {
  }
}

/**
  * @brief This function handles System service call via SWI instruction.
  */
void SVC_Handler(void)
{
}

/**
  * @brief This function handles Debug monitor.
  */
void DebugMon_Handler(void)
{
}

/**
  * @brief This function handles Pendable request for system service.
  */
void PendSV_Handler(void)
{
}

/**
  * @brief This function handles System tick timer.
  */
void SysTick_Handler(void)
{
  HAL_IncTick();
}

/******************************************************************************/
/* STM32U5xx Peripheral Interrupt Handlers                                    */
/* Add here the Interrupt Handlers for the used peripherals.                  */
/* For the available peripheral interrupt handler names,                      */
/* please refer to the startup file (startup_stm32u5xx.s).                    */
/******************************************************************************/

/**
  * @brief This function handles EXTI Line11 interrupt.
  */
void EXTI11_IRQHandler(void)
{
  HAL_EXTI_IRQHandler(&H_EXTI_11);
}

/**
  * @brief This function handles EXTI Line13 interrupt.
  */
void EXTI13_IRQHandler(void)
{
  HAL_EXTI_IRQHandler(&H_EXTI_13);
}

/**
  * @brief This function handles EXTI Line4 interrupt.
  */
void EXTI4_IRQHandler(void)
{
  HAL_EXTI_IRQHandler(&H_EXTI_4);
}

/**
  * @brief  This function handles TIM1 Interrupt request
  * @param  None
  * @retval None
  */
void TIM1_CC_IRQHandler(void)
{
  HAL_TIM_IRQHandler(&TimCCHandle);
}


