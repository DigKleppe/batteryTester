#include "driver/gpio.h"
#include "esp_event.h"
#include "esp_log.h"
#include "keys.h"
#include "settings.h"
#include "wifiConnect.h"
#include "measureTask.h"
#include "lcd.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"

#include "driver/temperature_sensor.h"

#include <stdbool.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

#define KEYPIN GPIO_NUM_14

void currentRegulatorTask(void *pvParameter);
esp_err_t init_spiffs(void);
float temperature;

static const char *TAG = "main";
static 	char LCDline[400];
bool initLCD();

QueueHandle_t LCDsemphr;

myKey_t getKeyPins(void) {
	if (gpio_get_level(KEYPIN))
		return 0;

	return 1;
}

#define NOKEYTIME 2 // seconds to end setup

typedef enum uiState {
	uiStateWAIT, uiStateSET, uiStateTestMode1, uiStateTestMode2
} uiState_t;

const int stdCapacity[] = { 500, 750, 1000, 1500, 2000, 2500, 3000, 0 };

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
	int channelIdx = 0;
	int testModeTimer = 0;

	do {
		for (int n = 0; n < NR_CHANNELS; n++) {
			switch (state) {
			case uiStateWAIT:
				if (testChannel[n].status == STATUS_SETUP) {
					channelIdx = n;
					if (xSemaphoreTakeRecursive(LCDsemphr, (TickType_t) 10000) == pdTRUE) {
						timer = NOKEYTIME * 10;
						state = uiStateSET;
					}
				}
				if (keysRT & 1) {
					key(1); // read key away
					if (testModeTimer++ == 30)
						state = uiStateTestMode1;
				} else {
					testModeTimer = 0;
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

						testChannel[n].setCurrent = -testChannel[n].deChargeCurrent * 2;
						if (testChannel[n].setCurrent < -MAXCURRENT)
							testChannel[n].setCurrent = -MAXCURRENT;
						testChannel[n].status = STATUS_DECHARGING1; // decharge first

						state = uiStateWAIT;
						xSemaphoreGiveRecursive(LCDsemphr);
					}
				}
				break;

			case uiStateTestMode1:
				testChannel[n].status = STATUS_TESTMODE;
				key(1);
				if (!keysRT) // wait key to release
					state = uiStateTestMode2;
				break;

			case uiStateTestMode2:

				if (key(1)) {
					for (int m = 0; m < NR_CHANNELS; m++)
						testChannel[m].status = STATUS_NO_BAT;
					state = uiStateWAIT;
				}
				break;
			}
		}
		vTaskDelay(100);

	} while (1);
}

extern "C" void app_main(void) {
	esp_err_t err;
	int presc = 1;
	bool ipShown = false;
	bool smartConfigShown = false;
	bool tempRangeLow = true;

	LCDsemphr = xSemaphoreCreateRecursiveMutex();

	vTaskDelay(100);
	initLCD();

	err = nvs_flash_init();
	if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
		ESP_ERROR_CHECK(nvs_flash_erase());
		err = nvs_flash_init();
		ESP_LOGI(TAG, "nvs flash erased");
	}
	ESP_ERROR_CHECK(err);

	ESP_ERROR_CHECK(esp_event_loop_create_default());
	ESP_ERROR_CHECK(init_spiffs());

	err = loadSettings();

	gpio_set_direction(KEYPIN, GPIO_MODE_INPUT);
	//  keysRepeat = 1; // only 1 key

	xTaskCreate(&currentRegulatorTask, "crTask", 1024 * 4, NULL, configMAX_PRIORITIES, NULL);
	xTaskCreate(&testTask, "testTask", 1024 * 4, NULL, 0, NULL);
	xTaskCreate(&guiTask, "guiTask", 1024 * 4, NULL, 0, NULL);

	wifiConnect();

	temperature_sensor_handle_t temp_sensor = NULL;
//	temperature_sensor_config_t temp_sensor_config = TEMPERATURE_SENSOR_CONFIG_DEFAULT(50, 125);
	temperature_sensor_config_t temp_sensor_config = TEMPERATURE_SENSOR_CONFIG_DEFAULT(20, 100);
	ESP_ERROR_CHECK(temperature_sensor_install(&temp_sensor_config, &temp_sensor));
    ESP_ERROR_CHECK(temperature_sensor_enable(temp_sensor));

	while (true) {
		vTaskDelay(10);
		keysTimerHandler_ms(10);
		if (presc-- == 0) {
			presc = 100;
			temperature_sensor_get_celsius(temp_sensor, &temperature);
/*			if( tempRangeLow) {
				if (temperature > 55){
					temp_sensor_config.range_max = 125;
					temp_sensor_config.range_min = 50;
				    ESP_ERROR_CHECK(temperature_sensor_disable(temp_sensor));
					temperature_sensor_uninstall(temp_sensor);
					ESP_ERROR_CHECK(temperature_sensor_install(&temp_sensor_config, &temp_sensor));
				    ESP_ERROR_CHECK(temperature_sensor_enable(temp_sensor));
				    tempRangeLow = false;
				}
			}
			else {
				if (temperature < 50){
					temp_sensor_config.range_max = 50;
					temp_sensor_config.range_min = 10;
				    ESP_ERROR_CHECK(temperature_sensor_disable(temp_sensor));
					temperature_sensor_uninstall(temp_sensor);
					ESP_ERROR_CHECK(temperature_sensor_install(&temp_sensor_config, &temp_sensor));
				    ESP_ERROR_CHECK(temperature_sensor_enable(temp_sensor));
				    tempRangeLow = true;
				}
			}*/
			//ESP_LOGI(TAG, "Temperature value %.02f ℃", temperature);
		}
		if (!ipShown) {
			if (!smartConfigShown) {
				if (connectStatus == SMARTCONFIG_ACTIVE) {
					if (xSemaphoreTakeRecursive(LCDsemphr, (TickType_t) 1000) == pdTRUE) {
						sprintf(LCDline, "Geen Wifi,");
						LCDprintLine(1, LCDline);
						sprintf(LCDline, "wacht op");
						LCDprintLine(2, LCDline);
						sprintf(LCDline, "EspTouch...");
						LCDprintLine(3, LCDline);
						sprintf(LCDline, " ");
						LCDprintLine(4, LCDline);
					}
					vTaskDelay(1000);
					xSemaphoreGiveRecursive(LCDsemphr);
					smartConfigShown = true;
				}
			}
			if (connectStatus == IP_RECEIVED) {
				if (xSemaphoreTakeRecursive(LCDsemphr, (TickType_t) 1000) == pdTRUE) {
					sprintf(LCDline, "Verbonden met,");
					LCDprintLine(1, LCDline);
					sprintf(LCDline, "%s", wifiSettings.SSID);
					LCDprintLine(2, LCDline);
					sprintf(LCDline, "IP adres:");
					LCDprintLine(3, LCDline);
					sprintf(LCDline, "%s", myIpAddress);
					LCDprintLine(4, LCDline);
				}
				vTaskDelay(2000);
				xSemaphoreGiveRecursive(LCDsemphr);
				ipShown = true;
			}
		}
	}
}
