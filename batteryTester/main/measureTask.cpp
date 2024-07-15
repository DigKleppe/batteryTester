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
#include "log.h"
#include "measureTask.h"
#include <Arduino.h>
#include "hd44780.h"
#include "hd44780ioClass/hd44780_NTCU20025ECPB_pinIO.h" 
#include "settings.h"
#include <stdio.h>

extern int scriptState;
static const char *TAG = "testTask";

testChannel_t testChannel[NR_CHANNELS];
extern bool currentRegulatorStarted;

bool isFull(int idx)
{
	float diff;

	if (testChannel[idx].voltage > MAXCHARGEDVOLATGE)
	{
		ESP_LOGE(TAG, "full voltage max voltage exeeded ");
		return true;
	}
	diff = testChannel[idx].maxVoltage - testChannel[idx].voltage;
	if (diff > CHARGEDDROP)
	{
		ESP_LOGE(TAG, "full voltage dropped %f ", diff);
		return true;
	}
	if (testChannel[idx].maxVoltage < testChannel[idx].voltage)
		testChannel[idx].maxVoltage = testChannel[idx].voltage;
	return false;
}

void testTask(void *pvParameter)
{
	TickType_t xLastWakeTime;
	//  int insertTimer = 2;
	char LCDline[400];
	int len;
	bool toggle;

	while (!currentRegulatorStarted)
		vTaskDelay(100);

	for (int n = 0; n < NR_CHANNELS; n++)
	{
		if (testChannel[n].status == STATUS_INA_ERROR)
		{
			ESP_LOGE(TAG, "ina %d+1 error", n);
		}
		else
		{
			ESP_LOGI(TAG, "ina %d+1 initialized", n);
		}
		testChannel[n].chargeCurrent = 400;
		testChannel[n].deChargeCurrent = 400;
	}
	xLastWakeTime = xTaskGetTickCount();
	while (1)
	{
		memset(LCDline, 0, LCD_COLS + 1); // clr buffer
		for (int n = 0; n < NR_CHANNELS; n++)
		{
			len = sprintf(LCDline, "%d ", n + 1);
			switch (testChannel[n].status)
			{
			case STATUS_INA_ERROR:
				strcpy(LCDline + len, "* INA fout *");
				break;

			case STATUS_NO_BAT:
				ESP_LOGI(TAG, "%d no battery %4.3f V", n + 1, testChannel[n].voltage);
				if (testChannel[n].voltage < ERRORVOLTAGE)
					sprintf(LCDline + len, "* Fout * %3.2f V", testChannel[n].voltage);
				else
				{
					if (testChannel[n].voltage < NOBATVOLTAGE)
					{
						strcpy(LCDline + len, "");
						testChannel[n].measuredCapacity = 0;
						testChannel[n].measuredPower = 0;
						testChannel[n].maxVoltage = 0;
						testChannel[n].setCurrent = 0;
						testChannel[n].status = STATUS_SETUP;
					}
					else
					{
						strcpy(LCDline + len, "Geen batterij");
					}
				}
				break;

			case STATUS_SETUP: // handled in main
				break;

			case STATUS_CHARGING:
				ESP_LOGI(TAG, "%d charging %3d mA %4.3f V", n + 1, testChannel[n].averagedCurrent, testChannel[n].voltage);
				sprintf(LCDline + len, "L %3dmA %4.3fV", testChannel[n].averagedCurrent, testChannel[n].voltage);
				if (isFull(n))
				{
					testChannel[n].inPower = testChannel[n].measuredPower;
					testChannel[n].setCurrent = 0;
					testChannel[n].status = STATUS_WAIT1;
				}
				break;

			case STATUS_WAIT1:
				sprintf(LCDline + len, "O %3dmA %4.3fV", testChannel[n].averagedCurrent, testChannel[n].voltage);
				ESP_LOGI(TAG, "%d wait1 %d mA %4.3f V", n + 1, testChannel[n].current, testChannel[n].voltage);

				if (testChannel[n].current < NOCURRENT)
				{
					testChannel[n].measuredCapacity = 0;
					testChannel[n].samples = 0;
					testChannel[n].measuredPower = 0;
					testChannel[n].setCurrent = -testChannel[n].deChargeCurrent;
					testChannel[n].status = STATUS_DECHARGING;
				}
				break;

			case STATUS_DECHARGING:
				ESP_LOGI(TAG, "%d decharging %d mA %4.3f V", n + 1, -testChannel[n].averagedCurrent, testChannel[n].voltage);
				sprintf(LCDline + len, "O %3dmA %4.3fV", -testChannel[n].averagedCurrent, testChannel[n].voltage);

				testChannel[n].measuredCapacity -= testChannel[n].averagedCurrent;
				testChannel[n].samples++;

				if ((testChannel[n].voltage < DECHARDEDVOLATAGE) || (testChannel[n].voltage >
				NOBATVOLTAGE)) // removed
				// testChannel[n].outPower =
				// testChannel[n].measuredPower;
				{
					//          testChannel[n].measuredPower = 0;
					testChannel[n].measuredCapacity = testChannel[n].measuredCapacity / 3600; // to mAh
					testChannel[n].setCurrent = 0;
					testChannel[n].status = STATUS_WAIT2;
				}
				break;

			case STATUS_WAIT2:
				sprintf(LCDline + len, "O %3dmA %4.3fV", -testChannel[n].averagedCurrent, testChannel[n].voltage);
				ESP_LOGI(TAG, "%d wait2 %d mA %4.3f V", n + 1, testChannel[n].current, testChannel[n].voltage);
				if (testChannel[n].current < NOCURRENT)
				{
					testChannel[n].setCurrent = testChannel[n].chargeCurrent;
					testChannel[n].status = STATUS_TESTED;
					testChannel[n].maxVoltage = 0;
				}
				break;

			case STATUS_TESTED: // recharge after testing
				ESP_LOGI(TAG, "%d tested charging %d mA %4.3f V %d mAH", n + 1, testChannel[n].averagedCurrent, testChannel[n].voltage, testChannel[n].measuredCapacity);

				sprintf(LCDline + len, "T*%4dmAh* %4.3fV", testChannel[n].measuredCapacity, testChannel[n].voltage);

				if (isFull(n))
				{
					testChannel[n].status = STATUS_CHARGED;
					testChannel[n].setCurrent = 10; // trickle
				}
				break;

			case STATUS_CHARGED:
				ESP_LOGI(TAG, "%d ready %4d mA %4.3f V", n + 1, testChannel[n].averagedCurrent, testChannel[n].voltage);

				sprintf(LCDline + len, "T *%4dmAh* Vol", testChannel[n].measuredCapacity);

				if (testChannel[n].voltage > NOBATVOLTAGE)
				{
					testChannel[n].status = STATUS_NO_BAT;
				}
				break;

			case STATUS_TESTMODE:
				sprintf(LCDline + len, "test %4dmA %4.3fV", testChannel[n].averagedCurrent, testChannel[n].voltage);
				toggle = !toggle;
				if (toggle)
					testChannel[n].setCurrent = 100;
				else
					testChannel[n].setCurrent = 0;
				break;
			}

			if (xSemaphoreTakeRecursive(LCDsemphr, (TickType_t)0) == pdTRUE)
			{
				LCDline[LCD_COLS] = 0; // limit to actual value
				lcd.setCursor(0, n);

				for (int x = strlen(LCDline); x < LCD_COLS; x++)
					LCDline[x] = ' '; // fill with spaces

				lcd.print(LCDline);
				xSemaphoreGiveRecursive(LCDsemphr);
			}
		}
		xTaskDelayUntil(&xLastWakeTime, 1000);
	}
}

