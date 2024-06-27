/*
 * currentregulatorTask.cpp
 *
 *  Created on: Jun 7, 2024
 *      Author: dig
 */

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "driver/gpio.h"
#include "esp_log.h"
#include "Arduino.h"

static const char *TAG = "cr";

#define CHARGEPIN1 GPIO_NUM_1
#define DECHARGEPIN1 GPIO_NUM_2


#define CHARGERCPIN2 GPIO_NUM_16
#define CHARGERCPIN3 GPIO_NUM_17
#define CHARGERCPIN4 GPIO_NUM_18

#include "INA226.h"

INA226 ina(0x40);

float wantedCurrent = 100; // mA
float maxVoltage = 2.5;

void currentRegulatorTask(void *pvParameter) {

	float vBus;
	float current;
	
	Wire.begin();

	if (!ina.begin()) {
		ESP_LOGE(TAG, "could not connect. Fix and Reboot\n\n");

		while (1)
			vTaskDelay(25);

	}
	ina.setMaxCurrentShunt(0.81, 0.1, true);

	gpio_set_direction(CHARGEPIN1, GPIO_MODE_OUTPUT);
	gpio_set_direction(DECHARGEPIN1, GPIO_MODE_OUTPUT);

/*	gpio_set_level(DECHARGEPIN1, 0);
	do {
		vBus = ina.getBusVoltage();
		current = ina.getCurrent_mA();
		
		if ( current > wantedCurrent)
			gpio_set_level(CHARGEPIN1, 0);
		else
			gpio_set_level(CHARGEPIN1, 1);

		printf("bus voltage: %3.2f \t", vBus);
		printf("current: %3.0f\n", current);
		vTaskDelay(5);
	} while (1);
*/
	gpio_set_level(CHARGEPIN1, 0);
	do {
		vBus = ina.getBusVoltage();
		current = -ina.getCurrent_mA();
		
		if ( current > wantedCurrent)
			gpio_set_level(DECHARGEPIN1, 0);
		else
			gpio_set_level(DECHARGEPIN1, 1);

		printf("bus voltage: %3.2f \t", vBus);
		printf("current: %3.0f\n", current);
		vTaskDelay(5);
	} while (1);



}