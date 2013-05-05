/**
  ******************************************************************************
  * @file    Project/STM32L1xx_StdPeriph_Template/main.h 
  * @author  MCD Application Team
  * @version V1.0.0
  * @date    13-September-2011
  * @brief   Header for main.c module
  ******************************************************************************
  * @attention
  *
  * THE PRESENT FIRMWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
  * WITH CODING INFORMATION REGARDING THEIR PRODUCTS IN ORDER FOR THEM TO SAVE
  * TIME. AS A RESULT, STMICROELECTRONICS SHALL NOT BE HELD LIABLE FOR ANY
  * DIRECT, INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING
  * FROM THE CONTENT OF SUCH FIRMWARE AND/OR THE USE MADE BY CUSTOMERS OF THE
  * CODING INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
  *
  * <h2><center>&copy; COPYRIGHT 2010 STMicroelectronics</center></h2>
  ******************************************************************************
  */ 
  
/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

/* Includes ------------------------------------------------------------------*/
#include "stm32l1xx.h"
#include "GTSV_BlackControl_board.h"
#include "GTSV_BlackControl_lcd.h"
#include "stm32_tsl_api.h"

#include <stdio.h>

/* Exported types ------------------------------------------------------------*/
struct SystemConfig {
	uint8_t sys_clk_src; //0x00: MSI | 0x04:HSI | 0x08:HSE | 0x0C: PLL --> used as sysclk source
	RCC_ClocksTypeDef RCC_Clk;
	unsigned RCC_FLAG_HSIRDY1:1;
	unsigned RCC_FLAG_MSIRDY1:1;
	unsigned RCC_FLAG_HSERDY1:1;
	unsigned RCC_FLAG_PLLRDY1:1;
	unsigned RCC_FLAG_LSERDY1:1;
	unsigned RCC_FLAG_LSIRDY1:1; //LSI oscillator clock ready
    unsigned RCC_FLAG_OBLRST1:1;  // Option Byte Loader (OBL) reset 
	unsigned RCC_FLAG_PINRST1:1;// Pin reset
	unsigned RCC_FLAG_PORRST1:1;// POR/PDR reset
	unsigned RCC_FLAG_SFTRST1:1;// Software reset
	unsigned RCC_FLAG_IWDGRST1:1;// Independent Watchdog reset
	unsigned RCC_FLAG_WWDGRST1:1;// Window Watchdog reset
	unsigned RCC_FLAG_LPWRRST1:1;// Low Power reset

};




struct SystemFlags {
	unsigned ms10_flag:1;
	unsigned ms50_flag:1;
	unsigned ms100_flag:1;
	unsigned ms200_flag:1;
	unsigned ms500_flag:1;
	unsigned ms300_flag:1;
	unsigned fanRotate:2;
	unsigned system_state:1;
};


extern uint16_t msTicks;
extern struct SystemFlags gSystemFlags;
extern uint16_t gBuzzerSoundFreqHz;
extern uint16_t gBuzzerSoundLenghtMs;

/* Exported constants --------------------------------------------------------*/
#define DEBUG
#define BUZZER_SOUND_FREQ_HZ			2000
#define BUZZER_SOUND_LENGHT_MS	300	
/* Exported macro ------------------------------------------------------------*/

#ifndef BITBAND_PERI
#define BITBAND_PERI(a,b) ((PERIPH_BB_BASE + (a-PERIPH_BASE)*32 + (b*4)))
#endif
#ifndef BITBAND_SRAM
#define BITBAND_SRAM(a,b) ((PERIPH_BB_BASE + (a-PERIPH_BASE)*32 + (b*4)))
#endif




#ifdef DEBUG
#define TIME_DEBUG_SET(pin_num)	*((volatile unsigned char *)(BITBAND_PERI(GPIOB_BASE + 0x18, pin_num))) = 1
#define TIME_DEBUG_RESET(pin_num) *((volatile unsigned char *)(BITBAND_PERI(GPIOB_BASE + 0x1A, pin_num))) = 1
#endif

#define BUZZER_PORT_BASE	GPIOC_BASE
#define BUZZER_PIN_NUM			13
#define BUZZER_PIN_BIT_OUT\
		*((volatile unsigned char *)(BITBAND_PERI(GPIOB_BASE + 0x14, BUZZER_PIN_NUM)))
#define BUZZER_TOGGLE()\
		BUZZER_PIN_BIT_OUT ^= 1;
/* Exported functions ------------------------------------------------------- */

//void TimingDelay_Decrement(void);
//void Delay(__IO uint32_t nTime);


void Cpu_to_default_config(void);
void Get_system_clk_config(void);
void Ports_to_default_config(void);
void Timers_to_default_config(void);
void Buzzer_bip(void);
void Buzzer_bip_ms(uint16_t ms);

void Gtsv_main_loop(void);

void Enable_touch_keys(void);


#endif /* __MAIN_H */

/******************* (C) COPYRIGHT 2010 STMicroelectronics *****END OF FILE****/
