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

#include "main.h"




/* Private variables ---------------------------------------------------------*/
static volatile uint32_t TimingDelay;

struct SystemConfig _gSystemConfig;
struct SystemFlags  gSystemFlags;
uint32_t _LCD_RAM[8];

uint16_t msTicks;

uint8_t hours=0, mins=0;


GPIO_InitTypeDef GPIO_InitStructure;
EXTI_InitTypeDef EXTI_InitStructure;
NVIC_InitTypeDef NVIC_InitStructure;
TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;


uint32_t myRam[16];
uint32_t tmp_ir_cmd;
uint8_t com_seg_set[2], com_seg_clear[2];
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
	int i;
#ifdef DEBUG
	//get system config for debug purposes
	Get_system_clk_config();
#endif

	Cpu_to_default_config();
	//RTC_to_default_config();
	Ports_to_default_config();
  	//Lcd_to_default_config();
	Buzzer_timer_to_default_state();
	Tsense_to_default_config();
	//Timers_to_default_config();

	LED_ALL = 1;

	/*Until application reset*/
	while (1)
	{
		/* Run TSL RC state machine */
	  	TSL_Action();;
		if(gSystemFlags.ms100_flag){
			gSystemFlags.ms100_flag = 0;
			Lcd_blink_systicISR_ms();
		}
				
		for(i=0; i<9; i++){
			//LCD->RAM[i] = myRam[i];
			//Lcd_fill_pos_with_num(i, i);
		}
		
		if(gSystemFlags.ms500_flag){
			gSystemFlags.ms500_flag=0;
			//LED_ALL ^= 1;
			RTC_GetTime(RTC_Format_BIN, &RTC_TimeStructure);
			Lcd_fill_pos_with_double_digit_num(0, RTC_TimeStructure.RTC_Hours);
			Lcd_fill_pos_with_double_digit_num(1, RTC_TimeStructure.RTC_Minutes);
			Lcd_fill_pos_with_double_digit_num(2, RTC_TimeStructure.RTC_Seconds);
		}
		
		Lcd_fill_pos_with_num(8, 2);
		Lcd_icon_on(LCD_ICON_LIGHT);
		if(Lcd_get_blink_cursor()){
			Lcd_icon_on(LCD_ICON_COLON1);
			Lcd_icon_on(LCD_ICON_CLOCK);
		}else{
			Lcd_icon_off(LCD_ICON_COLON1);
			Lcd_icon_off(LCD_ICON_CLOCK);
		}

		Lcd_icon_fan(Lcd_get_fan_cursor_slow());
		
		LCD_UpdateDisplayRequest();

	}
}



void Cpu_to_default_config(void)
{
	if (SysTick_Config(SystemCoreClock / 1000)){
		//printf("ERROR: SysTick_Config failed\n");
	} else {
		//printf("SysTick_Config success!\n");
		NVIC_SetPriority(SysTick_IRQn, (1 << __NVIC_PRIO_BITS) - 2);
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

//config GPIO for Buzzer and turn it low
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOA, ENABLE);
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_12;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
	GPIO_ResetBits(GPIOA,GPIO_Pin_12);

//config GPIO for UI LEDs
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOA, ENABLE);
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_OD;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);



//config GPIO for FAN: open drain w/ pull up
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOA | RCC_AHBPeriph_GPIOB | RCC_AHBPeriph_GPIOC, ENABLE);
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4 | GPIO_Pin_6;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
	//GPIO_Init(GPIOA, &GPIO_InitStructure);
	//GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;
	//GPIO_Init(GPIOB, &GPIO_InitStructure);
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_2 | GPIO_Pin_3;
	GPIO_Init(GPIOC, &GPIO_InitStructure);

//config GPIO for LMP  : open drain w/ pull up
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOB, ENABLE);
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	//config GPIO for LCD
	//Lcd_configure_GPIO();

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
	//Buzzer_timer_to_default_state();

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
