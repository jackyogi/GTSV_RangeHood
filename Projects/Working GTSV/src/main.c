/**
  ******************************************************************************
  * @file    main.c
  * @author  Long Pham
  * @version V0.1
  * @date    April 2013
  * @brief   Main program body
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
  * <h2><center>&copy; COPYRIGHT 2013 GTSV</center></h2>
  */

/* Includes ------------------------------------------------------------------*/

/* Standard STM32L1xxx driver headers */
#include "stm32l1xx_conf.h"


/* discovery board and specific drivers headers*/
#include "GTSV_BlackControl_board.h"
#include "GTSV_BlackControl_lcd.h"
#include "IRremote.h"
#include "main.h"
#include "GTSV_RH_RTC_config.h"
#include "tsl_user.h"



/* Private variables ---------------------------------------------------------*/
static volatile uint32_t TimingDelay;

struct SystemConfig _gSystemConfig;
struct SystemFlags  gSystemFlags;
uint32_t _LCD_RAM[8];

uint16_t msTicks;

uint8_t hours=0, mins=0;

uint16_t gBuzzerSoundFreqHz = BUZZER_SOUND_FREQ_HZ;
uint16_t gBuzzerSoundLenghtMs = BUZZER_SOUND_LENGHT_MS;

GPIO_InitTypeDef GPIO_InitStructure;
EXTI_InitTypeDef EXTI_InitStructure;
NVIC_InitTypeDef NVIC_InitStructure;
TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;





uint32_t tmp_ir_cmd;



/*******************************************************************************/
/**
  * @brief main entry point.
  * @par Parameters None
  * @retval void None
  * @par Required preconditions: None
  */
int main(void)
{

 /*!< At this stage the microcontroller clock setting is already configured,
       this is done through SystemInit() function which is called from startup
       file (startup_stm32l1xx_md.s) before to branch to application main.
       To reconfigure the default setting of SystemInit() function, refer to
       system_stm32l1xx.c file
     */

	/* Check if the StandBy flag is set */
	if (PWR_GetFlagStatus(PWR_FLAG_SB) != RESET)
	{
		/* System resumed from STANDBY mode */
		/* Clear StandBy flag */
		//RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR,ENABLE);
		//PWR_ClearFlag(PWR_FLAG_SB);
		/* set StandbyWakeup indicator*/
		//StanbyWakeUp = TRUE;
	} else
	{
		/* Reset StandbyWakeup indicator*/
		//StanbyWakeUp = FALSE;
	}


	Cpu_to_default_config();
	RTC_to_default_config();
	Lcd_to_default_config();
	Ports_to_default_config();
	Timers_to_default_config();

#ifdef DEBUG
	//get system config for debug purposes
	Get_system_clk_config();
#endif

	Irr_init();
	
    TSL_user_Init();
    
  /*Until application reset*/
  while (1)
  {
	Gtsv_main_loop();

  }
}


