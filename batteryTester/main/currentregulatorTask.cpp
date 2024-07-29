/*
 * currentregulatorTask.cpp
 *
 *  Created on: Jun 7, 2024
 *      Author: dig
 */

#include "Arduino.h"
#include "INA226.h"
#include "averager.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "hal/gpio_types.h"
#include "measureTask.h"
#include "settings.h"

static const char *TAG = "cr";

#define INA_AVGS INA226_1_SAMPLE // INA226_4_SAMPLES //INA226_16_SAMPLES // INA226_1024_SAMPLES
#define RCURRENTSENSE 0.1        // on INA board

#define AVGS 64 // current and voltage

#define CHARGEPIN1 GPIO_NUM_1
#define DECHARGEPIN1 GPIO_NUM_2
#define CHARGEPIN2 GPIO_NUM_41
#define DECHARGEPIN2 GPIO_NUM_42
#define CHARGEPIN3 GPIO_NUM_39
#define DECHARGEPIN3 GPIO_NUM_40
#define CHARGEPIN4 GPIO_NUM_37
#define DECHARGEPIN4 GPIO_NUM_38



const gpio_num_t chargePin[] = { CHARGEPIN1, CHARGEPIN2, CHARGEPIN3, CHARGEPIN4 };
const gpio_num_t deChargePin[] = { DECHARGEPIN1, DECHARGEPIN2, DECHARGEPIN3, DECHARGEPIN4 };

uint16_t manID[NR_CHANNELS];
uint16_t dieId[NR_CHANNELS];

INA226 ina1 (0x40); // A0 A1 gnd (default)
INA226 ina2 (0x41); // A0 VSS
INA226 ina3 (0x44); // A1 VSS
INA226 ina4 (0x45); // A0 A1 VSS

INA226 *ina[] = { &ina1, &ina2, &ina3, &ina4 };

Averager avg1 (AVGS);
Averager avg2 (AVGS);
Averager avg3 (AVGS);
Averager avg4 (AVGS);
Averager avg5 (AVGS);
Averager avg6 (AVGS);
Averager avg7 (AVGS);
Averager avg8 (AVGS);

Averager *iavg[] = { &avg1, &avg2, &avg3, &avg4 };
Averager *vavg[] = { &avg5, &avg6, &avg7, &avg8 };

bool currentRegulatorStarted;

void
currentRegulatorTask (void *pvParameter)
{
  int cycles = 0;
  TickType_t xLastWakeTime;
  xLastWakeTime = xTaskGetTickCount ();
  Wire.begin ();
 // Wire.setClock (400000);

  for (int n = 0; n < NR_CHANNELS; n++)
    {
      testChannel[n].status = STATUS_NO_BAT;
      if (!(ina[n]->begin ()))
        testChannel[n].status = STATUS_INA_ERROR;
      else
        {
          if (ina[n]->setMaxCurrentShunt (0.81, RCURRENTSENSE, true) != INA226_ERR_NONE)
            testChannel[n].status = STATUS_INA_ERROR;
          else
            ina[n]->setAverage (INA_AVGS); // for volatage , current and power
          manID[n] = ina[n]->getManufacturerID ();
          dieId[n] = ina[n]->getDieID ();
        }
     
      gpio_reset_pin(chargePin[n]);
      gpio_reset_pin(deChargePin[n]);
      gpio_set_level (chargePin[n], 0);
      gpio_set_level (deChargePin[n], 0);
      gpio_set_direction (chargePin[n], GPIO_MODE_OUTPUT);
      gpio_set_direction (deChargePin[n], GPIO_MODE_OUTPUT);
    }
  while (1)
    {
      for (int n = 0; n < NR_CHANNELS; n++)
        {
          if (testChannel[n].status != STATUS_INA_ERROR)
            {

              vavg[n]->write (ina[n]->getBusVoltage () * 1000.0);

              testChannel[n].voltage = vavg[n]->average () / 1000.0;

              //      testChannel[n].measuredPower = ina[n]->getPower();
              testChannel[n].current = ((ina[n]->getShuntVoltage () * 1000.0) / RCURRENTSENSE) * userSettings.currentGain[n]; // real time
              iavg[n]->write (testChannel[n].current);
              testChannel[n].averagedCurrent = iavg[n]->average ();
              if (testChannel[n].setCurrent == 0)
                {
                  gpio_set_level (chargePin[n], 0);
                  gpio_set_level (deChargePin[n], 0);
                }
              else
                {
                  if (testChannel[n].setCurrent > 0)
                    { // charge
                      gpio_set_level (deChargePin[n], 0);
                      if (testChannel[n].current > testChannel[n].setCurrent)
                        gpio_set_level (chargePin[n], 0);
                      else
                        gpio_set_level (chargePin[n], 1);
                    }
                  else
                    {
                      gpio_set_level (chargePin[n], 0);
                      if (testChannel[n].current < testChannel[n].setCurrent) // negative current
                        gpio_set_level (deChargePin[n], 0);
                      else
                        gpio_set_level (deChargePin[n], 1);
                    }
                }
            }
        }
      xTaskDelayUntil (&xLastWakeTime, 5);
      if (cycles++ > 10) // avererage
        currentRegulatorStarted = true;
    }
}