// called from CGI

int getSensorNameScript(char *pBuffer, int count)
{
	int len = 0;
	switch (scriptState)
	{
	case 0:
		scriptState++;
		len += sprintf(pBuffer + len, "Actueel,Nieuw\n");
		len += sprintf(pBuffer + len, "%s\n", userSettings.moduleName);
		return len;
		break;
	default:
		break;
	}
	return 0;
}

int getInfoValuesScript(char *pBuffer, int count)
{
	int len = 0;
	char str[10];
	switch (scriptState)
	{
	case 0:
		scriptState++;
		len += sprintf(pBuffer + len, "%s\n", "Meting,Actueel,Offset,Gain");
		for (int n = 0; n < NR_CHANNELS; n++)
		{
			sprintf(str, "Positie %d", n + 1);
			len += sprintf(pBuffer + len, "%s,%3.2f, 0, 1.0\n", str, testChannel[n].voltage); // no gain and offset needed for voltage
			len += sprintf(pBuffer + len, "%s,%3.2f,%3.2f,%3.2f\n", str, testChannel[n].current - userSettings.currentOffset[n], userSettings.currentOffset[n],
					userSettings.currentGain[n]);
		}
		return len;
		break;
	default:
		break;
	}
	return 0;
}

// only build javascript table

int getCalValuesScript(char *pBuffer, int count)
{
	int len = 0;
	switch (scriptState)
	{
	case 0:
		scriptState++;
		len += sprintf(pBuffer + len, "%s\n", "Meting,Referentie,Stel in,Herstel");
		len += sprintf(pBuffer + len, "%s\n", "Positie 1\n Positie 2\n Positie 3\n Positie 4\n");
		return len;
		break;
	default:
		break;
	}
	return 0;
}