void Gtsv_main_loop (void)
{
  //============================================================================
  // Main loop
  //============================================================================

  for (;;)
  {
    // Execute STMTouch Driver state machine
    if (TSL_user_Action() == TSL_STATUS_OK)
    {
    		Buzzer_bip();
      //ProcessSensors(); // Execute sensors related tasks
    }
    else
    {
		 if(Irr_decode(&irr_decode_results)){
			
			if(irr_decode_results.value == IRR_NEC_REPEAT){
				if(tmp_ir_cmd== IRR_NEC_CMD_SPEEDDOWN){
					Buzzer_bip();
				} else if(tmp_ir_cmd== IRR_NEC_CMD_SPEEDUP){
					Buzzer_bip();
				}
			}else if(gSystemFlags.system_state==1){
				tmp_ir_cmd= irr_decode_results.value;
				
				switch(tmp_ir_cmd){
				case IRR_NEC_CMD_LIGHT:
					Lcd_icon_toggle(LCD_LIGHTBULB_ICON);
					Lcd_icon_toggle(LCD_LIGHTRAY_ICON);
					Buzzer_bip();
					break;
				case IRR_NEC_CMD_ONOFF:
					//turn off led
					GPIOB->ODR = !GPIO_Pin_12;
					gSystemFlags.system_state = 0;
					Buzzer_bip();
					break;
				case IRR_NEC_CMD_TIMER:
					Lcd_icon_toggle(LCD_CLOCK_ICON);
					Buzzer_bip();
					break;
				case IRR_NEC_CMD_AUTO:
					Lcd_icon_toggle(LCD_ROTATE_ICON);
					Buzzer_bip();
					break;
				case IRR_NEC_CMD_SPEEDDOWN:
					Buzzer_bip();
					break;
				case IRR_NEC_CMD_SPEEDUP:
					Buzzer_bip();
					break;
				default:
					break;
				};
			} else if(gSystemFlags.system_state == 0){
				tmp_ir_cmd= irr_decode_results.value;
				if(tmp_ir_cmd == IRR_NEC_CMD_ONOFF){
					gSystemFlags.system_state = 1;
					GPIOB->ODR = GPIO_Pin_12;
					Buzzer_bip();
				}
			}
			
			Irr_resume();	
		}


			RTC_GetTime(RTC_Format_BIN, &RTC_TimeStructure);
		RTC_GetDate(RTC_Format_BIN, &RTC_DateStructure);
		Lcd_fill_hours(RTC_TimeStructure.RTC_Minutes);
		Lcd_fill_mins(RTC_TimeStructure.RTC_Seconds);

		//Lcd_icon_on(LCD_LIGHTBULB_ICON);
		//Lcd_icon_on(LCD_ROTATE_ICON);
		//Lcd_icon_on(LCD_CLOCK_ICON);
		//Lcd_icon_on(LCD_LIGHTRAY_ICON);
		if(gSystemFlags.ms500_flag){
			gSystemFlags.ms500_flag = 0;
			GPIO_TOGGLE(GPIOB,GPIO_Pin_7);
			Lcd_icon_toggle(LCD_COLON_ICON);

		}

		if(gSystemFlags.ms300_flag){
			gSystemFlags.ms300_flag=0;
			if(++gSystemFlags.fanRotate==3)
				gSystemFlags.fanRotate=0;
			
		}
		if(gSystemFlags.system_state == 1)
			Lcd_icon_fan(gSystemFlags.fanRotate);
		else
			Lcd_icon_off(LCD_ALL_ICON);


		LCD_UpdateDisplayRequest();
    }
  }

}

void Cpu_to_default_config(void)
{
	if (SysTick_Config(SystemCoreClock / 1000)){
		//printf("ERROR: SysTick_Config failed\n");
	} else {
		//printf("SysTick_Config success!\n");
		NVIC_SetPriority(SysTick_IRQn, (1 << __NVIC_PRIO_BITS) -1);
	}
}

/**
  * @brief  To initialize the I/O ports
  * @caller main
  * @param None
  * @retval None
  */



void Ports_to_default_config(void)
{

	
//config GPIO for LCD
	Lcd_configure_GPIO();

//config GPIO for LED: Green & Blue
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOB, ENABLE);
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6 | GPIO_Pin_7;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
	GPIO_Init(GPIOB, &GPIO_InitStructure);
	
//config GPIO for Buzzer and turn it low
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOC, ENABLE);
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_13;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
	GPIO_Init(GPIOC, &GPIO_InitStructure);
	GPIO_ResetBits(GPIOC,GPIO_Pin_13);

//config GPIO for UI LEDs
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOA |RCC_AHBPeriph_GPIOB, ENABLE);
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1 | GPIO_Pin_2 | GPIO_Pin_4;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_OD;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_12 | GPIO_Pin_13 | GPIO_Pin_14 ;
	GPIO_Init(GPIOB, &GPIO_InitStructure);

	//GPIO_SetBits(GPIOB, GPIO_Pin_12);

//config GPIO for IR
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOB, ENABLE);	
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_15;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
	GPIO_Init(GPIOB, &GPIO_InitStructure);



//config GPIO for FAN
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOC, ENABLE);
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_2 | GPIO_Pin_3;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
	GPIO_Init(GPIOC, &GPIO_InitStructure);

//config GPIO for LMP
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOB, ENABLE);
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_7;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
	GPIO_Init(GPIOB, &GPIO_InitStructure);

}


void Timers_to_default_config(void)
{

// Init timer 6 generate 50us interupt for Ir receiver
	//clock to TIME6
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM6,ENABLE);
	//TIM6 TimeBase init
	TIM_TimeBaseStructure.TIM_Period = ((SystemCoreClock / 20000) - 1);  //50us equal 20kHz ~20000
	TIM_TimeBaseStructure.TIM_Prescaler = 0;
	TIM_TimeBaseStructure.TIM_ClockDivision = 0;
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
	TIM_TimeBaseInit(TIM6, &TIM_TimeBaseStructure);
	
	//enable local timer interupt
	TIM_ClearITPendingBit(TIM6,TIM_IT_Update);
	TIM_ITConfig(TIM6,TIM_IT_Update,ENABLE);
	
	//Enable Global TIM6 INT
	NVIC_InitStructure.NVIC_IRQChannel = TIM6_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = (1 << __NVIC_PRIO_BITS) -2;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);
	
	TIM_Cmd(TIM6, ENABLE);

