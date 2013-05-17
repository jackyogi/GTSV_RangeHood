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
struct SystemFlags  gSystemFlags = {
	.sys_state = SYS_STATE_INITIAL,
	.ctime_hrs = 0,
	.ctime_mins =0,
	.fan_spd_default = 1,
	.blower_apo_mins =1,
};
uint32_t _LCD_RAM[8];

uint16_t msTicks;

uint16_t tmp_ms1, tmp_ms2, diff_ms;

uint8_t hours=0, mins=0;


GPIO_InitTypeDef GPIO_InitStructure;
EXTI_InitTypeDef EXTI_InitStructure;
NVIC_InitTypeDef NVIC_InitStructure;
TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
SPI_InitTypeDef SPI_InitStructure;

uint8_t tmp_spi=0x10, spi_init[8] = {0x00, 0x40, 0xC0, 0xFF, 0xFF,
								0xFF, 0xFF, 0x97};

uint32_t myRam[16];
uint32_t tmp_ir_cmd;
uint8_t com_seg_set[2], com_seg_clear[2];


void Spi_to_default_config(void);


#define STB_SET_BIT		GPIO_SetBits(GPIOA, GPIO_Pin_3);
#define STB_RESET_BIT	GPIO_ResetBits(GPIOA, GPIO_Pin_3);


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
	RTC_to_default_config();
	Ports_to_default_config();
  	//Lcd_to_default_config();
	Buzzer_timer_to_default_state();
	Tsense_to_default_config();
	//Timers_to_default_config();
	Spilcd_to_default_config();



	//Lcd_set();
	gSystemFlags.sys_state = SYS_STATE_OFF;
	while (1)
	{
		// Run TSL RC state machine
		TSL_Action();

		main_tick();
		if(gSystemFlags.ms100_flag){
			gSystemFlags.ms100_flag = 0;
			RTC_GetTime(RTC_Format_BIN, &RTC_TimeStructure);
		}


		if(gSystemFlags.ms500_flag){
			gSystemFlags.ms500_flag = 0;



		}

		Spilcd_flush_buf_to_lcd();

	}
}

uint8_t test=0;
void main_tick(void)
{

	Tsense_key_detect();

	if(gSystemFlags.ms50_flag){
		gSystemFlags.ms50_flag=0;

	}
	main_big_switch();
/*
	Lcd_icon_fan(Lcd_get_fan_cursor_slow());
	if(Lcd_get_blink_cursor())
		Lcd_icon_on(LCD_ICON_COLON1);
	else
		Lcd_icon_off(LCD_ICON_COLON1);
*/
//for any SYS State
	if(Tsense_check_key_up(TSENSE_KEY_LIGHT)){
		Buzzer_bip();
		gSystemFlags.light_state ^= 1;
	}

	if(gSystemFlags.light_state){
		Lcd_icon_on(LCD_ICON_LIGHT);
		MAIN_LAMP_ON;
	}else{
		Lcd_icon_off(LCD_ICON_LIGHT);
		MAIN_LAMP_OFF;
	}

	if((gSystemFlags.sys_state == SYS_STATE_OFF)
		&&(gSystemFlags.light_state == 0)
		&& !Tsense_check_key_holding(TSENSE_KEY_ANY)){
		LED_ALL_OFF;
	}else{
		LED_ALL_ON;
	}



}