int saveSettingsScript(char *pBuffer, int count)
{
	saveSettings();
	return 0;
}

int cancelSettingsScript(char *pBuffer, int count)
{
	loadSettings();
	return 0;
}

calValues_t calValues;
// @formatter:off
char tempName[MAX_STRLEN];

const CGIdesc_t writeVarDescriptors[] = {
    { "Stroom", &calValues.current, FLT, NR_CHANNELS },
    { "moduleName",tempName, STR, 1 }
};

#define NR_CALDESCRIPTORS (sizeof (writeVarDescriptors)/ sizeof (CGIdesc_t))
// @formatter:on

int getRTMeasValuesScript(char *pBuffer, int count)
{
	int len = 0;

	switch (scriptState)
	{
	case 0:
		scriptState++;

		len = sprintf(pBuffer + len, "%d,", (int) timeStamp++);
		for (int n = 0; n < NR_CHANNELS; n++)
		{
			len += sprintf(pBuffer + len, "%3.2f,", testChannel[n].voltage);
		}
		return len;
		break;
	default:
		break;
	}
	return 0;
}

// these functions only work for one user!

int getNewMeasValuesScript(char *pBuffer, int count)
{

	int left, len = 0;
	if (dayLogRxIdx != (dayLogTxIdx))
	{  // something to send?
		do
		{
			len += sprintf(pBuffer + len, "%d,", (int) dayLog[dayLogRxIdx].timeStamp);
			for (int n = 0; n < NR_CHANNELS; n++)
			{
				len += sprintf(pBuffer + len, "%3.2f,", dayLog[dayLogRxIdx].voltage[n]);
			}
			len += sprintf(pBuffer + len, "\n");
			dayLogRxIdx++;
			if (dayLogRxIdx > MAXDAYLOGVALUES)
				dayLogRxIdx = 0;
			left = count - len;

		} while ((dayLogRxIdx != dayLogTxIdx) && (left > 40));

	}
	return len;
}

void parseCGIWriteData(char *buf, int received)
{
	/*	if (strncmp(buf, "setCal:", 7) == 0) {  //

	 float ref = (refSensorAverager.average() / 1000.0);
	 for ( int n = 0; n < NR_CHANNELS; n++){
	 if (lastTemperature[n] != ERRORTEMP ){
	 float t =  ntcAverager[n].average() / 1000.0;
	 userSettings.temperatureOffset[n] = t - ref;
	 }
	 }
	 } else {
	 if (strncmp(buf, "setName:", 8) == 0) {
	 if (readActionScript(&buf[8], writeVarDescriptors, NR_CALDESCRIPTORS)) {
	 if (strcmp(tempName, userSettings.moduleName) != 0) {
	 strcpy(userSettings.moduleName, tempName);
	 ESP_ERROR_CHECK(mdns_hostname_set(userSettings.moduleName));
	 ESP_LOGI(TAG, "Hostname set to %s", userSettings.moduleName);
	 saveSettings();
	 }
	 }
	 }
	 }*/
}

