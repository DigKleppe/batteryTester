/*
 * lcd.h
 *
 *  Created on: Jul 1, 2024
 *      Author: dig
 */

#ifndef MAIN_INCLUDE_LCD_H_
#define MAIN_INCLUDE_LCD_H_

#include <hd44780.h>
#include <hd44780ioClass/hd44780_NTCU20025ECPB_pinIO.h> // Arduino pin i/o class header

extern hd44780_NTCU20025ECPB_pinIO lcd;

#define LCD_COLS 20
#define LCD_ROWS 4	

#endif /* MAIN_INCLUDE_LCD_H_ */
