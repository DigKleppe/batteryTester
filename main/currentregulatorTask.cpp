/*
 * currentregulatorTask.cpp
 *
 *  Created on: Jun 7, 2024
 *      Author: dig
 */

#include "ina226.h"
#include "averager.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "hal/gpio_types.h"
#include "measureTask.h"
#include "settings.h"
#include <string.h>

static const char *TAG = "cr";

#define I2CCLOCKSPEED_HZ 400000

#define INA_AVGS INA226_1_SAMPLE // INA226_4_SAMPLES //INA226_16_SAMPLES // INA226_1024_SAMPLES
#define RCURRENTSENSE 0.1        // on INA board

#define AVGS 64 // current

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


Averager avg1(AVGS);
Averager avg2(AVGS);
Averager avg3(AVGS);
Averager avg4(AVGS);

Averager *iavg[] = { &avg1, &avg2, &avg3, &avg4 };

static ina226_t ina[NR_CHANNELS];
static uint8_t I2CAddress[] = { 0x40, 0x41, 0x44, 0x45 };

bool currentRegulatorStarted;
extern i2c_master_bus_handle_t i2cBusHandle;

void currentRegulatorTask(void *pvParameter) {
	int cycles = 0;
	esp_err_t err;
	float shuntVoltage;

//	TickType_t xLastWakeTime;
//	xLastWakeTime = xTaskGetTickCount();

	for (int n = 0; n < NR_CHANNELS; n++) {
		testChannel[n].status = STATUS_NO_BAT;
		memset(&ina[n], 0, sizeof(ina226_t));
		err = ina226_init_desc(&ina[n], I2CAddress[n], I2C_NUM_0 , I2CCLOCKSPEED_HZ);
		err |= i2c_master_bus_add_device(i2cBusHandle, &ina[n].i2c_dev.cfg, &ina[n].i2c_dev.dev_handle);
		err |= i2c_master_probe(i2cBusHandle, I2CAddress[n], 100);
		if (err)
			printf("Error probing ina226 %02x\n", I2CAddress[n]);
		else {
			printf("probing ina226 %02x OK\n", I2CAddress[n]);
			err = ina226_init(&ina[n]);
			err |= ina226_set_config(&ina[n], INA226_MODE_CONT_SHUNT_BUS, INA226_AVG_1, INA226_CT_1100, INA226_CT_1100);
		}
		if (err)
			printf("Error initializing ina226 %02x\n", I2CAddress[n]);

		gpio_reset_pin(chargePin[n]);
		gpio_reset_pin(deChargePin[n]);
		gpio_set_level(chargePin[n], 0);
		gpio_set_level(deChargePin[n], 0);
		gpio_set_direction(chargePin[n], GPIO_MODE_OUTPUT);
		gpio_set_direction(deChargePin[n], GPIO_MODE_OUTPUT);
	}
	while (1) {
		for (int n = 0; n < NR_CHANNELS; n++) {
			if (testChannel[n].status != STATUS_INA_ERROR) {
		        ESP_ERROR_CHECK(ina226_get_bus_voltage(&ina[n], &testChannel[n].voltage));
		        ESP_ERROR_CHECK(ina226_get_shunt_voltage(&ina[n], &shuntVoltage));
				testChannel[n].current = ((shuntVoltage * 1000.0) / RCURRENTSENSE) * userSettings.currentGain[n]; // real time
				iavg[n]->write(testChannel[n].current);
				testChannel[n].averagedCurrent = iavg[n]->average();
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
						if (testChannel[n].current < testChannel[n].setCurrent) // negative current
							gpio_set_level(deChargePin[n], 0);
						else
							gpio_set_level(deChargePin[n], 1);
					}
				}
			}
		}
		vTaskDelay(1);
	//	xTaskDelayUntil(&xLastWakeTime, 3);
		if (cycles++ > 10) // avererage
			currentRegulatorStarted = true;
	}
}
