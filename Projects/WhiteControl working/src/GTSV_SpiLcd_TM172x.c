#include "main.h"
#include "GTSV_SpiLcd_TM172x.h"


#ifndef LCD_COM_SEG
#define LCD_COM_SEG(lcd_com, lcd_seg)\
			*((volatile unsigned char *)(BITBAND_PERI(LCD_BASE + lcd_com, lcd_seg)))
#endif

#define SPILCD_STB_BIT		BITBAND_POINTER_AT(GPIOA_BASE + ODR_REG_OFFSET, 3)
#define SPILCD_STB_BIT_SET	BITBAND_POINTER_AT(GPIOA_BASE + BSRRL_REG_OFFSET, 3)=1
#define SPILCD_STB_BIT_RESET 	BITBAND_POINTER_AT(GPIOA_BASE + BSRRH_REG_OFFSET, 3)=1



//this is the offset of the com channel address in memory relative to the LCD_RAM_BASE (or COM0 Offset)
//the table is the offset of MCU com channel that drive the LCD pins: COM0, COM1, COM2, COM3
static const uint8_t _lcd_com[4] =
					{0x14, 0x1C, 0x24, 0x2C};
//this is the segment offset relative to MCU segment 0 ,
//the table is the segment offset of MCU segment channel that drive the LCD SEGMENT: SEG0... SEG13
//to change the MCU segment, just change the offset
//currently MCUSeg0 driving LCDSeg0, MCUSeg1 driving LCDSeg1, ..., MCUSeg7 driving  LCDSeg4
//Ex: if you want MCUSeg0(offset=0) driving LCDSeg8 then _lcd_segment_offset[8]=0
static const uint8_t _lcd_segment[18] =
					{0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17};

//change from number to 7 segments: g f e d c b a
//Ex: number 0 will have: g=0, f=1, e=1, d=1, c=1, b=1, a=1 => the binary: 0111111 or hex:0x3F
//0,1, 2, 3, 4, 5, 6, 7, 8, 9, F, blank
static const uint8_t _number_to_7segments[12] =
					{0x3F, 0x06, 0x5B, 0x4F, 0x66, 0x6D, 0x7D, 0x07, 0x7F, 0x6F, 0x71, 0x00};


static uint8_t _lcd_buf[16];

volatile int j;
void delay_lcd(void)
{
	//j=0;
}

void Spilcd_flush_buf_to_lcd(void)
{
	uint8_t i;



	SPILCD_STB_BIT = 0;
	SPI1->DR = 0x40;  //write data to LCD ram/ auto increment
	while(SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_BSY)){}
	SPILCD_STB_BIT = 1;
	SPILCD_STB_BIT = 0;
	SPI1->DR = 0xC2; //begin at address 0x02
	while(SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_BSY)){}

	for(i=2; i<15; i++){
		SPI1->DR = _lcd_buf[i];
		while(SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_BSY)){}
	}
	SPILCD_STB_BIT = 1;

}

void Lcd_clear(void)
{
	uint8_t i;

	SPILCD_STB_BIT = 0;
	SPI1->DR = 0x40;
	while(SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_BSY)){}
	SPILCD_STB_BIT = 1;
	SPILCD_STB_BIT = 0;
	SPI1->DR = 0xC0;
	while(SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_BSY)){}

	for(i=0; i<16; i++){
		SPI1->DR = 0x00;
		while(SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_BSY)){}
	}
	SPILCD_STB_BIT = 1;
}

void Lcd_set(void)
{
	uint8_t i;

	SPILCD_STB_BIT = 0;
	SPI1->DR = 0x40;
	while(SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_BSY)){}
	SPILCD_STB_BIT = 1;
	SPILCD_STB_BIT = 0;
	SPI1->DR = 0xC0;
	while(SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_BSY)){}

	for(i=0; i<16; i++){
		SPI1->DR = 0xFF;
		while(SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_BSY)){}
	}
	SPILCD_STB_BIT = 1;
}
/*

void Spilcd_flush_to_spi_lcd(void)
{
	uint32_t _com[4] = {LCD->RAM[LCD_RAMRegister_0],
					LCD->RAM[LCD_RAMRegister_2],
					LCD->RAM[LCD_RAMRegister_4],
					LCD->RAM[LCD_RAMRegister_6]};
	uint8_t i;
	uint8_t bytes[16];


	for(i=0; i<16; i++){

		bytes[i] = (uint8_t)( ((_com[0]>>(2*i))&0x00000001)
							|((_com[1]>>(2*i))&0x00000001) << 1
							|((_com[2]>>(2*i))&0x00000001) << 2
							|((_com[3]>>(2*i))&0x00000001) << 3
							|((_com[0]>>((2*i)+1))&0x00000001) << 4
							|((_com[1]>>((2*i)+1))&0x00000001) << 5
							|((_com[2]>>((2*i)+1))&0x00000001) << 6
							|((_com[3]>>((2*i)+1))&0x00000001) << 7 );


	}



}
*/


