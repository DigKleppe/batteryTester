/*
 * lcd.h
 *
 *  Created on: Jul 1, 2024
 *      Author: dig
 */

#ifndef MAIN_INCLUDE_LCD_H_
#define MAIN_INCLUDE_LCD_H_

#include "hd44780.h"
extern hd44780_t lcd;

#define LCD_COLS 16
#define LCD_ROWS 4

void LCDprintLine(int line, char *str);

#endif /* MAIN_INCLUDE_LCD_H_ */
