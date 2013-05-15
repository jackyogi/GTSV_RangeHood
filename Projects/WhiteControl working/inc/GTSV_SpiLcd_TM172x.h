/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __GTSV_SPILCD_TM172x_H_INCLUDED
#define __GTSV_SPILCD_TM172x_H_INCLUDED
#include "main.h"


enum Lcd_Icons {
	LCD_ICON_CLOCK,
	LCD_ICON_LIGHT,
	LCD_ICON_COLON1,
	LCD_ICON_COLON2,
	LCD_ICON_FAN1,
	LCD_ICON_FAN2,
	LCD_ICON_FAN3,
	LCD_ICON_ALL
};


extern uint8_t _lcd_buf[16];

void Spilcd_configure_GPIO(void);
void Spilcd_to_default_config(void);

void Spilcd_flush_buf_to_lcd(void);
void Lcd_clear(void);
void Lcd_set(void);


void Lcd_icon_on(enum Lcd_Icons icon);
void Lcd_icon_off(enum Lcd_Icons icon);
void Lcd_icon_toggle(enum Lcd_Icons icon);
void Lcd_icon_fan(uint8_t num);


uint8_t Lcd_get_blink_cursor(void);
uint8_t Lcd_get_fan_cursor_slow(void);
uint8_t Lcd_get_fan_cursor_fast(void);


#endif
