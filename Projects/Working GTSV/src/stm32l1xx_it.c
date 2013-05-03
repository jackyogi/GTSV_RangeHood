/**
  ******************************************************************************
  * @file    Project/STM32L1xx_StdPeriph_Template/stm32l1xx_it.c 
  * @author  MCD Application Team
  * @version V1.0.2
  * @date    September-2011
  * @brief   Main Interrupt Service Routines.
  *          This file provides template for all exceptions handler and 
  *          peripherals interrupt service routine.
  ******************************************************************************
  * @copy
  *
  * THE PRESENT FIRMWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
  * WITH CODING INFORMATION REGARDING THEIR PRODUCTS IN ORDER FOR THEM TO SAVE
  * TIME. AS A RESULT, STMICROELECTRONICS SHALL NOT BE HELD LIABLE FOR ANY
  * DIRECT, INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING
  * FROM THE CONTENT OF SUCH FIRMWARE AND/OR THE USE MADE BY CUSTOMERS OF THE
  * CODING INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
  *
  * <h2><center>&copy; COPYRIGHT 2010 STMicroelectronics</center></h2>
  */ 

/* Includes ------------------------------------------------------------------*/
#include "stm32l1xx_it.h"
#include "GTSV_RH_RTC_config.h"
#include "GTSV_BlackControl_board.h"
#include "IRremote.h"
#include "main.h"


/** @addtogroup Template_Project
  * @{
  */

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
/* Private functions ---------------------------------------------------------*/

/******************************************************************************/
/*            Cortex-M3 Processor Exceptions Handlers                         */
/******************************************************************************/

/**
  * @brief  This function handles NMI exception.
  * @param  None
  * @retval None
  */
void NMI_Handler(void)
{
}

/**
  * @brief  This function handles Hard Fault exception.
  * @param  None
  * @retval None
  */
void HardFault_Handler(void)
{
  /* Go to infinite loop when Hard Fault exception occurs */
  while (1);
}

/**
  * @brief  This function handles Memory Manage exception.
  * @param  None
  * @retval None
  */
void MemManage_Handler(void)
{
  /* Go to infinite loop when Memory Manage exception occurs */
  while (1);
}

/**
  * @brief  This function handles Bus Fault exception.
  * @param  None
  * @retval None
  */
void BusFault_Handler(void)
{
  /* Go to infinite loop when Bus Fault exception occurs */
  while (1);
}

/**
  * @brief  This function handles Usage Fault exception.
  * @param  None
  * @retval None
  */
void UsageFault_Handler(void)
{
  /* Go to infinite loop when Usage Fault exception occurs */
  while (1);
}

/**
  * @brief  This function handles SVCall exception.
  * @param  None
  * @retval None
  */
void SVC_Handler(void)
{  
  /* Go to infinite loop when Hard Fault exception occurs */
  while (1);
}

/**
  * @brief  This function handles Debug Monitor exception.
  * @param  None
  * @retval None
  */
void DebugMon_Handler(void)
{
 /* Go to infinite loop when Hard Fault exception occurs */
  while (1);
}

/**
  * @brief  This function handles PendSVC exception.
  * @param  None
  * @retval None
  */
void PendSV_Handler(void)
{
    /* Go to infinite loop when Hard Fault exception occurs */
  while (1);
}

/**
  * @brief  This function handles SysTick interrupts.
  * @param  None
  * @retval None
  */
void SysTick_Handler(void)
{
    msTicks++;
	if((msTicks%10) ==0){
		gSystemFlags.ms10_flag =1;
		if((msTicks%50) == 0){
			gSystemFlags.ms50_flag = 1;
			if((msTicks%100) == 0){
				gSystemFlags.ms100_flag = 1;
				if((msTicks%200)==0)
					gSystemFlags.ms200_flag =1;					
				if((msTicks%300) == 0)
					gSystemFlags.ms300_flag = 1;
				if((msTicks%500) == 0)
					gSystemFlags.ms500_flag = 1;
			
			}
		}
	}
}


#define IRR_GAP_TICKS 	500
/**
  * @brief  This function handles TIM6 global interrupt request.
  * @param  None
  * @retval None
  */
