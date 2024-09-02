/*
 * lcd.cpp
 *
 *  Created on: Jun 14, 2024
 *      Author: dig
 */

#include "lcd.h"
#include "hd44780.h"
#include "string.h"

hd44780_t lcd = {
    .write_cb = NULL,
    .pins = {
        .rs = GPIO_NUM_15,
        .e  = GPIO_NUM_17,
        .d4 = GPIO_NUM_4,
        .d5 = GPIO_NUM_5,
        .d6 = GPIO_NUM_6,
        .d7 = GPIO_NUM_7,
        .bl = HD44780_NOT_USED
    },
    .font = HD44780_FONT_5X8,
    .lines = 4
 };

void LCDprintLine(int line, char *str) {
    hd44780_gotoxy(&lcd, 0, line-1);
	for (int x = strlen(str); x < LCD_COLS; x++)
		str[x] = ' '; // fill with spaces
	str[LCD_COLS] = 0;

	hd44780_puts(&lcd,str);
}



bool initLCD() {
  ESP_ERROR_CHECK(hd44780_init(&lcd));
  return true;
}