void main_big_switch(void)
{
	static uint8_t blinking_disable=0;

	switch(gSystemFlags.sys_state){
	case SYS_STATE_INITIAL:
		gSystemFlags.sys_state = SYS_STATE_OFF;
		break;
	case SYS_STATE_OFF:
		//*****update LCD & LED
		if(!gSystemFlags.sys_state_off_changing){
			Lcd_clear_hour2_min2();
			Lcd_fill_pos_with_num(LCD_POS_FAN_SPEED, 11); //off
			Lcd_icon_fan(3);  //fan icons off
			Lcd_icon_off(LCD_ICON_COLON2);
			Lcd_icon_off(LCD_ICON_CLOCK);
			Lcd_fill_hour1(RTC_TimeStructure.RTC_Hours);
			Lcd_fill_min1(RTC_TimeStructure.RTC_Minutes);
			//blink colon2 icon
			if(Lcd_get_blink_cursor()){
				Lcd_icon_on(LCD_ICON_COLON1);
			}else{
				Lcd_icon_off(LCD_ICON_COLON1);
			}

		}

		//*****check keys
		//key power  --> begin blowing with def spd & counting time
		if(Tsense_check_key_up(TSENSE_KEY_POWER)
				&& !gSystemFlags.sys_state_off_changing){
			Buzzer_bip();
			gSystemFlags.sys_state = SYS_STATE_BLOWING;
			Ctime_begin_counting();
			Blower_set_speed(gSystemFlags.fan_spd_default);
		}
		//hold key minus --> change to Dtime_adj
		if(gSystemFlags.sys_state_off_changing){
			if(Tsense_check_key_up(TSENSE_KEY_MINUS)
						|| Tsense_check_key_up(TSENSE_KEY_POWER)){

				Buzzer_bip();
				gSystemFlags.sys_state = SYS_STATE_APO_DTIME_ADJ;
				gSystemFlags.time_adj_delay =0;

			}
		}
		if(Tsense_check_key_holding(TSENSE_KEY_MINUS)
				|| Tsense_check_key_holding(TSENSE_KEY_POWER)){

			gSystemFlags.sys_state_off_changing = 1;
			Lcd_icon_off(LCD_ICON_COLON1); //off colon1 icon
			Lcd_icon_fan(5); //off fan icons
			Lcd_clear_hour1_min1();
			Lcd_icon_on(LCD_ICON_CLOCK); //on clock icon
			Lcd_icon_on(LCD_ICON_COLON2); //on colon2 icon
			Lcd_fill_hour2(gSystemFlags.blower_apo_mins);
			Lcd_fill_min2(0);
		}

		//hold key Plus --> clear ctime & change to blowing
		if(Tsense_check_key_holding(TSENSE_KEY_PLUS)){
			Buzzer_bip();
			Ctime_to_zero();
			gSystemFlags.sys_state = SYS_STATE_BLOWING;
			Ctime_begin_counting();
			Blower_set_speed(gSystemFlags.fan_spd_default);
		}
		//gSystemFlags.sys_state_apo_dtime_adj=0;

		break;
	case SYS_STATE_BLOWING:
		//*****update LCD & LED
		Lcd_icon_off(LCD_ICON_CLOCK);

		//blink colon1 icon
		if(Lcd_get_blink_cursor()){
			Lcd_icon_on(LCD_ICON_COLON1);
			Lcd_icon_on(LCD_ICON_COLON2);
		}else{
			Lcd_icon_off(LCD_ICON_COLON1);
			Lcd_icon_off(LCD_ICON_COLON2);
		}
		//fill ctime
		Lcd_fill_hour1(gSystemFlags.ctime_hrs);
		Lcd_fill_min1(gSystemFlags.ctime_mins);
		//fill zero
		Lcd_fill_hour2(RTC_TimeStructure.RTC_Hours);
		Lcd_fill_min2(RTC_TimeStructure.RTC_Minutes);
		//fan rotate
		if(gSystemFlags.fan_spd>2)
			Lcd_icon_fan(Lcd_get_fan_cursor_fast());
		else
			Lcd_icon_fan(Lcd_get_fan_cursor_slow());
		//fill fan spd
		Lcd_fill_pos_with_num(LCD_POS_FAN_SPEED, gSystemFlags.fan_spd);

		////*****check keys
		//key plus
		if(Tsense_check_key(TSENSE_KEY_PLUS)){
			Buzzer_bip();
			if(gSystemFlags.fan_spd_default<4){
				gSystemFlags.fan_spd_default++;
			}
			Blower_set_speed(gSystemFlags.fan_spd_default);
		}
		//key minus
		if(Tsense_check_key(TSENSE_KEY_MINUS)){
			Buzzer_bip();
			if(gSystemFlags.fan_spd_default>1){
				gSystemFlags.fan_spd_default--;

			}
			Blower_set_speed(gSystemFlags.fan_spd_default);
		}
		//PW key & not holding  -> change to off
		if(Tsense_check_key_up(TSENSE_KEY_POWER)
			&&(!Tsense_check_key_holding(TSENSE_KEY_POWER))){
			Buzzer_bip();
			gSystemFlags.sys_state = SYS_STATE_OFF;
			Blower_set_speed(0);
			Ctime_stop_counting();
		}
		//hold PW key  --> to APO BLOWING with default APO Dtime
		if(Tsense_check_key_holding(TSENSE_KEY_POWER)){
			Buzzer_bip();
			gSystemFlags.sys_state = SYS_STATE_APO_BLOWING;
			gSystemFlags.blower_apo_remaining_sec = gSystemFlags.blower_apo_mins*60;
			gSystemFlags.blower_apo_time_out = 0;
			//Lcd_clear();
		}
		break;
	case SYS_STATE_APO_BLOWING:
		//*****update LCD & LED
		Lcd_icon_on(LCD_ICON_CLOCK);
		//fill ctime
		Lcd_fill_hour1(gSystemFlags.ctime_hrs);
		Lcd_fill_min1(gSystemFlags.ctime_mins);
		//fill remaining Time
		Lcd_fill_hour2(gSystemFlags.blower_apo_remaining_sec/60);
		Lcd_fill_min2(gSystemFlags.blower_apo_remaining_sec%60);

		if(Lcd_get_blink_cursor()){
			Lcd_icon_on(LCD_ICON_COLON1);
			Lcd_icon_on(LCD_ICON_COLON2);
		}else{
			Lcd_icon_off(LCD_ICON_COLON1);
			Lcd_icon_off(LCD_ICON_COLON2);
		}
		//fan rotate
		if(gSystemFlags.fan_spd>2)
			Lcd_icon_fan(Lcd_get_fan_cursor_fast());
		else
			Lcd_icon_fan(Lcd_get_fan_cursor_slow());
		//fill fan spd
		Lcd_fill_pos_with_num(LCD_POS_FAN_SPEED, gSystemFlags.fan_spd);


		//check API time out
		if(gSystemFlags.blower_apo_time_out){
			Blower_set_speed(0);
			gSystemFlags.sys_state = SYS_STATE_OFF;
			Lcd_clear();
			//all_ui_led_off();
			Buzzer_bip();
			Ctime_stop_counting();
		}

		//key plus
		if(Tsense_check_key(TSENSE_KEY_PLUS)){
			Buzzer_bip();
			if(gSystemFlags.fan_spd_default<4){
				gSystemFlags.fan_spd_default++;
			}
			Blower_set_speed(gSystemFlags.fan_spd_default);
		}
		//key minus
		if(Tsense_check_key(TSENSE_KEY_MINUS)){
			Buzzer_bip();
			if(gSystemFlags.fan_spd_default>1){
				gSystemFlags.fan_spd_default--;

			}
			Blower_set_speed(gSystemFlags.fan_spd_default);
		}
		//PW key & not holding  -> change to off
		if(Tsense_check_key_up(TSENSE_KEY_POWER)
			&&(!Tsense_check_key_holding(TSENSE_KEY_POWER))){
			Buzzer_bip();
			gSystemFlags.sys_state = SYS_STATE_OFF;
			Blower_set_speed(0);
			Ctime_stop_counting();
		}
		break;
	case SYS_STATE_APO_DTIME_ADJ:

		//*****update LCD & LED
		Lcd_icon_off(LCD_ICON_COLON1); //off colon1 icon
		Lcd_icon_fan(5); //off fan icons
		Lcd_clear_hour1_min1();

		Lcd_icon_on(LCD_ICON_CLOCK); //on clock icon
		Lcd_icon_on(LCD_ICON_COLON2); //on colon2 icon

		//blink colon1 icon
		if(Lcd_get_blink_cursor()
			     || Tsense_check_key_touching(TSENSE_KEY_PLUS)
			     || Tsense_check_key_touching(TSENSE_KEY_MINUS)
			     || Tsense_check_key(TSENSE_KEY_MINUS)){

			Lcd_fill_hour2(gSystemFlags.blower_apo_mins);
			Lcd_fill_min2(0);  //min2 == 0
		}else{
			Lcd_clear_hour2_min2();
		}

		//******check time out
		//count time adj delay
		if(gSystemFlags.s1_flag){
			gSystemFlags.s1_flag =0;
			gSystemFlags.time_adj_delay++;
		}
		if(Tsense_check_key_touching(TSENSE_KEY_ANY))
			gSystemFlags.time_adj_delay =0;
		//check time out time adjust delay
		if(gSystemFlags.time_adj_delay>8){
			gSystemFlags.sys_state = SYS_STATE_OFF;
		}

		//******check keys
		if(Tsense_check_key_up(TSENSE_KEY_POWER)
				&& !gSystemFlags.sys_state_off_changing){
			//gSystemFlags.sys_state_apo_dtime_adj=1;
			Buzzer_bip();
			gSystemFlags.sys_state = SYS_STATE_OFF;
		}
		if(Tsense_check_key(TSENSE_KEY_PLUS)
				|| (Tsense_check_key_holding(TSENSE_KEY_PLUS) && gSystemFlags.ms300_flag) ){
			gSystemFlags.ms300_flag=0;
			Buzzer_bip();

			if(gSystemFlags.blower_apo_mins>15)
				gSystemFlags.blower_apo_mins = 1;
			else
				gSystemFlags.blower_apo_mins++;
		}
		if((Tsense_check_key(TSENSE_KEY_MINUS)
				|| (Tsense_check_key_holding(TSENSE_KEY_MINUS) && gSystemFlags.ms300_flag))
				&& (!gSystemFlags.sys_state_off_changing)){
			gSystemFlags.ms300_flag=0;
			Buzzer_bip();

			if(gSystemFlags.blower_apo_mins==1)
				gSystemFlags.blower_apo_mins = 15;
			else
				gSystemFlags.blower_apo_mins--;
		}
		gSystemFlags.sys_state_off_changing = 0;
		break;
	default:
		gSystemFlags.sys_state = SYS_STATE_OFF;
		break;

	};

}

