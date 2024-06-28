/*
 * testTask.cpp
 *
 *  Created on: Jun 28, 2024
 *      Author: dig
 */

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "Arduino.h"
#include "INA226.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "tester.h"

static const char *TAG = "testTask";

testChannel_t testChannel[NR_CHANNELS];
extern bool currentRegulatorStarted;

void testTask(void *pvParameter) {
  while (!currentRegulatorStarted)
    vTaskDelay(100);

  for (int n = 0; n < NR_CHANNELS; n++) {
    if (testChannel[n].status == STATUS_INA_ERROR) {
      ESP_LOGE(TAG, "ina %d+1 error", n);
    } else {
      ESP_LOGI(TAG, "ina %d+1 initialized", n);
    }
  }
  for (int n = 0; n < NR_CHANNELS; n++) {
    switch (testChannel[n].status) {
    case STATUS_INA_ERROR:

      break;

    case STATUS_NO_BAT:
      if (testChannel[n].voltage < NOBATVOLTAGE) {
        ESP_LOGI(TAG, "%d+1 No bat", n);
        ESP_LOGI(TAG, "battery %d+1 inserted", n);
        testChannel[n].measuredCapacity = 0;
        testChannel[n].measuredPower = 0;
        testChannel[n].setCurrent = testChannel[n].chargeCurrent;
        testChannel[n].status = STATUS_CHARGING;
      }
      break;
    case STATUS_CHARGING:
      ESP_LOGI(TAG, "%d+1 charging %d mA %f V", n,
               testChannel[n].averagedCurrent, testChannel[n].voltage);
      if (testChannel[n].voltage > CHARGEDVOLATGE) {
        testChannel[n].inPower = testChannel[n].measuredPower;
        testChannel[n].measuredCapacity = 0;
        testChannel[n].measuredPower = 0;
        testChannel[n].setCurrent = -testChannel[n].deChargeCurrent;
        testChannel[n].status = STATUS_DECHARGING;
      }
      break;

    case STATUS_DECHARGING:
      ESP_LOGI(TAG, "%d+1 decharging %d mA %f V", n,
               testChannel[n].averagedCurrent, testChannel[n].voltage);

      if (testChannel[n].voltage < DECHARDEDVOLATAGE) {
        testChannel[n].outPower = testChannel[n].measuredPower;
        testChannel[n].measuredCapacity = 0;
        testChannel[n].measuredPower = 0;

        testChannel[n].setCurrent = testChannel[n].chargeCurrent;
        testChannel[n].status = STATUS_TESTED;
      }
      break;

    case STATUS_TESTED:
      ESP_LOGI(TAG, "%d+1 charging %d mA %f V", n,
               testChannel[n].averagedCurrent, testChannel[n].voltage);
      if (testChannel[n].voltage > CHARGEDVOLATGE) {
        testChannel[n].status = STATUS_CHARGED;
        testChannel[n].setCurrent = 0;
      }
      break;

    case STATUS_CHARGED:
      ESP_LOGI(TAG, "%d+1 ready  %d mA %f V", n, testChannel[n].averagedCurrent,
               testChannel[n].voltage);
      if (testChannel[n].voltage > NOBATVOLTAGE) {
        testChannel[n].status = STATUS_NO_BAT;
      }
      break;
    }
  }
  vTaskDelay(100);
}