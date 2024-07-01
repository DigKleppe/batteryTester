#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include "driver/gpio.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "hd44780.h"
#include "tester.h"

void currentRegulatorTask(void *pvParameter);
static const char *TAG = "main";
void initLCD();



extern "C" void app_main(void)
{
	printf("Hello from app_main!\n");
	vTaskDelay(100);


//	initLCD();

//	while(1) {
//		 vTaskDelay(1000);
//	//    lcd.write(0, 0, (unsigned char*)"Hello World!");
//	    vTaskDelay(100);
//	}

	xTaskCreate(&currentRegulatorTask, "crTask", 1024*2 , NULL, configMAX_PRIORITIES, NULL);
	xTaskCreate(&testTask, "testTask", 1024*4 , NULL, 0, NULL);
	while (true) {
      //  printf("Hello from app_main!\n");
		vTaskDelay(25);
     }
}
