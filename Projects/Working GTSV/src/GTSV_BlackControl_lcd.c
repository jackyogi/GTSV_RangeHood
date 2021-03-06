/**
  ******************************************************************************
  * @file    stm32l_discovery_lcd.c
  * @author  Microcontroller Division
  * @version V1.0.0
  * @date    Apri-2011
  * @brief   This file includes driver for the glass LCD Module mounted on
  *          STM32l discovery board MB963
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
  * <h2><center>&copy; COPYRIGHT 2011 STMicroelectronics</center></h2>
  */

/* Includes ------------------------------------------------------------------*/
#include "GTSV_BlackControl_lcd.h"
#include "GTSV_BlackControl_board.h"
#include "stm32l1xx_lcd.h"
#include "main.h"


#ifndef BITBAND_PERI
#define BITBAND_PERI(a,b) ((PERIPH_BB_BASE + (a-PERIPH_BASE)*32 + (b*4)))
#endif
#ifndef BITBAND_SRAM
#define BITBAND_SRAM(a,b) ((PERIPH_BB_BASE + (a-PERIPH_BASE)*32 + (b*4)))
#endif



#define LCD_RAM_BASE	LCD_BASE

#ifndef LCD_COM_SEG
#define LCD_COM_SEG(lcd_com, lcd_seg)\
			*((volatile unsigned char *)(BITBAND_PERI(LCD_RAM_BASE + lcd_com, lcd_seg)))
#endif

