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



/* Private variables ---------------------------------------------------------*/
static volatile uint32_t TimingDelay;

struct SystemConfig _gSystemConfig;
struct SystemFlags  gSystemFlags;


uint16_t msTicks;
uint8_t hours=0, mins=0;


GPIO_InitTypeDef GPIO_InitStructure;
EXTI_InitTypeDef EXTI_InitStructure;
NVIC_InitTypeDef NVIC_InitStructure;
TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;


void all_ui_led_off(void)
{
	LED_AUTO_BT = 0;
	LED_MINUS_BT = 0;
	LED_PLUS_BT = 0;
	LED_TIMER_BT = 0;
	LED_LIGHT_BT = 0;
}

uint32_t tmp_ir_cmd;


void main_big_switch(void)
{
	switch(gSystemFlags.sys_state){
	case SYS_STATE_OFF:
		//*****update LCD & LED
		RTC_GetTime(RTC_Format_BIN, &RTC_TimeStructure);
		Lcd_fill_hours(RTC_TimeStructure.RTC_Hours);
		Lcd_fill_mins(RTC_TimeStructure.RTC_Minutes);
		//blink colon icon
		if(Lcd_get_blink_cursor())
			Lcd_icon_on(LCD_COLON_ICON);
		else
			Lcd_icon_off(LCD_COLON_ICON);

		//*****check keys
		//key Timer
		if(Tsense_check_key_pushing(TSENSE_KEY_TIMER) ||
			(tmp_ir_cmd == IRR_NEC_CMD_TIMER)){
			gSystemFlags.tmp_hour = RTC_TimeStructure.RTC_Hours;
			gSystemFlags.tmp_min = RTC_TimeStructure.RTC_Minutes;
			gSystemFlags.time_adj_stage =0;
			gSystemFlags.sys_state = SYS_STATE_CLK_ADJ;
			gSystemFlags.time_adj_delay=0;
			Lcd_clear();		
			all_ui_led_off();
		}
		//key plus
		if(Tsense_check_key_pushing(TSENSE_KEY_PLUS) ||
			(tmp_ir_cmd == IRR_NEC_CMD_SPEEDUP) ){
			Blower_set_speed(1);
			gSystemFlags.sys_state = SYS_STATE_BLOWING;
			Lcd_clear();
			all_ui_led_off();
		}
		//key minus
		if(Tsense_check_key_pushing(TSENSE_KEY_MINUS) ||
			(tmp_ir_cmd == IRR_NEC_CMD_SPEEDDOWN) ){
			Blower_set_speed(4);
			gSystemFlags.sys_state = SYS_STATE_BLOWING;
			Lcd_clear();
			all_ui_led_off();
		}
		//key auto
		if(Tsense_check_key_pushing(TSENSE_KEY_AUTO) ||
			(tmp_ir_cmd == IRR_NEC_CMD_AUTO) ||
			(tmp_ir_cmd == IRR_NEC_CMD_ONOFF)){
			gSystemFlags.sys_state = SYS_STATE_AUTO;
			Lcd_clear();
			all_ui_led_off();
		}
		break;
	case SYS_STATE_AUTO:
		//*****update LCD & LED
		RTC_GetTime(RTC_Format_BIN, &RTC_TimeStructure);
		Lcd_fill_hours(RTC_TimeStructure.RTC_Hours);
		Lcd_fill_mins(RTC_TimeStructure.RTC_Minutes);
		//blink colon icon
		if(Lcd_get_blink_cursor())
			Lcd_icon_on(LCD_COLON_ICON);
		else
			Lcd_icon_off(LCD_COLON_ICON);
		Lcd_icon_on(LCD_ROTATE_ICON);
		LED_AUTO_BT = 1;

		//*****check keys
		//key Auto
		if(Tsense_check_key_pushing(TSENSE_KEY_AUTO) ||
			(tmp_ir_cmd == IRR_NEC_CMD_AUTO) ||
			(tmp_ir_cmd == IRR_NEC_CMD_ONOFF)){
			gSystemFlags.sys_state = SYS_STATE_OFF;
			Lcd_clear();
			all_ui_led_off();
		}
		
		break;
	case SYS_STATE_CLK_ADJ:
		//*****update LCD & LED
		Lcd_icon_on(LCD_COLON_ICON);
		LED_TIMER_BT = 1;
		LED_PLUS_BT = 1;
		LED_MINUS_BT = 1;
		
		gSystemFlags.time_adj_delay++;
		if(gSystemFlags.time_adj_stage == 0){ //adj hours
			//*****update LCD & LED
			Lcd_fill_mins(gSystemFlags.tmp_min);
			if((Lcd_get_blink_cursor())
				 ||(Tsense_check_key_pushed(TSENSE_KEY_PLUS))
				 ||(Tsense_check_key_pushed(TSENSE_KEY_MINUS)) ){
				Lcd_fill_hours(gSystemFlags.tmp_hour);
			}else{
				Lcd_fill_hours(88);
			}
			
			////*****check keys
			//key Timer
			if(Tsense_check_key_pushing(TSENSE_KEY_TIMER) 
				 ||(tmp_ir_cmd == IRR_NEC_CMD_TIMER)
				 ||(gSystemFlags.time_adj_delay > 133)){
				gSystemFlags.time_adj_stage++;
				gSystemFlags.time_adj_delay =0;
				if(!Tsense_check_key_pushing(TSENSE_KEY_TIMER))
						Buzzer_bip();
			}
			//key plus
			if(Tsense_check_key_pushing(TSENSE_KEY_PLUS)
			     ||(Tsense_check_key_holding(TSENSE_KEY_PLUS) && gSystemFlags.ms200_flag)
			     ||(tmp_ir_cmd == IRR_NEC_CMD_SPEEDUP)){
			    gSystemFlags.ms200_flag =0;
			    gSystemFlags.time_adj_delay = 0;
				if(gSystemFlags.tmp_hour == 23)
					gSystemFlags.tmp_hour = 0;
				else
					gSystemFlags.tmp_hour++;
			}
			//key minus
			if(Tsense_check_key_pushing(TSENSE_KEY_MINUS)
			      ||(Tsense_check_key_holding(TSENSE_KEY_MINUS) && gSystemFlags.ms200_flag)
			      ||(tmp_ir_cmd == IRR_NEC_CMD_SPEEDDOWN) ){
			    gSystemFlags.ms200_flag =0;
			    gSystemFlags.time_adj_delay = 0;
				if(gSystemFlags.tmp_hour == 0)
					gSystemFlags.tmp_hour = 23;
				else
					gSystemFlags.tmp_hour--;
			}
			//reset time_adj_delay if any button active
			if( Tsense_check_key_pushed(TSENSE_KEY_PLUS)
				 || Tsense_check_key_pushed(TSENSE_KEY_MINUS)
				 || Tsense_check_key_pushing(TSENSE_KEY_LIGHT)
				 || Tsense_check_key_pushed(TSENSE_KEY_LIGHT)
				 || (tmp_ir_cmd == IRR_NEC_CMD_LIGHT))
				gSystemFlags.time_adj_delay =0;

		}else{  //adj mins
			//*****update LCD & LED
			Lcd_fill_hours(gSystemFlags.tmp_hour);
			if(Lcd_get_blink_cursor()
				 || Tsense_check_key_pushed(TSENSE_KEY_PLUS)
				 || Tsense_check_key_pushed(TSENSE_KEY_MINUS) ){
				Lcd_fill_mins(gSystemFlags.tmp_min);
			}else{
				Lcd_fill_mins(88);
			}
			
			////*****check keys
			//key timer or auto change after adj_delay time
			if(Tsense_check_key_pushing(TSENSE_KEY_TIMER) 
				||(tmp_ir_cmd == IRR_NEC_CMD_TIMER)
				||(gSystemFlags.time_adj_delay > 133)){
				
					RTC_change_time(gSystemFlags.tmp_hour, gSystemFlags.tmp_min, 0);
					//save time
					//send cmd update time to serial
					gSystemFlags.sys_state = SYS_STATE_OFF;
					Lcd_clear();
					all_ui_led_off();
					//if(!Tsense_check_key_pushing(TSENSE_KEY_TIMER))
						Buzzer_2bips();
			}
			//key plus
			if(Tsense_check_key_pushing(TSENSE_KEY_PLUS)
			     || (Tsense_check_key_holding(TSENSE_KEY_PLUS) && gSystemFlags.ms200_flag)
			     || (tmp_ir_cmd == IRR_NEC_CMD_SPEEDUP)){
			    gSystemFlags.ms200_flag =0;
			    gSystemFlags.time_adj_delay = 0;
				if(gSystemFlags.tmp_min == 59)
					gSystemFlags.tmp_min= 0;
				else
					gSystemFlags.tmp_min++;
			}
			//key minus
			if(Tsense_check_key_pushing(TSENSE_KEY_MINUS)
			      || (Tsense_check_key_holding(TSENSE_KEY_MINUS) && gSystemFlags.ms200_flag)
			      || (tmp_ir_cmd == IRR_NEC_CMD_SPEEDDOWN) ){
			    gSystemFlags.ms200_flag =0;
			    gSystemFlags.time_adj_delay = 0;
				if(gSystemFlags.tmp_min == 0)
					gSystemFlags.tmp_min = 59;
				else
					gSystemFlags.tmp_min--;
			}
			//reset time_adj delay
			if(Tsense_check_key_pushed(TSENSE_KEY_PLUS)
				 || Tsense_check_key_pushed(TSENSE_KEY_MINUS)
				 || Tsense_check_key_pushing(TSENSE_KEY_LIGHT)
				 || Tsense_check_key_pushed(TSENSE_KEY_LIGHT)
				 || (tmp_ir_cmd == IRR_NEC_CMD_LIGHT))
				gSystemFlags.time_adj_delay =0;
		}
		break;
	case SYS_STATE_BLOWING:
		//*****update LCD & LED
		LED_PLUS_BT = 1;
		LED_MINUS_BT = 1;		
		//Lcd_icon_off(LCD_COLON_ICON);
		//Lcd_fill_pos_with_blank(0);
		//Lcd_fill_pos_with_blank(3);
		Lcd_fill_pos_with_num(2, gSystemFlags.blower_fan_speed);
		Lcd_fill_pos_with_num(1, 10);
		if(gSystemFlags.blower_fan_speed>2)
			Lcd_icon_fan(Lcd_get_fan_cursor_fast());
		else
			Lcd_icon_fan(Lcd_get_fan_cursor_slow());

		////*****check keys
		//key plus
		if(Tsense_check_key_pushing(TSENSE_KEY_PLUS)
			 || (tmp_ir_cmd == IRR_NEC_CMD_SPEEDUP) ){
			if(gSystemFlags.blower_fan_speed == 4){
				Blower_set_speed(0);
				gSystemFlags.sys_state = SYS_STATE_OFF;
				Lcd_clear();
				all_ui_led_off();
			}else{
				Blower_set_speed(++gSystemFlags.blower_fan_speed);
			}
		}
		//key minus
		if(Tsense_check_key_pushing(TSENSE_KEY_MINUS)
			 || (tmp_ir_cmd == IRR_NEC_CMD_SPEEDDOWN) ){			
			if(gSystemFlags.blower_fan_speed == 1){
				Blower_set_speed(0);
				gSystemFlags.sys_state = SYS_STATE_OFF;
				Lcd_clear();
				all_ui_led_off();
			}else{
				Blower_set_speed(--gSystemFlags.blower_fan_speed);
			}
		}
		//key auto
		if(Tsense_check_key_pushing(TSENSE_KEY_AUTO)
			  || (tmp_ir_cmd == IRR_NEC_CMD_AUTO)
			  || (tmp_ir_cmd == IRR_NEC_CMD_ONOFF)){
			Blower_set_speed(0);
			gSystemFlags.sys_state = SYS_STATE_AUTO;
			Lcd_clear();
			all_ui_led_off();
		}
		//key timer
		if(Tsense_check_key_pushing(TSENSE_KEY_TIMER)
			 || (tmp_ir_cmd == IRR_NEC_CMD_TIMER)){
			gSystemFlags.blower_apo_mins_tmp =1;
			gSystemFlags.sys_state = SYS_STATE_BLOWING_APO_ADJ;
			gSystemFlags.time_adj_delay =0;
			Lcd_clear();
			all_ui_led_off();
		}

		break;
	case SYS_STATE_BLOWING_APO_ADJ:
		//*****update LCD & LED
		LED_TIMER_BT = 1;
		Lcd_icon_on(LCD_COLON_ICON);
		Lcd_icon_on(LCD_CLOCK_ICON);
		Lcd_fill_mins(0);		
		if(Lcd_get_blink_cursor()
			  || Tsense_check_key_pushed(TSENSE_KEY_PLUS)
			  || Tsense_check_key_pushed(TSENSE_KEY_MINUS) ){
			Lcd_fill_hours(gSystemFlags.blower_apo_mins_tmp);
			if(gSystemFlags.blower_apo_mins_tmp<10)
				Lcd_fill_pos_with_blank(0);
		}else{
			Lcd_fill_hours(88);
		}
		if(gSystemFlags.blower_fan_speed>2)
			Lcd_icon_fan(Lcd_get_fan_cursor_fast());
		else
			Lcd_icon_fan(Lcd_get_fan_cursor_slow());

		////*****check keys
		gSystemFlags.time_adj_delay++;
		//key plus
		if(Tsense_check_key_pushing(TSENSE_KEY_PLUS)
		      || (Tsense_check_key_holding(TSENSE_KEY_PLUS) && gSystemFlags.ms500_flag)
		      || (tmp_ir_cmd == IRR_NEC_CMD_SPEEDUP)){
		    gSystemFlags.ms500_flag =0;
		    gSystemFlags.time_adj_delay = 0;
			if(gSystemFlags.blower_apo_mins_tmp == 15)
				gSystemFlags.blower_apo_mins_tmp= 1;
			else
				gSystemFlags.blower_apo_mins_tmp++;
		}
		//key minus
		if(Tsense_check_key_pushing(TSENSE_KEY_MINUS)
		      || (Tsense_check_key_holding(TSENSE_KEY_MINUS) && gSystemFlags.ms500_flag)
		      || (tmp_ir_cmd == IRR_NEC_CMD_SPEEDDOWN) ){
		    gSystemFlags.ms500_flag =0;
		    gSystemFlags.time_adj_delay = 0;
			if(gSystemFlags.blower_apo_mins_tmp == 1)
				gSystemFlags.blower_apo_mins_tmp = 15;
			else
				gSystemFlags.blower_apo_mins_tmp--;
		}
		//key Timer
		if(Tsense_check_key_pushing(TSENSE_KEY_TIMER) 
			  || (tmp_ir_cmd == IRR_NEC_CMD_TIMER)){
			gSystemFlags.sys_state = SYS_STATE_BLOWING;
			Lcd_clear();
			all_ui_led_off();
		}
		//reset time_adj_delay
		if(Tsense_check_key_pushed(TSENSE_KEY_PLUS)
			 || Tsense_check_key_pushed(TSENSE_KEY_MINUS)
			 || Tsense_check_key_pushing(TSENSE_KEY_LIGHT)
			 || Tsense_check_key_pushed(TSENSE_KEY_LIGHT)
			 || (tmp_ir_cmd == IRR_NEC_CMD_LIGHT))
			gSystemFlags.time_adj_delay =0;

		//auto change to Blowing auto power off after a delay time
		if(gSystemFlags.time_adj_delay>133){
			gSystemFlags.sys_state = SYS_STATE_BLOWING_APO;
			gSystemFlags.blower_apo_remaining_sec = gSystemFlags.blower_apo_mins_tmp*60;
			gSystemFlags.blower_apo_time_out = 0;
			Lcd_clear();
			all_ui_led_off();
			Buzzer_2bips();
		}
		break;
	case SYS_STATE_BLOWING_APO:
		//*****update LCD & LED
		LED_TIMER_BT = 1;
		LED_PLUS_BT = 1;
		LED_MINUS_BT = 1;
		Lcd_fill_pos_with_num(2, gSystemFlags.blower_fan_speed);
		Lcd_fill_pos_with_num(1, 10);
		if(Lcd_get_blink_cursor())
			Lcd_icon_on(LCD_CLOCK_ICON);
		else
			Lcd_icon_off(LCD_CLOCK_ICON);
		if(gSystemFlags.blower_fan_speed>2)
			Lcd_icon_fan(Lcd_get_fan_cursor_fast());
		else
			Lcd_icon_fan(Lcd_get_fan_cursor_slow());

		////*****check keys
		//key plus
		if(Tsense_check_key_pushing(TSENSE_KEY_PLUS)
			   || (tmp_ir_cmd == IRR_NEC_CMD_SPEEDUP) ){
			if(gSystemFlags.blower_fan_speed == 4){
				Blower_set_speed(0);
				gSystemFlags.sys_state = SYS_STATE_OFF;
				Lcd_clear();
				all_ui_led_off();
			}else{
				Blower_set_speed(++gSystemFlags.blower_fan_speed);
			}
		}
		//key minus
		if(Tsense_check_key_pushing(TSENSE_KEY_MINUS)
			 || (tmp_ir_cmd == IRR_NEC_CMD_SPEEDDOWN) ){
			
			if(gSystemFlags.blower_fan_speed == 1){
				Blower_set_speed(0);
				gSystemFlags.sys_state = SYS_STATE_OFF;
				Lcd_clear();
				all_ui_led_off();
			}else{
				Blower_set_speed(--gSystemFlags.blower_fan_speed);
			}
		}
		//key auto
		if(Tsense_check_key_pushing(TSENSE_KEY_AUTO)
			    || (tmp_ir_cmd == IRR_NEC_CMD_AUTO)
			    || (tmp_ir_cmd == IRR_NEC_CMD_ONOFF)){
			Blower_set_speed(0);
			gSystemFlags.sys_state = SYS_STATE_AUTO;
			Lcd_clear();
			all_ui_led_off();
		}
		//key timer
		if(Tsense_check_key_releasing(TSENSE_KEY_TIMER)
			  && (!Tsense_check_key_holding(TSENSE_KEY_TIMER))
			     || (tmp_ir_cmd == IRR_NEC_CMD_TIMER)){
			gSystemFlags.sys_state = SYS_STATE_BLOWING;
			Lcd_clear();
			all_ui_led_off();
		}
		//hold key timer ->show remaining time
		if(Tsense_check_key_holding(TSENSE_KEY_TIMER)){
			Lcd_fill_hours(gSystemFlags.blower_apo_remaining_sec/60);
			Lcd_fill_mins(gSystemFlags.blower_apo_remaining_sec%60);
			if(Lcd_get_blink_cursor()){
				Lcd_icon_on(LCD_COLON_ICON);
			}else{
				Lcd_icon_off(LCD_COLON_ICON);
			}
		}
		//check timer release to clear collon
		if(Tsense_check_key_releasing(TSENSE_KEY_TIMER)){
			//Lcd_icon_off(LCD_COLON_ICON);
			Lcd_clear();
		}

		//check auto power off time out
		if(gSystemFlags.blower_apo_time_out){
			Blower_set_speed(0);
			gSystemFlags.sys_state = SYS_STATE_OFF;
			Lcd_clear();
			all_ui_led_off();
			Buzzer_bip();
		}
		break;
	
  	}
}
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