// Init timer 7 to generate buzzer sound & auto off
	//clock to TIME7
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM7,ENABLE);
	//TIM7 TimeBase init
	TIM_TimeBaseStructure.TIM_Period = ((SystemCoreClock / (2*gBuzzerSoundFreqHz)) - 1);
	TIM_TimeBaseStructure.TIM_Prescaler = 0;
	TIM_TimeBaseStructure.TIM_ClockDivision = 0;
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
	TIM_TimeBaseInit(TIM7, &TIM_TimeBaseStructure);
	
	//enable local timer interupt
	TIM_ClearITPendingBit(TIM7,TIM_IT_Update);
	TIM_ITConfig(TIM7,TIM_IT_Update,ENABLE);
	
	//Enable Global TIM6 INT
	NVIC_InitStructure.NVIC_IRQChannel = TIM7_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = (1 << __NVIC_PRIO_BITS) -1;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);
	
	//TIM_Cmd(TIM7, ENABLE);

}


void Buzzer_bip(void){
	TIM_Cmd(TIM7, ENABLE);
}

void Buzzer_bip_ms(uint16_t ms)
{
	TIM_Cmd(TIM7, ENABLE);
}
/**
  * @brief  Get System Clock config for debug purposes
  * @param  None
  * @retval None
  */
void Get_system_clk_config(void)
{

    _gSystemConfig.sys_clk_src = RCC_GetSYSCLKSource();
  //RCC_GetFlagStatus
  RCC_GetClocksFreq(&_gSystemConfig.RCC_Clk);
  _gSystemConfig.RCC_FLAG_HSIRDY1 = RCC_GetFlagStatus(RCC_FLAG_HSIRDY);
  _gSystemConfig.RCC_FLAG_MSIRDY1 = RCC_GetFlagStatus(RCC_FLAG_MSIRDY);
  _gSystemConfig.RCC_FLAG_HSERDY1 = RCC_GetFlagStatus(RCC_FLAG_HSERDY);
  _gSystemConfig.RCC_FLAG_PLLRDY1 = RCC_GetFlagStatus(RCC_FLAG_PLLRDY);
  _gSystemConfig.RCC_FLAG_LSERDY1 = RCC_GetFlagStatus(RCC_FLAG_LSERDY);

  _gSystemConfig.RCC_FLAG_LSIRDY1 = RCC_GetFlagStatus(RCC_FLAG_LSIRDY);
  _gSystemConfig.RCC_FLAG_OBLRST1 = RCC_GetFlagStatus(RCC_FLAG_OBLRST);
  _gSystemConfig.RCC_FLAG_PINRST1 = RCC_GetFlagStatus(RCC_FLAG_PINRST);
  _gSystemConfig.RCC_FLAG_PORRST1 = RCC_GetFlagStatus(RCC_FLAG_PORRST);
  _gSystemConfig.RCC_FLAG_SFTRST1 = RCC_GetFlagStatus(RCC_FLAG_SFTRST);
  _gSystemConfig.RCC_FLAG_IWDGRST1 = RCC_GetFlagStatus(RCC_FLAG_IWDGRST);
  _gSystemConfig.RCC_FLAG_WWDGRST1 = RCC_GetFlagStatus(RCC_FLAG_WWDGRST);
  _gSystemConfig.RCC_FLAG_LPWRRST1 = RCC_GetFlagStatus(RCC_FLAG_LPWRRST);

}


/**
  * @brief  Executed when a sensor is in Error state
  * @param  None
  * @retval None
  */
void MyTKeys_ErrorStateProcess(void)
{
  // Add here your own processing when a sensor is in Error state
  TSL_tkey_SetStateOff();
/*
  for (;;)
  {
	//GPIOB->BSRR |= GPIO_Pin_6;
	GPIO_HIGH(GPIOB,GPIO_Pin_6);
  }
  */
}


/**
  * @brief  Executed when a sensor is in Off state
  * @param  None
  * @retval None
  */
void MyTKeys_OffStateProcess(void)
{
  // Add here your own processing when a sensor is in Off state
}



#ifdef  USE_FULL_ASSERT

/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t* file, uint32_t line)
{
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* Infinite loop */
  while (1);
}

#endif

/******************* (C) COPYRIGHT 2011 STMicroelectronics *****END OF FILE****/
