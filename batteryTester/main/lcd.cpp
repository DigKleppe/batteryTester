/*
 * lcd.cpp
 *
 *  Created on: Jun 14, 2024
 *      Author: dig
 */


#include "lcd.h" 

const int rs = 15, rw = 16, en = 17, db4 = 4, db5 = 5, db6 = 6,
          db7 = 7; // for all other devices

hd44780_NTCU20025ECPB_pinIO lcd(rs, rw, en, db4, db5, db6, db7);

static const int dummyvar = 0; // dummy declaration for older broken IDEs!!!!

#ifndef HD44780_LCDOBJECT

#endif



#ifndef LCD_COLS
#define LCD_COLS 20
#endif

#ifndef LCD_ROWS
#define LCD_ROWS 4
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

bool initLCD() {
  int cntr = 0;
  char str[17];
  int charBitmapSize = (sizeof(charBitmap) / sizeof(charBitmap[0]));

  if (!lcd.begin(LCD_COLS, LCD_ROWS)) {
	return false;
  //  lcd.setCursor(0, 0);
  //  lcd.print("Hello, World!");
  } 

  // create the custom bargraph characters
  for (int i = 0; i < charBitmapSize; i++) {
    lcd.createChar(i, (uint8_t *)charBitmap[i]);
  }
  lcd.setExecTimes(3000, 50);
  	
  return true;

/*  while (1) {
    lcd.setCursor(0, 1);
    sprintf (str,"Hello %d",cntr++);
    lcd.print(str);
    lcd.setCursor(0, 2);
    lcd.print("Hello line 3");
    lcd.setCursor(0, 3);
    lcd.print("Hello line 4");
    delay(100);
  }*/

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