#ifdef DEBUG
	//get system config for debug purposes
	Get_system_clk_config();
#endif

	Cpu_to_default_config();
	RTC_to_default_config();
	Ports_to_default_config();
  	Lcd_to_default_config();
	Timers_to_default_config();



	Irr_init();
	
	Tsense_to_default_config();
	LED_BACKLIGHT = 1;
  /*Until application reset*/
  while (1)
  {
	Tsense_key_detect();
	Irr_key_detect();
	
	if(Systick_check_delay50ms()){
		Tsense_key_hold_detect_tick50ms();
	}
	
	
	

	main_big_switch();
	
  	

//for any SYS State 
	if(Tsense_check_key_pushing(TSENSE_KEY_LIGHT) ||
		(tmp_ir_cmd == IRR_NEC_CMD_LIGHT)){
		gSystemFlags.light_state ^= 1;
		//send cmd for others to turn on lamp
		//wait for confirm of lamp on (time out??)
		//send cmd for others to turn off lamp
		//wait for confirm of lamp off (time out??)

	}

	if(gSystemFlags.light_state){		
		Lcd_icon_on(LCD_LIGHTBULB_ICON);
		Lcd_icon_on(LCD_LIGHTRAY_ICON);
		MAIN_LAMP_ON;
		LED_LIGHT_BT_ON;
		
	}else{
		Lcd_icon_off(LCD_LIGHTBULB_ICON);
		Lcd_icon_off(LCD_LIGHTRAY_ICON);
		MAIN_LAMP_OFF;
		LED_LIGHT_BT_OFF;
	}
	
	if((gSystemFlags.sys_state == SYS_STATE_OFF) &&
		(gSystemFlags.light_state == 0)){
		LED_BACKLIGHT_OFF;
	}else{
		LED_BACKLIGHT_ON;
	}
	
	LCD_UpdateDisplayRequest();
  }
}





