/*
 * lcdtest.cpp
 *
 *  Created on: Jun 14, 2024
 *      Author: dig
 */

#include <hd44780.h>
#include <hd44780ioClass/hd44780_NTCU20025ECPB_pinIO.h> // Arduino pin i/o class header

const int rs = 15, rw = 16, en = 17, db4 = 4, db5 = 5, db6 = 6,
          db7 = 7; // for all other devices

hd44780_NTCU20025ECPB_pinIO lcd(rs, rw, en, db4, db5, db6, db7);

static const int dummyvar = 0; // dummy declaration for older broken IDEs!!!!
// vi:ts=4
// ----------------------------------------------------------------------------
// LCDLibTest - LCD library Test sketch
// Copyright 2012-2020 Bill perry
// ---------------------------------------------------------------------------
//
//  LCDlibTest is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation version 3 of the License.
//
//  LCDlibTest is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with LCDLibTest.  If not, see <http://www.gnu.org/licenses/>.
//
// ---------------------------------------------------------------------------
//
// Sketch to measure and report the speed of the interface to the LCD and
// speed of updating the LCD as well as test some of the library extensions
// for hardware control like backlight control.
//
// It runs a Frames/Sec (FPS) test which writes a "frame" of each digit 0-9 to
// the display.
// A "frame" is a full display of characters.
// It is created by positioning the cursor to the begining of each row
// and then writing a character to every position on the row, until the
// entire display is filled.
// The FPS test does a frame of 9's then 8's, .... down to 0's
// On fast interfaces it will not normally be seen.
//
// The sketch will then calculate & report transfer speeds and
// LCD update rates to the LCD display.
//
// Reported Information:
// - Single byte transfer speed (ByteXfer)
//		This is the time it takes for a single character to be sent from
//		the sketch to the LCD display.
//
// - Frame/Sec (FPS)
//		This is the number of times the full display can be updated
//		in one second.
//
// - Frame Time (Ftime)
//		This is the amount of time it takes to update the full LCD
// display.
//
//
// The sketch will also report "independent" FPS and Ftime values.
// These are timing values that are independent of the size of the LCD under
// test. Currently they represent the timing for a 16x2 LCD The value of always
// having numbers for a 16x2 display is that these numbers can be compared to
// each other since they are independent of the size of the actual LCD display
// that is running the test.
//
// All times & rates are measured and calculeted from what a sketch "sees"
// using the LiquidCrystal API.
// It includes any/all s/w overhead including the time to go through the
// Arduino Print class and LCD library.
// The actual low level hardware times are obviously lower.
//
// History
// 2018.03.23 bperrybap - bumped default instruction time to 38us
// 2012.04.01 bperrybap - Original creation
//
// @author Bill Perry - bperrybap@opensource.billsworld.billandterrie.com
// ---------------------------------------------------------------------------

#ifndef HD44780_LCDOBJECT

// #error "Special purpose sketch: Use i/o class example wrapper sketch
// instead."

/*
 * If not using a hd44780 library i/o class example wrapper sketch,
 * you must modify the sketch to include any needed header files for the
 * intended library and define the lcd object.
 *
 * Add your includes and constructor.
 * The lcd object must be named "lcd"
 * and comment out the #error message.
 */

#endif

#ifndef LCD_COLS
#define LCD_COLS 16
#endif

#ifndef LCD_ROWS
#define LCD_ROWS 4
#endif

// ============================================================================
// End of user configurable options
// ============================================================================

// Turn on extra stuff for certain libraries
//

#if defined(hd44780_h) || defined(LiquidCrystal_I2C_h) ||                      \
    (defined(_LCD_H_) && defined(FOUR_BITS) && defined(BACKLIGHT_ON))
// #define ONOFFCMDS	// If on() and off() commands exist
// #define SETBACKLIGHTCMD	// if setbacklight() exists
// #define BACKLIGHTCMDS	// if backlight()/noBacklight() exist
#endif

// Data format is for each custom character is 8 bytes
// Pixels within the bytes is as follows:
// lowest byte is at top, MSB of byte is on right
// only lower 5 bits of each byte is used.

// Data for a set of new characters for a bargraph
// start with 1 underbar and add additional bars until full 5x8 block

const uint8_t charBitmap[][8] = {
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1f}, // char 0
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x1f, 0x1f}, // char 1
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x1f, 0x1f, 0x1f}, // char 2
    {0x00, 0x00, 0x00, 0x00, 0x1f, 0x1f, 0x1f, 0x1f}, // char 3
    {0x00, 0x00, 0x00, 0x1f, 0x1f, 0x1f, 0x1f, 0x1f}, // char 4
    {0x00, 0x00, 0x1f, 0x1f, 0x1f, 0x1f, 0x1f, 0x1f}, // char 5
    {0x00, 0x1f, 0x1f, 0x1f, 0x1f, 0x1f, 0x1f, 0x1f}, // char 6
    {0x1f, 0x1f, 0x1f, 0x1f, 0x1f, 0x1f, 0x1f, 0x1f}, // char 7
};
/*
 * prototypes
 */

void initLCD() {
  int cntr = 0;
  char str[17];
  int charBitmapSize = (sizeof(charBitmap) / sizeof(charBitmap[0]));

  if (!lcd.begin(LCD_COLS, LCD_ROWS)) {
    lcd.setCursor(0, 0);
    lcd.print("Hello, World!");
  } else {
    while (1)
      ;
  }

  // create the custom bargraph characters
  for (int i = 0; i < charBitmapSize; i++) {
    lcd.createChar(i, (uint8_t *)charBitmap[i]);
  }

  while (1) {
    lcd.setCursor(0, 1);
    sprintf (str,"Hello %d",cntr++);
    lcd.print(str);
    lcd.setCursor(0, 2);
    lcd.print("Hello line 3");
    lcd.setCursor(0, 3);
    lcd.print("Hello line 4");
    delay(100);
  }

  // lcd.clear();
}

void loop() {}

// fatalError() - loop & blink and error code
void fatalError(int ecode) {
#if defined(hd44780_h)
  // if using hd44780 library use built in fatalError()
  hd44780::fatalError(ecode);
#else
  if (ecode) {
  } // dummy if statement to remove warning about not using ecode
  while (1) {
    delay(1); // delay to prevent WDT on some cores
  }
#endif
}