volatile uint8_t _lcd_blink_cursor;
volatile uint8_t _lcd_fan_cursor_slow=0;
volatile uint8_t _lcd_fan_cursor_fast=0;
void Lcd_blink_tick50ms(void)
{

	static uint8_t cnt_50ms=0;
	static uint8_t cnt_50ms_fan_fast=0;
	static uint8_t cnt_50ms_fan_slow=0;

	if(++cnt_50ms==20)
	cnt_50ms = 0;

	if(cnt_50ms<10){
		//_lcd_icons_buf |= _lcd_blink_mask;
		_lcd_blink_cursor = 1;

	}else{
		//_lcd_icons_buf &= ~(_lcd_blink_mask);
		_lcd_blink_cursor = 0;

	}


	if(++cnt_50ms_fan_fast==3){
		cnt_50ms_fan_fast = 0;
		if(++_lcd_fan_cursor_fast==3)
			_lcd_fan_cursor_fast=0;
	}
	if(++cnt_50ms_fan_slow==6){
		cnt_50ms_fan_slow= 0;
		if(++_lcd_fan_cursor_slow==3)
			_lcd_fan_cursor_slow=0;
	}
}

 uint8_t Lcd_get_blink_cursor(void)
 {
	return _lcd_blink_cursor;
 }

uint8_t Lcd_get_fan_cursor_slow(void)
{
	return _lcd_fan_cursor_slow;
}

uint8_t Lcd_get_fan_cursor_fast(void)
{
	return _lcd_fan_cursor_fast;
}


void Spilcd_to_default_config(void)
{
	SPI_InitTypeDef SPI_InitStructure;
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_SPI1, ENABLE);
	SPI_I2S_DeInit(SPI1);
	SPI_InitStructure.SPI_Direction = SPI_Direction_1Line_Tx;
	SPI_InitStructure.SPI_DataSize = SPI_DataSize_8b;
	SPI_InitStructure.SPI_CPOL = SPI_CPOL_High;
	SPI_InitStructure.SPI_CPHA = SPI_CPHA_2Edge;
	SPI_InitStructure.SPI_NSS = SPI_NSS_Soft;
	SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_16;  //clk~1us
	SPI_InitStructure.SPI_FirstBit = SPI_FirstBit_LSB;
	SPI_InitStructure.SPI_CRCPolynomial = 7;
	SPI_InitStructure.SPI_Mode = SPI_Mode_Master;
  	SPI_Init(SPI1, &SPI_InitStructure);
	//SPI_SSOutputCmd(SPI1, ENABLE);
	SPI_Cmd(SPI1, ENABLE);


	while(SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_BSY)){}
	SPILCD_STB_BIT = 0;
	SPI1->DR = 0x00;
	while(SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_BSY)){}
	SPILCD_STB_BIT = 1;
	SPILCD_STB_BIT = 1;
	SPILCD_STB_BIT = 1;

	SPILCD_STB_BIT = 0;
	SPI1->DR = 0x97;
	while(SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_BSY)){}
	SPILCD_STB_BIT = 1;
	SPILCD_STB_BIT = 1;
	SPILCD_STB_BIT = 1;

	Lcd_clear();
}

void Spilcd_configure_GPIO(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	//config GPIO for SPI1
	//for STB pins
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOA, ENABLE);
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
	GPIO_SetBits(GPIOA, GPIO_Pin_3);


	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);



	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  	GPIO_InitStructure.GPIO_PuPd  = GPIO_PuPd_DOWN;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5 | GPIO_Pin_7;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
	GPIO_Init( GPIOA, &GPIO_InitStructure);

	GPIO_PinAFConfig(GPIOA, GPIO_PinSource5,GPIO_AF_SPI1);
	GPIO_PinAFConfig(GPIOA, GPIO_PinSource7,GPIO_AF_SPI1);
}