/*

void Irr_main_loop(void)
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



	
}
*/
void Blower_set_speed(uint8_t spd)
{
	switch(spd){
	case 1:
		gSystemFlags.blower_fan_speed = 1;
		BLOWER_FAN1 = 1;
		BLOWER_FAN2 = 0;
		BLOWER_FAN3 = 0;
		BLOWER_FAN4 = 0;
		break;
	case 2:
		gSystemFlags.blower_fan_speed = 2;
		BLOWER_FAN1 = 0;
		BLOWER_FAN2 = 1;
		BLOWER_FAN3 = 0;
		BLOWER_FAN4 = 0;
		break;
	case 3:
		gSystemFlags.blower_fan_speed = 3;
		BLOWER_FAN1 = 0;
		BLOWER_FAN2 = 0;
		BLOWER_FAN3 = 1;
		BLOWER_FAN4 = 0;
		break;
	case 4:
		gSystemFlags.blower_fan_speed = 4;
		BLOWER_FAN1 = 0;
		BLOWER_FAN2 = 0;
		BLOWER_FAN3 = 0;
		BLOWER_FAN4 = 1;
		break;
	default:
		gSystemFlags.blower_fan_speed = 0;
		BLOWER_FAN1 = 0;
		BLOWER_FAN2 = 0;
		BLOWER_FAN3 = 0;
		BLOWER_FAN4 = 0;
		Lcd_icon_fan(5);
		break;
	}
}

void Cpu_to_default_config(void)
{
	if (SysTick_Config(SystemCoreClock / 1000)){
		//printf("ERROR: SysTick_Config failed\n");
	} else {
		//printf("SysTick_Config success!\n");
		NVIC_SetPriority(SysTick_IRQn, INT_PRIORITY_SYSTICK);
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


//config GPIO for FAN: 
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOC, ENABLE);
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_2 | GPIO_Pin_3;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
	GPIO_Init(GPIOC, &GPIO_InitStructure);

//config GPIO for LMP  : 
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOB, ENABLE);
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_7;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
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
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = INT_PRIORITY_TIM6;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);

	TIM_Cmd(TIM6, ENABLE);

// Init timer 7 to generate buzzer sound & auto off
	//clock to TIME7
	Buzzer_timer_to_default_state();

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
