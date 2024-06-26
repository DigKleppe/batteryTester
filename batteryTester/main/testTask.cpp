/*
 * testTask.cpp
 *
 *  Created on: Jun 28, 2024
 *      Author: dig
 */

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "INA226.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "lcd.h"
#include "tester.h"
#include <Arduino.h>
#include <stdio.h>

static const char *TAG = "testTask";

testChannel_t testChannel[NR_CHANNELS];
extern bool currentRegulatorStarted;

bool isFull(int idx) {
  float diff;

  if (testChannel[idx].voltage > MAXCHARGEDVOLATGE) {
    ESP_LOGE(TAG, "full voltage max voltage exeeded ");
    return true;
  }
  diff = testChannel[idx].maxVoltage - testChannel[idx].voltage;
  if (diff > CHARGEDDROP) {
    ESP_LOGE(TAG, "full voltage dropped %f ", diff);
    return true;
  }
  if (testChannel[idx].maxVoltage < testChannel[idx].voltage)
    testChannel[idx].maxVoltage = testChannel[idx].voltage;
  return false;
}

void testTask(void *pvParameter) {
  TickType_t xLastWakeTime;
  int insertTimer = 2;
  char LCDline[400];
  int len;
  int cntr = 0;

  while (!currentRegulatorStarted)
    vTaskDelay(100);

  for (int n = 0; n < NR_CHANNELS; n++) {
    if (testChannel[n].status == STATUS_INA_ERROR) {
      ESP_LOGE(TAG, "ina %d+1 error", n);
    } else {
      ESP_LOGI(TAG, "ina %d+1 initialized", n);
    }
    testChannel[n].chargeCurrent = 400;
    testChannel[n].deChargeCurrent = 400;
  }
  xLastWakeTime = xTaskGetTickCount();
  while (1) {
    memset(LCDline, 0, LCD_COLS + 1); // clr buffer
    for (int n = 0; n < NR_CHANNELS; n++) {
      len = sprintf(LCDline, "%d", n + 1);
      switch (testChannel[n].status) {
      case STATUS_INA_ERROR:
        strcpy(LCDline + len, " INA fout");
        break;

      case STATUS_NO_BAT:
        ESP_LOGI(TAG, "%d no battery %4.3f V", n + 1, testChannel[n].voltage);
        if (testChannel[n].voltage < NOBATVOLTAGE) {
          strcpy(LCDline + len, "");
          if (insertTimer-- == 0) {
            ESP_LOGI(TAG, "battery %d inserted", n + 1);
            testChannel[n].measuredCapacity = 0;
            testChannel[n].measuredPower = 0;
            testChannel[n].setCurrent = testChannel[n].chargeCurrent;
            testChannel[n].maxVoltage = 0;
            testChannel[n].status = STATUS_CHARGING;
          }
        } else {
          insertTimer = 2;
          strcpy(LCDline + len, " Geen batterij");
        }
        break;
      case STATUS_CHARGING:
        ESP_LOGI(TAG, "%d charging %d mA %4.3f V", n + 1,
                 testChannel[n].averagedCurrent, testChannel[n].voltage);
        sprintf(LCDline + len, "L %dmA %4.3fV", testChannel[n].averagedCurrent,
                testChannel[n].voltage);
        if (isFull(n)) {
          testChannel[n].inPower = testChannel[n].measuredPower;
          testChannel[n].setCurrent = 0;
          testChannel[n].status = STATUS_WAIT1;
          cntr = 2;
        }
        break;

      case STATUS_WAIT1:
        sprintf(LCDline + len, "O %dmA %4.3fV", testChannel[n].averagedCurrent,
                testChannel[n].voltage);
        ESP_LOGI(TAG, "%d wait1 %d mA %4.3f V", n + 1, testChannel[n].current,
                 testChannel[n].voltage);

        if (testChannel[n].current < NOCURRENT) {
          testChannel[n].measuredCapacity = 0;
          testChannel[n].samples = 0;
          testChannel[n].measuredPower = 0;
          testChannel[n].setCurrent = -testChannel[n].deChargeCurrent;
          testChannel[n].status = STATUS_DECHARGING;
        }
        break;

      case STATUS_DECHARGING:
        ESP_LOGI(TAG, "%d decharging %d mA %4.3f V", n + 1,
                 -testChannel[n].averagedCurrent, testChannel[n].voltage);
        sprintf(LCDline + len, "O %dmA %4.3fV", -testChannel[n].averagedCurrent,
                testChannel[n].voltage);

        testChannel[n].measuredCapacity += testChannel[n].averagedCurrent;
        testChannel[n].samples++;

        if ((testChannel[n].voltage < DECHARDEDVOLATAGE) ||
            (testChannel[n].voltage >
             NOBATVOLTAGE)) // removed
                            // testChannel[n].outPower =
                            // testChannel[n].measuredPower;
        {
          //          testChannel[n].measuredPower = 0;
          testChannel[n].measuredCapacity =
              testChannel[n].measuredCapacity / 3600; // to mAh
          testChannel[n].setCurrent = 0;
          testChannel[n].status = STATUS_WAIT2;
        }
        break;

      case STATUS_WAIT2:
        sprintf(LCDline + len, "O %dmA %4.3fV", -testChannel[n].averagedCurrent,
                testChannel[n].voltage);
        ESP_LOGI(TAG, "%d wait2 %d mA %4.3f V", n + 1, testChannel[n].current,
                 testChannel[n].voltage);
        if (testChannel[n].current < NOCURRENT) {
          testChannel[n].setCurrent = testChannel[n].chargeCurrent;
          testChannel[n].status = STATUS_TESTED;
          testChannel[n].maxVoltage = 0;
        }
        break;

      case STATUS_TESTED: // recharge after testing
        ESP_LOGI(TAG, "%d tested charging %d mA %4.3f V %d mAH", n + 1,
                 testChannel[n].averagedCurrent, testChannel[n].voltage,
                 testChannel[n].measuredCapacity);

        sprintf(LCDline + len, "T*%dmAh* %4.3fV",
                testChannel[n].measuredCapacity, testChannel[n].voltage);

        if (isFull(n)) {
          testChannel[n].status = STATUS_CHARGED;
          testChannel[n].setCurrent = 10; // trickle
        }
        break;

      case STATUS_CHARGED:
        ESP_LOGI(TAG, "%d ready  %d mA %4.3f V", n + 1,
                 testChannel[n].averagedCurrent, testChannel[n].voltage);

        sprintf(LCDline + len, "T *%dmAh* Vol",
                testChannel[n].measuredCapacity);

        if (testChannel[n].voltage > NOBATVOLTAGE) {
          testChannel[n].status = STATUS_NO_BAT;
        }
        break;
      }
      LCDline[LCD_COLS] = 0; // limit to actual value
      lcd.setCursor(0, n);

      for (int x = strlen(LCDline); x < LCD_COLS; x++)
        LCDline[x] = ' '; // fill with spaces

      lcd.print(LCDline);

      xTaskDelayUntil(&xLastWakeTime, 250);
    }

    // xTaskDelayUntil(&xLastWakeTime, 1000);
  }
}