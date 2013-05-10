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
//uint32_t _LCD_RAM[8];

uint16_t msTicks;

uint8_t hours=0, mins=0;


GPIO_InitTypeDef GPIO_InitStructure;
EXTI_InitTypeDef EXTI_InitStructure;
NVIC_InitTypeDef NVIC_InitStructure;
TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;



uint32_t tmp_ir_cmd;



void sys_state_change_to_CLK_ADJ(void)
{
	gSystemFlags.tmp_hour = RTC_TimeStructure.RTC_Hours;
	gSystemFlags.tmp_min = RTC_TimeStructure.RTC_Minutes;
	gSystemFlags.time_adj_stage =0;
	gSystemFlags.sys_state = SYS_STATE_CLK_ADJ;
	gSystemFlags.time_adj_delay=0;
}

void save_tmp_clock_and_change_to_OFF(void)
{
	RTC_change_time(gSystemFlags.tmp_hour, gSystemFlags.tmp_min, 0);
	//save time
	//send cmd update time to serial
	LED_TIMER_BT = 0;
	LED_PLUS_BT = 0;
	LED_MINUS_BT = 0;
	gSystemFlags.sys_state = SYS_STATE_OFF;
	
}

void main_big_switch(void)
{
	switch(gSystemFlags.sys_state){
	case SYS_STATE_OFF:
		//update current time
		RTC_GetTime(RTC_Format_BIN, &RTC_TimeStructure);
		Lcd_fill_hours(RTC_TimeStructure.RTC_Hours);
		Lcd_fill_mins(RTC_TimeStructure.RTC_Minutes);
		
		//key Timer
		if(Tsense_check_rising_edge(TSENSE_KEY_TIMER) ||
			(tmp_ir_cmd == IRR_NEC_CMD_TIMER)){
			gSystemFlags.tmp_hour = RTC_TimeStructure.RTC_Hours;
			gSystemFlags.tmp_min = RTC_TimeStructure.RTC_Minutes;
			gSystemFlags.time_adj_stage =0;
			gSystemFlags.sys_state = SYS_STATE_CLK_ADJ;
			gSystemFlags.time_adj_delay=0;
			
		}
		//key plus
		if(Tsense_check_rising_edge(TSENSE_KEY_PLUS) ||
			(tmp_ir_cmd == IRR_NEC_CMD_SPEEDUP) ){
			Blower_set_speed(1);
			gSystemFlags.sys_state = SYS_STATE_BLOWING;
		}
		//key minus
		if(Tsense_check_rising_edge(TSENSE_KEY_MINUS) ||
			(tmp_ir_cmd == IRR_NEC_CMD_SPEEDDOWN) ){
			Blower_set_speed(4);
			gSystemFlags.sys_state = SYS_STATE_BLOWING;
		}
		//key auto
		if(Tsense_check_rising_edge(TSENSE_KEY_AUTO) ||
			(tmp_ir_cmd == IRR_NEC_CMD_AUTO) ||
			(tmp_ir_cmd == IRR_NEC_CMD_ONOFF)){
			gSystemFlags.sys_state = SYS_STATE_AUTO;
		}
		//blink colon icon
		if(Lcd_get_blink_cursor())
			Lcd_icon_on(LCD_COLON_ICON);
		else
			Lcd_icon_off(LCD_COLON_ICON);
		break;
	case SYS_STATE_AUTO:
		//update current time
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

		//key Auto
		if(Tsense_check_rising_edge(TSENSE_KEY_AUTO) ||
			(tmp_ir_cmd == IRR_NEC_CMD_AUTO) ||
			(tmp_ir_cmd == IRR_NEC_CMD_ONOFF)){
			gSystemFlags.sys_state = SYS_STATE_OFF;
			LED_AUTO_BT =0;
			Lcd_icon_off(LCD_ROTATE_ICON);
		}
		
		break;
	case SYS_STATE_CLK_ADJ:
		Lcd_icon_on(LCD_COLON_ICON);
		LED_TIMER_BT = 1;
		LED_PLUS_BT = 1;
		LED_MINUS_BT = 1;
		gSystemFlags.time_adj_delay++;
		if(gSystemFlags.time_adj_stage == 0){ //adj hours
			//key Timer
			if(Tsense_check_rising_edge(TSENSE_KEY_TIMER) || 
				(tmp_ir_cmd == IRR_NEC_CMD_TIMER) ||
				(gSystemFlags.time_adj_delay > 133)){
				gSystemFlags.time_adj_stage++;
				gSystemFlags.time_adj_delay =0;
			}
			//key plus
			if(Tsense_check_rising_edge(TSENSE_KEY_PLUS) ||
			    (Tsense_check_key_hold(TSENSE_KEY_PLUS) && gSystemFlags.ms200_flag) ||
			    (tmp_ir_cmd == IRR_NEC_CMD_SPEEDUP)){
			    gSystemFlags.ms200_flag =0;
			    gSystemFlags.time_adj_delay = 0;
				if(gSystemFlags.tmp_hour == 23)
					gSystemFlags.tmp_hour = 0;
				else
					gSystemFlags.tmp_hour++;
			}
			//key minus
			if(Tsense_check_rising_edge(TSENSE_KEY_MINUS)  ||
			    (Tsense_check_key_hold(TSENSE_KEY_MINUS) && gSystemFlags.ms200_flag) ||
			    (tmp_ir_cmd == IRR_NEC_CMD_SPEEDDOWN) ){
			    gSystemFlags.ms200_flag =0;
			    gSystemFlags.time_adj_delay = 0;
				if(gSystemFlags.tmp_hour == 0)
					gSystemFlags.tmp_hour = 23;
				else
					gSystemFlags.tmp_hour--;
			}
			//reset time_adj_delay if any button active
			if(Tsense_check_high_level(TSENSE_KEY_PLUS) ||
				Tsense_check_high_level(TSENSE_KEY_MINUS)||
				Tsense_check_rising_edge(TSENSE_KEY_LIGHT) ||
				Tsense_check_high_level(TSENSE_KEY_LIGHT)||
				tmp_ir_cmd == IRR_NEC_CMD_LIGHT)
				gSystemFlags.time_adj_delay =0;
			
			if((Lcd_get_blink_cursor()) ||
				(Tsense_check_high_level(TSENSE_KEY_PLUS)) ||
				(Tsense_check_high_level(TSENSE_KEY_MINUS)) ){
				Lcd_fill_hours(gSystemFlags.tmp_hour);
			}else{
				Lcd_fill_hours(88);
			}
			Lcd_fill_mins(gSystemFlags.tmp_min);
			
		}else{  //adj mins
			//key timer
			if(Tsense_check_rising_edge(TSENSE_KEY_TIMER)|| 
				(tmp_ir_cmd == IRR_NEC_CMD_TIMER)||
				(gSystemFlags.time_adj_delay > 133)){
				save_tmp_clock_and_change_to_OFF();
			}
			//key plus
			if(Tsense_check_rising_edge(TSENSE_KEY_PLUS) ||
			    (Tsense_check_key_hold(TSENSE_KEY_PLUS) && gSystemFlags.ms200_flag) ||
			    (tmp_ir_cmd == IRR_NEC_CMD_SPEEDUP)){
			    gSystemFlags.ms200_flag =0;
			    gSystemFlags.time_adj_delay = 0;
				if(gSystemFlags.tmp_min == 59)
					gSystemFlags.tmp_min= 0;
				else
					gSystemFlags.tmp_min++;
			}
			//key minus
			if(Tsense_check_rising_edge(TSENSE_KEY_MINUS)  ||
			    (Tsense_check_key_hold(TSENSE_KEY_MINUS) && gSystemFlags.ms200_flag)||
			    (tmp_ir_cmd == IRR_NEC_CMD_SPEEDDOWN) ){
			    gSystemFlags.ms200_flag =0;
			    gSystemFlags.time_adj_delay = 0;
				if(gSystemFlags.tmp_min == 0)
					gSystemFlags.tmp_min = 59;
				else
					gSystemFlags.tmp_min--;
			}
			//reset time_adj delay
			if(Tsense_check_high_level(TSENSE_KEY_PLUS) ||
				Tsense_check_high_level(TSENSE_KEY_MINUS)||
				Tsense_check_rising_edge(TSENSE_KEY_LIGHT) ||
				Tsense_check_high_level(TSENSE_KEY_LIGHT)||
				tmp_ir_cmd == IRR_NEC_CMD_LIGHT)
				gSystemFlags.time_adj_delay =0;
			
			Lcd_fill_hours(gSystemFlags.tmp_hour);
			if(Lcd_get_blink_cursor() ||
				Tsense_check_high_level(TSENSE_KEY_PLUS) ||
				Tsense_check_high_level(TSENSE_KEY_MINUS) ){
				Lcd_fill_mins(gSystemFlags.tmp_min);
			}else{
				Lcd_fill_mins(88);
			}
		}
		
		break;
	case SYS_STATE_BLOWING:
		LED_PLUS_BT = 1;
		LED_MINUS_BT = 1;
		if(gSystemFlags.blower_fan_speed>2)
			Lcd_icon_fan(Lcd_get_fan_cursor_fast());
		else
			Lcd_icon_fan(Lcd_get_fan_cursor_slow());
		Lcd_icon_off(LCD_COLON_ICON);
		Lcd_fill_pos_with_blank(0);
		Lcd_fill_pos_with_blank(3);
		Lcd_fill_pos_with_num(2, gSystemFlags.blower_fan_speed);
		Lcd_fill_pos_with_num(1, 10);
		//key plus
		if(Tsense_check_rising_edge(TSENSE_KEY_PLUS) ||
			(tmp_ir_cmd == IRR_NEC_CMD_SPEEDUP) ){
			if(gSystemFlags.blower_fan_speed == 4){
				Blower_set_speed(0);
				gSystemFlags.sys_state = SYS_STATE_OFF;
				LED_PLUS_BT = 0;
				LED_MINUS_BT = 0;
			}else{
				Blower_set_speed(++gSystemFlags.blower_fan_speed);
			}
		}
		//key minus
		if(Tsense_check_rising_edge(TSENSE_KEY_MINUS) ||
			(tmp_ir_cmd == IRR_NEC_CMD_SPEEDDOWN) ){
			
			if(gSystemFlags.blower_fan_speed == 1){
				Blower_set_speed(0);
				gSystemFlags.sys_state = SYS_STATE_OFF;
				LED_PLUS_BT = 0;
				LED_MINUS_BT = 0;
			}else{
				Blower_set_speed(--gSystemFlags.blower_fan_speed);
			}
		}
		//key auto
		if(Tsense_check_rising_edge(TSENSE_KEY_AUTO) ||
			(tmp_ir_cmd == IRR_NEC_CMD_AUTO) ||
			(tmp_ir_cmd == IRR_NEC_CMD_ONOFF)){
			Blower_set_speed(0);
			gSystemFlags.sys_state = SYS_STATE_AUTO;
		}
		//key timer
		if(Tsense_check_rising_edge(TSENSE_KEY_TIMER) ||
			(tmp_ir_cmd == IRR_NEC_CMD_TIMER)){
			gSystemFlags.blower_apo_mins_tmp =1;
			gSystemFlags.sys_state = SYS_STATE_BLOWING_APO_ADJ;
			gSystemFlags.time_adj_delay =0;
		}

		break;
	case SYS_STATE_BLOWING_APO_ADJ:
		gSystemFlags.time_adj_delay++;
		LED_TIMER_BT = 1;
		if(gSystemFlags.blower_fan_speed>2)
			Lcd_icon_fan(Lcd_get_fan_cursor_fast());
		else
			Lcd_icon_fan(Lcd_get_fan_cursor_slow());
		Lcd_icon_on(LCD_COLON_ICON);
		Lcd_icon_on(LCD_CLOCK_ICON);
		Lcd_fill_mins(0);
		if(Lcd_get_blink_cursor() ||
			Tsense_check_high_level(TSENSE_KEY_PLUS) ||
			Tsense_check_high_level(TSENSE_KEY_MINUS) ){
			Lcd_fill_hours(gSystemFlags.blower_apo_mins_tmp);
			if(gSystemFlags.blower_apo_mins_tmp<10)
				Lcd_fill_pos_with_blank(0);
		}else{
			Lcd_fill_hours(88);
		}

		//key plus
		if(Tsense_check_rising_edge(TSENSE_KEY_PLUS) ||
		    (Tsense_check_key_hold(TSENSE_KEY_PLUS) && gSystemFlags.ms500_flag) ||
		    (tmp_ir_cmd == IRR_NEC_CMD_SPEEDUP)){
		    gSystemFlags.ms500_flag =0;
		    gSystemFlags.time_adj_delay = 0;
			if(gSystemFlags.blower_apo_mins_tmp == 15)
				gSystemFlags.blower_apo_mins_tmp= 1;
			else
				gSystemFlags.blower_apo_mins_tmp++;
		}
		//key minus
		if(Tsense_check_rising_edge(TSENSE_KEY_MINUS)  ||
		    (Tsense_check_key_hold(TSENSE_KEY_MINUS) && gSystemFlags.ms500_flag)||
		    (tmp_ir_cmd == IRR_NEC_CMD_SPEEDDOWN) ){
		    gSystemFlags.ms500_flag =0;
		    gSystemFlags.time_adj_delay = 0;
			if(gSystemFlags.blower_apo_mins_tmp == 1)
				gSystemFlags.blower_apo_mins_tmp = 15;
			else
				gSystemFlags.blower_apo_mins_tmp--;
		}
		//reset time_adj_delay
		if(Tsense_check_high_level(TSENSE_KEY_PLUS) ||
			Tsense_check_high_level(TSENSE_KEY_MINUS)||
			Tsense_check_rising_edge(TSENSE_KEY_LIGHT) ||
			Tsense_check_high_level(TSENSE_KEY_LIGHT)||
			tmp_ir_cmd == IRR_NEC_CMD_LIGHT)
			gSystemFlags.time_adj_delay =0;
		//key Timer
		if(Tsense_check_rising_edge(TSENSE_KEY_TIMER)|| 
			(tmp_ir_cmd == IRR_NEC_CMD_TIMER)){
			gSystemFlags.sys_state = SYS_STATE_BLOWING;
			Lcd_icon_off(LCD_CLOCK_ICON);
		}
		//auto change to Blowing auto power off
		if(gSystemFlags.time_adj_delay>133){
			gSystemFlags.sys_state = SYS_STATE_BLOWING_APO;
			//get current time
			//RTC_GetTime(RTC_Format_BIN, &(gSystemFlags.blower_apo_begin));
			RTC_GetTime(RTC_Format_BIN, &(gSystemFlags.blower_apo_end));
			//calculate off time
			if((gSystemFlags.blower_apo_end.RTC_Minutes + gSystemFlags.blower_apo_mins_tmp)<60){
				gSystemFlags.blower_apo_end.RTC_Minutes += gSystemFlags.blower_apo_mins_tmp;
			}else {
				gSystemFlags.blower_apo_end.RTC_Minutes += gSystemFlags.blower_apo_mins_tmp -60;
				if(gSystemFlags.blower_apo_begin.RTC_Hours<23)
					gSystemFlags.blower_apo_end.RTC_Hours += 1;
				else 
					gSystemFlags.blower_apo_end.RTC_Hours = 0;
				
			}
			
			
			
		}
		break;
	case SYS_STATE_BLOWING_APO:
		LED_TIMER_BT = 1;
		LED_PLUS_BT = 1;
		LED_MINUS_BT = 1;
		if(Lcd_get_blink_cursor())
			Lcd_icon_on(LCD_CLOCK_ICON);
		else
			Lcd_icon_off(LCD_CLOCK_ICON);
		if(gSystemFlags.blower_fan_speed>2)
			Lcd_icon_fan(Lcd_get_fan_cursor_fast());
		else
			Lcd_icon_fan(Lcd_get_fan_cursor_slow());
		Lcd_icon_off(LCD_COLON_ICON);
		Lcd_fill_pos_with_blank(0);
		Lcd_fill_pos_with_blank(3);
		Lcd_fill_pos_with_num(2, gSystemFlags.blower_fan_speed);
		Lcd_fill_pos_with_num(1, 10);
		
		//key plus
		if(Tsense_check_rising_edge(TSENSE_KEY_PLUS) ||
			(tmp_ir_cmd == IRR_NEC_CMD_SPEEDUP) ){
			if(gSystemFlags.blower_fan_speed == 4){
				Blower_set_speed(0);
				gSystemFlags.sys_state = SYS_STATE_OFF;
				Lcd_icon_off(LCD_CLOCK_ICON);
				LED_PLUS_BT = 0;
				LED_MINUS_BT = 0;
				LED_TIMER_BT =0;
			}else{
				Blower_set_speed(++gSystemFlags.blower_fan_speed);
			}
		}
		//key minus
		if(Tsense_check_rising_edge(TSENSE_KEY_MINUS) ||
			(tmp_ir_cmd == IRR_NEC_CMD_SPEEDDOWN) ){
			
			if(gSystemFlags.blower_fan_speed == 1){
				Blower_set_speed(0);
				gSystemFlags.sys_state = SYS_STATE_OFF;
				Lcd_icon_off(LCD_CLOCK_ICON);
				LED_PLUS_BT = 0;
				LED_MINUS_BT = 0;
				LED_TIMER_BT =0;
			}else{
				Blower_set_speed(--gSystemFlags.blower_fan_speed);
			}
		}
		//key auto
		if(Tsense_check_rising_edge(TSENSE_KEY_AUTO) ||
			(tmp_ir_cmd == IRR_NEC_CMD_AUTO) ||
			(tmp_ir_cmd == IRR_NEC_CMD_ONOFF)){
			Blower_set_speed(0);
			gSystemFlags.sys_state = SYS_STATE_AUTO;
			Lcd_icon_off(LCD_CLOCK_ICON);
			LED_PLUS_BT = 0;
			LED_MINUS_BT = 0;
			LED_TIMER_BT =0;
		}
		//key timer
		if(Tsense_check_rising_edge(TSENSE_KEY_TIMER)|| 
			(tmp_ir_cmd == IRR_NEC_CMD_TIMER)){
			gSystemFlags.sys_state = SYS_STATE_BLOWING;
			Lcd_icon_off(LCD_CLOCK_ICON);
		}

		RTC_GetTime(RTC_Format_BIN, &RTC_TimeStructure);
		if((RTC_TimeStructure.RTC_Seconds == gSystemFlags.blower_apo_end.RTC_Seconds) &&
			(RTC_TimeStructure.RTC_Minutes == gSystemFlags.blower_apo_end.RTC_Minutes) &&
			(RTC_TimeStructure.RTC_Hours == gSystemFlags.blower_apo_end.RTC_Hours)){
			Buzzer_bip();
			Blower_set_speed(0);
			gSystemFlags.sys_state = SYS_STATE_OFF;
			Lcd_icon_off(LCD_CLOCK_ICON);
			LED_PLUS_BT = 0;
			LED_MINUS_BT = 0;
			LED_TIMER_BT =0;
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


	Cpu_to_default_config();
	RTC_to_default_config();
	Ports_to_default_config();
  	Lcd_to_default_config();
	Timers_to_default_config();

#ifdef DEBUG
	//get system config for debug purposes
	Get_system_clk_config();
#endif

	Irr_init();
	
	Tsense_to_default_config();
	LED_BACKLIGHT = 1;
  /*Until application reset*/
  while (1)
  {
  	/* Run TSL RC state machine */
	Tsense_action();
	Tsense_key_detect_first();
	
	if(Irr_decode(&irr_decode_results)){
		tmp_ir_cmd= irr_decode_results.value;

		switch(tmp_ir_cmd){
		case IRR_NEC_CMD_LIGHT:
			Buzzer_bip();
			break;

		case IRR_NEC_CMD_TIMER:
			
			Buzzer_bip();
			break;
		case IRR_NEC_CMD_AUTO:
		case IRR_NEC_CMD_ONOFF:

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


		Irr_resume();
	}else {
		tmp_ir_cmd=0;
	}
	

	main_big_switch();
	
  	

//for any SYS State 
	if(Tsense_check_rising_edge(TSENSE_KEY_LIGHT) ||
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
	
	Tsense_key_detect_last();
  }
}



void auto_power_off_check_time(void)
{
	RTC_GetTime(RTC_Format_BIN, &RTC_TimeStructure);
	if((RTC_TimeStructure.RTC_Seconds == gSystemFlags.blower_apo_end.RTC_Seconds) &&
		(RTC_TimeStructure.RTC_Minutes == gSystemFlags.blower_apo_end.RTC_Minutes) &&
		(RTC_TimeStructure.RTC_Hours == gSystemFlags.blower_apo_end.RTC_Hours)){
		Buzzer_bip();
		Blower_set_speed(0);
		gSystemFlags.sys_state = SYS_STATE_OFF;
		Lcd_icon_off(LCD_CLOCK_ICON);
		LED_PLUS_BT = 0;
		LED_MINUS_BT = 0;
		LED_TIMER_BT =0;
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
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = (1 << __NVIC_PRIO_BITS) -2;
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