#define LCD_PIXEL(px)\
			*((volatile unsigned char *)(BITBAND_PERI(LCD_RAM_BASE + LCD_COM_##px, LCD_SEG_##px)))
#define LCD_PIXEL_ON(px)\
			LCD_PIXEL(px) = 1
			//*((volatile unsigned char *)(BITBAND_PERI(LCD_RAM_BASE + LCD_COM_##px, LCD_SEG_##px)))=1

#define LCD_PIXEL_OFF(px)\
			LCD_PIXEL(px) = 0


//this is the offset of the com channel address in memory relative to the LCD_RAM_BASE (or COM0 Offset)
//the table is the offset of MCU com channel that drive the LCD pins: COM0, COM1, COM2, COM3
//to change the MCU com, just change the offset 
//currently MCU COM0(0x00) driving LCDCOM0, ... MCU COM1(0x08) driving LCDCOM1...
//Ex: if  you want MCU COM0 driving LCDCOM3 then _lcd_com_offset[3] = 0;
static const uint8_t _lcd_com[4] = 
					{0x14, 0x1C, 0x24, 0x2C}; 

//this is the segment offset relative to MCU segment 0 , 
//the table is the segment offset of MCU segment channel that drive the LCD SEGMENT: SEG0... SEG13
//to change the MCU segment, just change the offset
//currently MCUSeg0 driving LCDSeg0, MCUSeg1 driving LCDSeg1, ..., MCUSeg7 driving  LCDSeg4
//Ex: if you want MCUSeg0(offset=0) driving LCDSeg8 then _lcd_segment_offset[8]=0 
static const uint8_t _lcd_segment[10] = 
					{17, 28, 29, 30, 31, 7, 8, 9, 16, 27};

//change from number to 7 segments: g f e d c b a
//Ex: number 0 will have: g=0, f=1, e=1, d=1, c=1, b=1, a=1 => the binary: 0111111 or hex:0x3F
static const uint8_t _number_to_7segments[10] = 
					{0x3F, 0x06, 0x5B, 0x4F, 0x66, 0x6D, 0x7D, 0x07, 0x7F, 0x6F}; 


//num should be 0..9
//pos should be 0..3
void Lcd_fill_pos_with_num(uint8_t pos, uint8_t num)
{
		uint8_t segments = _number_to_7segments[num];
	uint8_t i, j;
	i= _lcd_segment[2*pos+1];
	j= _lcd_segment[2*pos+2];

	LCD_COM_SEG(_lcd_com[3], j) = (segments & 1);
	LCD_COM_SEG(_lcd_com[2], j) = (segments & (1<<1))>>1;
	LCD_COM_SEG(_lcd_com[1], j) = (segments & (1<<2))>>2;
	LCD_COM_SEG(_lcd_com[0], j) = (segments & (1<<3))>>3;
	LCD_COM_SEG(_lcd_com[1], i)   = (segments & (1<<4))>>4;
	LCD_COM_SEG(_lcd_com[2], i)   = (segments & (1<<6))>>6;
	LCD_COM_SEG(_lcd_com[3], i)   = (segments & (1<<5))>>5;


	
}

void Lcd_fill_num_to_position(uint8_t num, uint8_t pos)
{
	Lcd_fill_pos_with_num(pos, num);
}


void Lcd_fill_hours(uint8_t num)
{
	Lcd_fill_pos_with_num(1, num%10);
	Lcd_fill_pos_with_num(0, num/10);
}
void Lcd_fill_mins(uint8_t num)
{
	Lcd_fill_pos_with_num(3, num%10);
	Lcd_fill_pos_with_num(2, num/10);
}



static uint16_t _lcd_icons=0;;; //for saving current state of all icons.

/*
struct Lcd_Icon_State {
	unsigned clock:1;
	unsigned light_bulb:1;
	unsigned light_ray:1;
	unsigned fan1:1;
	unsigned fan2:1;
	unsigned fan3:1;
	unsigned rotate:1;
	unsigned colon:1;
} ;
struct Lcd_Icon_State _lcd_icons_state;
*/

void Lcd_icon_buff_flush(void)
{
	LCD_COM_SEG(_lcd_com[0], _lcd_segment[0]) = (_lcd_icons & (1<<LCD_CLOCK_ICON))>>LCD_CLOCK_ICON;
	LCD_COM_SEG(_lcd_com[0], _lcd_segment[1]) = (_lcd_icons & (1<<LCD_LIGHTRAY_ICON))>>LCD_LIGHTRAY_ICON;
	LCD_COM_SEG(_lcd_com[0], _lcd_segment[3]) = (_lcd_icons & (1<<LCD_LIGHTBULB_ICON))>>LCD_LIGHTBULB_ICON;
	LCD_COM_SEG(_lcd_com[0], _lcd_segment[5]) = (_lcd_icons & (1<<LCD_COLON_ICON))>>LCD_COLON_ICON;
	LCD_COM_SEG(_lcd_com[0], _lcd_segment[9]) = (_lcd_icons & (1<<LCD_ROTATE_ICON))>>LCD_ROTATE_ICON;
	LCD_COM_SEG(_lcd_com[1], _lcd_segment[9]) = (_lcd_icons & (1<<LCD_FAN1_ICON))>>LCD_FAN1_ICON;
	LCD_COM_SEG(_lcd_com[2], _lcd_segment[9]) = (_lcd_icons & (1<<LCD_FAN2_ICON))>>LCD_FAN2_ICON;
	LCD_COM_SEG(_lcd_com[3], _lcd_segment[9]) = (_lcd_icons & (1<<LCD_FAN3_ICON))>>LCD_FAN3_ICON;
}	

void Lcd_icon_on(enum Lcd_Icons icon)
{
	_lcd_icons |= (1<<icon);
	Lcd_icon_buff_flush();
}

void Lcd_icon_off(enum Lcd_Icons icon)
{
	if(icon == LCD_ALL_ICON)
		_lcd_icons = 0;
	else
		_lcd_icons  &= ~(1<<icon);
	Lcd_icon_buff_flush();
}

void Lcd_icon_toggle(enum Lcd_Icons icon)
{
	
	_lcd_icons ^= (1<<icon);
	Lcd_icon_buff_flush();
}

void Lcd_icon_fan(uint8_t num)
{

		if(num==0){
			Lcd_icon_on(LCD_FAN1_ICON);
			Lcd_icon_off(LCD_FAN2_ICON);
			Lcd_icon_off(LCD_FAN3_ICON);
		}
		if(num==1){
			Lcd_icon_off(LCD_FAN1_ICON);
			Lcd_icon_on(LCD_FAN2_ICON);
			Lcd_icon_off(LCD_FAN3_ICON);
		}
		if(num==2){
			Lcd_icon_off(LCD_FAN1_ICON);
			Lcd_icon_off(LCD_FAN2_ICON);
			Lcd_icon_on(LCD_FAN3_ICON);
		}
}

void Lcd_icon_fan_rotate(FunctionalState st)
{
	
}
/**
  * @brief  Configures the LCD GLASS relative GPIO port IOs and LCD peripheral.
  * @param  None
  * @retval None
  */
void Lcd_to_default_config(void)
{
  LCD_InitTypeDef LCD_InitStruct;

  RCC_APB1PeriphClockCmd(RCC_APB1Periph_LCD,ENABLE);
  
  LCD_InitStruct.LCD_Prescaler = LCD_Prescaler_1;
  LCD_InitStruct.LCD_Divider = LCD_Divider_31;
  LCD_InitStruct.LCD_Duty = LCD_Duty_1_4;
  LCD_InitStruct.LCD_Bias = LCD_Bias_1_3;
  LCD_InitStruct.LCD_VoltageSource = LCD_VoltageSource_Internal;


  /* Initialize the LCD */
  LCD_Init(&LCD_InitStruct);

  LCD_MuxSegmentCmd(ENABLE);

  /* To set contrast to mean value */
  LCD_ContrastConfig(LCD_Contrast_Level_7);

  LCD_DeadTimeConfig(LCD_DeadTime_0);
  LCD_PulseOnDurationConfig(LCD_PulseOnDuration_6);

  /* Wait Until the LCD FCR register is synchronized */
  LCD_WaitForSynchro();

  /* Enable LCD peripheral */
  LCD_Cmd(ENABLE);

  /* Wait Until the LCD is enabled */
  while(LCD_GetFlagStatus(LCD_FLAG_ENS) == RESET)
  {
  }
  /*!< Wait Until the LCD Booster is ready */
  while(LCD_GetFlagStatus(LCD_FLAG_RDY) == RESET)
  {
  }

  LCD_BlinkConfig(LCD_BlinkMode_Off,LCD_BlinkFrequency_Div32);
  Lcd_clear();
}

/**
  * @brief  To initialize the LCD pins
  * @caller main
  * @param None
  * @retval None
  */

void Lcd_configure_GPIO(void)
{
  GPIO_InitTypeDef GPIO_InitStructure;

/* Enable GPIOs clock */
  RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOA | RCC_AHBPeriph_GPIOB | RCC_AHBPeriph_GPIOC |
                        RCC_AHBPeriph_GPIOD | RCC_AHBPeriph_GPIOE | RCC_AHBPeriph_GPIOH, ENABLE);


/* Configure Output for LCD */
/* Port A */

  GPIO_StructInit(&GPIO_InitStructure);
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8 | GPIO_Pin_9 | GPIO_Pin_10 | GPIO_Pin_15;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
  GPIO_Init( GPIOA, &GPIO_InitStructure);

  GPIO_PinAFConfig(GPIOA, GPIO_PinSource8,GPIO_AF_LCD) ;
  GPIO_PinAFConfig(GPIOA, GPIO_PinSource9,GPIO_AF_LCD) ;
  GPIO_PinAFConfig(GPIOA, GPIO_PinSource10,GPIO_AF_LCD) ;
  GPIO_PinAFConfig(GPIOA, GPIO_PinSource15,GPIO_AF_LCD) ;

/* Configure Output for LCD */
/* Port B */
  GPIO_InitStructure.GPIO_Pin =    GPIO_Pin_3 | GPIO_Pin_4 | GPIO_Pin_5 | GPIO_Pin_8 | GPIO_Pin_9;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
  GPIO_Init( GPIOB, &GPIO_InitStructure);

  GPIO_PinAFConfig(GPIOB, GPIO_PinSource3,GPIO_AF_LCD) ;
  GPIO_PinAFConfig(GPIOB, GPIO_PinSource4,GPIO_AF_LCD) ;
  GPIO_PinAFConfig(GPIOB, GPIO_PinSource5,GPIO_AF_LCD) ;
  GPIO_PinAFConfig(GPIOB, GPIO_PinSource8,GPIO_AF_LCD) ;
  GPIO_PinAFConfig(GPIOB, GPIO_PinSource9,GPIO_AF_LCD) ;

/* Configure Output for LCD */
/* Port C*/

  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9 | GPIO_Pin_10 | GPIO_Pin_11 | GPIO_Pin_12 ;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
  GPIO_Init( GPIOC, &GPIO_InitStructure);

  GPIO_PinAFConfig(GPIOC, GPIO_PinSource9,GPIO_AF_LCD) ;
  GPIO_PinAFConfig(GPIOC, GPIO_PinSource10,GPIO_AF_LCD) ;
  GPIO_PinAFConfig(GPIOC, GPIO_PinSource11,GPIO_AF_LCD) ;
  GPIO_PinAFConfig(GPIOC, GPIO_PinSource12,GPIO_AF_LCD) ;
  
  /* Port D*/
  
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
	GPIO_Init( GPIOD, &GPIO_InitStructure);
  
	GPIO_PinAFConfig(GPIOD, GPIO_PinSource2,GPIO_AF_LCD) ;



/* Disable GPIOs clock */
  //RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOA | RCC_AHBPeriph_GPIOB | RCC_AHBPeriph_GPIOC |
                        //RCC_AHBPeriph_GPIOD | RCC_AHBPeriph_GPIOE | RCC_AHBPeriph_GPIOH, DISABLE);

}




/**
  * @brief  This function Clear the whole LCD RAM.
  * @param  None
  * @retval None
  */
void Lcd_clear(void)
{
  uint8_t counter = 0;

  /* TO wait LCD Ready */
  while( LCD_GetFlagStatus (LCD_FLAG_UDR) != RESET) ;

  for (counter = LCD_RAMRegister_0; counter <= LCD_RAMRegister_15; counter++)
  {
    LCD->RAM[counter] = 0;
  }

  /* Update the LCD display */
  LCD_UpdateDisplayRequest();

}



/******************* (C) COPYRIGHT 2011 STMicroelectronics *****END OF FILE****/