void Blower_set_speed(uint8_t spd)
{


	switch(spd){
	case 1:
		gSystemFlags.fan_spd = 1;
		BLOWER_FAN1 = 1;
		BLOWER_FAN2 = 0;
		BLOWER_FAN3 = 0;
		BLOWER_FAN4 = 0;
		break;
	case 2:
		gSystemFlags.fan_spd = 2;
		BLOWER_FAN1 = 0;
		BLOWER_FAN2 = 1;
		BLOWER_FAN3 = 0;
		BLOWER_FAN4 = 0;
		break;
	case 3:
		gSystemFlags.fan_spd = 3;
		BLOWER_FAN1 = 0;
		BLOWER_FAN2 = 0;
		BLOWER_FAN3 = 1;
		BLOWER_FAN4 = 0;
		break;
	case 4:
		gSystemFlags.fan_spd = 4;
		BLOWER_FAN1 = 0;
		BLOWER_FAN2 = 0;
		BLOWER_FAN3 = 0;
		BLOWER_FAN4 = 1;
		break;
	default:
		gSystemFlags.fan_spd = 0;
		BLOWER_FAN1 = 0;
		BLOWER_FAN2 = 0;
		BLOWER_FAN3 = 0;
		BLOWER_FAN4 = 0;
		Lcd_icon_fan(5);
		break;
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
	//GPIO_ResetBits(GPIOA,GPIO_Pin_12);

//config GPIO for UI LEDs
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOA, ENABLE);
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_OD;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
	//GPIO_SetBits(GPIOA, GPIO_Pin_11);
	//GPIO_ResetBits(GPIOA, GPIO_Pin_11);

	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOC, ENABLE);
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_12;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_OD;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
	GPIO_Init(GPIOC, &GPIO_InitStructure);
	GPIO_SetBits(GPIOC, GPIO_Pin_12);
	//GPIO_ResetBits(GPIOC, GPIO_Pin_12);

//config GPIO for FAN: open drain w/ pull up
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOB | RCC_AHBPeriph_GPIOC, ENABLE);
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3 | GPIO_Pin_5 | GPIO_Pin_8;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
	GPIO_Init(GPIOB, &GPIO_InitStructure);
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
	GPIO_Init(GPIOC, &GPIO_InitStructure);

//config GPIO for LMP  : open drain w/ pull up
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOA, ENABLE);
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	Spilcd_configure_GPIO();

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

void Ctime_begin_counting(void)
{
	gSystemFlags.ctime_counting = 1;

}
void Ctime_stop_counting(void)
{
	gSystemFlags.ctime_counting = 0;
}
void Ctime_to_zero(void)
{
	gSystemFlags.ctime_mins = 0;
	gSystemFlags.ctime_hrs = 0;
}
void Ctime_tick_min(void)
{
	if(gSystemFlags.ctime_counting){
		gSystemFlags.ctime_mins++;
		if(gSystemFlags.ctime_mins==60){
			gSystemFlags.ctime_hrs++;
		}
	}
}
bool Ctime_check_overtime(void)
{
	if(gSystemFlags.ctime_hrs>=30){
		return TRUE;
	}else {
		return FALSE;
	}
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