void TIM6_IRQHandler(void)
{ 
	//TIME_DEBUG_SET(6);
	if (TIM_GetITStatus(TIM6, TIM_IT_Update) != RESET){
		TIM_ClearITPendingBit(TIM6, TIM_IT_Update);		
		uint8_t ir_data = *irrparams.recv_pin;
		irrparams.tick_cnt++; //one more 50us tick
		
		//check buffer overflow
		if(irrparams.rawbuff_len >= 88){
			irrparams.recv_state = IRR_STATE_STOP;
		}
		switch(irrparams.recv_state){
		case IRR_STATE_IDLE: 
			//wait until mark, increasing tick_cnt => tick_cnt is measuring Space.
			//if we got a mark, check if previous space big enoug.
			//if the previous space is not big enough 
			//then zero tick & wait for another bigger space.
			if(ir_data == IRR_MARK){ 
				if(irrparams.tick_cnt < IRR_GAP_TICKS){ //not big enough to be a gap
					irrparams.tick_cnt = 0; //reset counter & wait for another mark
				} else { //we get here until we got a tick_cnt>500 (5ms)
					irrparams.rawbuff_len = 0;
					irrparams.rawbuff[irrparams.rawbuff_len++] = irrparams.tick_cnt;
					irrparams.tick_cnt = 0;
					irrparams.recv_state = IRR_STATE_MARK;
				}
			} else {  //we are counting the space gap
				if(irrparams.tick_cnt==65534)
					irrparams.tick_cnt = 65533;
			}
			break;
		case IRR_STATE_MARK: //timing MARK
			if(ir_data == IRR_SPACE){
				irrparams.rawbuff[irrparams.rawbuff_len++] = irrparams.tick_cnt;
				irrparams.tick_cnt = 0;
				irrparams.recv_state = IRR_STATE_SPACE;
			} else {
				//mark state too long --> do something! (normally the mark is not too long)
			}
			break;
		case IRR_STATE_SPACE: //timing SPACE
			if(ir_data == IRR_MARK){
				irrparams.rawbuff[irrparams.rawbuff_len++] = irrparams.tick_cnt;
				irrparams.tick_cnt = 0;
				irrparams.recv_state = IRR_STATE_MARK;
			} else {
				if(irrparams.tick_cnt> IRR_GAP_TICKS){
					// big SPACE, indicates gap between codes
        			// Mark current code as ready for processing
        			// Switch to STOP
        			// Don't reset timer; keep counting space width
        			irrparams.recv_state = IRR_STATE_STOP;
				}
			}
			break;
		case IRR_STATE_STOP:
			if(ir_data == IRR_MARK){
				irrparams.tick_cnt=0;
			}
			break;

		}
		
		
	}
	//TIME_DEBUG_RESET(6);
	
}

/**
  * @brief  This function handles TIM7 global interrupt request.
  * @param  None
  * @retval None
  */
void TIM7_IRQHandler(void)
{	
	static uint16_t delay_ms;
	
	if(++delay_ms == ((uint32_t)(gBuzzerSoundFreqHz-1000)*gBuzzerSoundLenghtMs)/1000 ){
		TIM_Cmd(TIM7,DISABLE);
		delay_ms=0;
	}else{
		GPIOC->ODR ^= GPIO_Pin_13;
	}

	
	TIM_ClearITPendingBit(TIM7, TIM_IT_Update);
}

void EXTI0_IRQHandler(void)
{
  /* Disable general interrupts */
  //disableInterrupts();
  	  //RTC_GetTime(RTC_Format_BIN, &RTC_TimeStructure);
  //RTC_GetDate(RTC_Format_BIN, &RTC_DateStructure);
  EXTI_ClearITPendingBit(EXTI_Line0);
  //enableInterrupts();
}


void RTC_WKUP_IRQHandler (void)
{
	if(RTC_GetITStatus(RTC_IT_WUT) != RESET)
	{
	  /* Toggle LED1 */
	  GPIO_TOGGLE(LD_GPIO_PORT,LD_GREEN_GPIO_PIN);
	  RTC_ClearITPendingBit(RTC_IT_WUT);
	  EXTI_ClearITPendingBit(EXTI_Line20);
	} 

}

/******************************************************************************/
/*                 STM32L1xx Peripherals Interrupt Handlers                   */
/*  Add here the Interrupt Handler for the used peripheral(s) (PPP), for the  */
/*  available peripheral interrupt handler's name please refer to the startup */
/*  file (startup_stm32l1xx_md.s).                                            */
/******************************************************************************/

/**
  * @brief  This function handles PPP interrupt request.
  * @param  None
  * @retval None
  */
/*void PPP_IRQHandler(void)
{
}*/

/**
  * @}
  */ 


/******************* (C) COPYRIGHT 2010 STMicroelectronics *****END OF FILE****/
