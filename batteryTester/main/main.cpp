#include "driver/gpio.h"
#include "esp_log.h"
#include "keys.h"
 
#include "tester.h"
#include <stdbool.h>
#include <stdio.h>
#include <unistd.h>

#define KEYPIN GPIO_NUM_14

void currentRegulatorTask(void *pvParameter);
static const char *TAG = "main";
bool initLCD();

SemaphoreHandle_t LCDsemphr;

myKey_t getKeyPins(void) {
  if (gpio_get_level(KEYPIN))
    return 0;

  return 1;
}

#define NOKEYTIME 5 // seconds to end setup

typedef enum uiState { uiStateWAIT, uiStateSET } uiState_t;

const int stdCapacity[] = {500, 750, 1000, 1500, 2000, 2500, 3000, 0};

void LCDprintLine(int line, char *str) {
  lcd.setCursor(0, line - 1);
  for (int x = strlen(str); x < LCD_COLS; x++)
    str[x] = ' '; // fill with spaces
  str[LCD_COLS] = 0;

  lcd.print(str);
}

void guiTask(void *pvParameter) {
  uiState_t state = uiStateWAIT;
  int timer = 0;
  int selCapIdx = 0;
  int channelIdx;
  char LCDline[400];

  do {
    for (int n = 0; n < NR_CHANNELS; n++) {
      switch (state) {
      case uiStateWAIT:
        if (testChannel[n].status == STATUS_SETUP) {
          channelIdx = n;
          if (xSemaphoreTakeRecursive(LCDsemphr, (TickType_t)100) == pdTRUE) {
            timer = NOKEYTIME * 10;
            state = uiStateSET;
          }
        }
        break;

      case uiStateSET:
        if (n == channelIdx) {
          sprintf(LCDline, "Capaciteit %d:", n + 1);
          LCDprintLine(1, LCDline);
          sprintf(LCDline, "%d mAHr", stdCapacity[selCapIdx]);
          LCDprintLine(2, LCDline);
          sprintf(LCDline, "Laadstroom: %d:", n + 1);
          LCDprintLine(3, LCDline);
          sprintf(LCDline, "%d mA", stdCapacity[selCapIdx] / 5);
          LCDprintLine(4, LCDline);

          if (key(1)) {
            selCapIdx++;
            if (stdCapacity[selCapIdx] == 0)
              selCapIdx = 0;
            timer = NOKEYTIME * 10;
          }

          if (timer-- == 0) {
            testChannel[n].deChargeCurrent = stdCapacity[selCapIdx] / 5;
            testChannel[n].chargeCurrent = stdCapacity[selCapIdx] / 5;
            testChannel[n].setCurrent = testChannel[n].chargeCurrent;
            testChannel[n].status = STATUS_CHARGING; //
            state = uiStateWAIT;
            xSemaphoreGiveRecursive(LCDsemphr);
          }
        }
        break;
      }
    }

    vTaskDelay(100);

  } while (1);
}

extern "C" void app_main(void) {
  LCDsemphr = xSemaphoreCreateRecursiveMutex();

  vTaskDelay(100);
  initLCD();

  gpio_set_direction(KEYPIN, GPIO_MODE_INPUT);
  keysRepeat = 1; // only 1 key

  xTaskCreate(&currentRegulatorTask, "crTask", 1024 * 2, NULL,
              configMAX_PRIORITIES, NULL);
  xTaskCreate(&testTask, "testTask", 1024 * 4, NULL, 0, NULL);
  xTaskCreate(&guiTask, "guiTask", 1024 * 4, NULL, 0, NULL);

  while (true) {
    //  printf("Hello from app_main!\n");
    vTaskDelay(10);
    keysTimerHandler_ms(10);
  }
}
