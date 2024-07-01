#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include "driver/gpio.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lcd.h"
#include "tester.h"

void currentRegulatorTask(void *pvParameter);
static const char *TAG = "main";
bool initLCD();



extern "C" void app_main(void)
{
	vTaskDelay(100);
	initLCD();

	xTaskCreate(&currentRegulatorTask, "crTask", 1024*2 , NULL, configMAX_PRIORITIES, NULL);
	xTaskCreate(&testTask, "testTask", 1024*4 , NULL, 0, NULL);
	
	while (true) {
		     //  printf("Hello from app_main!\n");
		vTaskDelay(25);
     }
}
