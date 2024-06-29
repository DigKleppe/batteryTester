/*
 * currentregulatorTask.cpp
 *
 *  Created on: Jun 7, 2024
 *      Author: dig
 */

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "Arduino.h"
#include "INA226.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "hal/gpio_types.h"
#include "tester.h"

static const char *TAG = "cr";

#define AVGS  INA226_1_SAMPLE  // INA226_4_SAMPLES //INA226_16_SAMPLES //   INA226_1024_SAMPLES
#define RCURRENTSENSE 0.1 // on INA board

#define CHARGEPIN1 GPIO_NUM_1
#define DECHARGEPIN1 GPIO_NUM_2
#define CHARGEPIN2 GPIO_NUM_1
#define DECHARGEPIN2 GPIO_NUM_2
#define CHARGEPIN3 GPIO_NUM_1
#define DECHARGEPIN3 GPIO_NUM_2
#define CHARGEPIN4 GPIO_NUM_1
#define DECHARGEPIN4 GPIO_NUM_2

const gpio_num_t chargePin[] = {CHARGEPIN1, CHARGEPIN2, CHARGEPIN3, CHARGEPIN4};
const gpio_num_t deChargePin[] = {DECHARGEPIN1, DECHARGEPIN2, DECHARGEPIN3,
                                  DECHARGEPIN4};

INA226 ina1(0x40); // A0 A1 gnd (default)
INA226 ina2(0x41); // A0 VSS
INA226 ina3(0x44); // A1 VSS
INA226 ina4(0x45); // A0 A1 VSS

INA226 *ina[] = {&ina1, &ina2, &ina3, &ina4};

bool currentRegulatorStarted;

void currentRegulatorTask(void *pvParameter) {
  int cycles = 0;
  Wire.begin();

  for (int n = 0; n < NR_CHANNELS; n++) {
    testChannel[n].status = STATUS_NO_BAT;

    if (!(ina[n]->begin()))
      testChannel[n].status = STATUS_INA_ERROR;
    else {
      if (ina[n]->setMaxCurrentShunt(0.81, RCURRENTSENSE, true) !=
          INA226_ERR_NONE)
        testChannel[n].status = STATUS_INA_ERROR;
      else
        ina[n]->setAverage(AVGS); // for volatage , current and power
    }
    gpio_set_level(chargePin[n], 0);
    gpio_set_level(deChargePin[n], 0);
    gpio_set_direction(chargePin[n], GPIO_MODE_OUTPUT);
    gpio_set_direction(deChargePin[n], GPIO_MODE_OUTPUT);
  }
  while (1) {
    for (int n = 0; n < NR_CHANNELS; n++) {
      if (testChannel[n].status != STATUS_INA_ERROR) {
        testChannel[n].voltage = ina[n]->getBusVoltage();
        testChannel[n].averagedCurrent = ina[n]->getCurrent_mA();
        testChannel[n].measuredPower = ina[n]->getPower();
        testChannel[n].current =
            (ina[n]->getShuntVoltage() * 1000.0) / RCURRENTSENSE; // real time

        if (testChannel[n].setCurrent == 0) {
          gpio_set_level(chargePin[n], 0);
          gpio_set_level(deChargePin[n], 0);
        } else {
          if (testChannel[n].setCurrent > 0) { // charge
            gpio_set_level(deChargePin[n], 0);
            if (testChannel[n].current > testChannel[n].setCurrent)
              gpio_set_level(chargePin[n], 0);
            else
              gpio_set_level(chargePin[n], 1);
          } else {
            gpio_set_level(chargePin[n], 0);

            if (testChannel[n].current <
                testChannel[n].setCurrent) // negative current
              gpio_set_level(deChargePin[n], 0);
            else
              gpio_set_level(deChargePin[n], 1);
          }
        }
      }
    }
    vTaskDelay(5);
    if (cycles++ > 10) // avererage
      currentRegulatorStarted = true;
  }
}
