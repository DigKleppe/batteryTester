/*
 * testTask.cpp
 *
 *  Created on: Jun 28, 2024
 *      Author: dig
 */

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "lcd.h"
#include "log.h"
#include "measureTask.h"
#include "settings.h"
#include "mdns.h"
#include "averager.h"
#include "timeLog.h"
#include <stdio.h>
#include <string.h>

extern int scriptState;
static const char *TAG = "testTask";
static const int FULLBATDEBOUNCES = 20;
static const int NOBATDEBOUNCES = 3;
static const int MEASINTERVAL = 300; // every 60 seconds measure voltage without current
static const int MEASTIME = 2;
static const int PULSETIME = 30; // seconds puls time with MAXCURRENT to try to awake deep charged cell

extern float temperature;

Averager avgm1(MEASTIME);
Averager avgm2(MEASTIME);
Averager avgm3(MEASTIME);
Averager avgm4(MEASTIME);

Averager *avgm[] = { &avgm1, &avgm2, &avgm3, &avgm4 };

TimeLog tl1(3600 / MEASINTERVAL);
TimeLog tl2(3600 / MEASINTERVAL);
TimeLog tl3(3600 / MEASINTERVAL);
TimeLog tl4(3600 / MEASINTERVAL);

TimeLog *tLog[] = { &tl1, &tl2, &tl3, &tl4 };

function_t function = FUNCTION_TESTING;

static const char *statusText[] = { { "Geen batterij" }, { "Instellen" }, { "Ontladen1" }, { "-" }, { "Laden" }, { "Meten" }, { "Meten" }, { "-" }, { "Ontladen2" }, { "-" }, {
		"Getest vol" }, { "Ontladen" }, {"Diep ontladen"},{"Diep ontladen 2"}, { "Testmode" }, { "Calibratie" }, { "Fout" } };

static const char *functionText[] = { { "Testen" }, { "Laden" }, { "Ontladen" } };

testChannel_t testChannel[NR_CHANNELS];

extern bool currentRegulatorStarted;

bool noBat(int idx) {
	if ((testChannel[idx].voltage > NOBATVOLTAGE) || (testChannel[idx].voltage < NOBATVOLTAGEDECHARGING)) {
		if (testChannel[idx].noBatDebounces >= NOBATDEBOUNCES) {
			return true;
		}
		testChannel[idx].noBatDebounces++;
	} else
		testChannel[idx].noBatDebounces = 0;

	return false;
}

bool isFull(int idx) {
//	float diff;
	if (testChannel[idx].voltage > MAXCHARGEDVOLATGE) // or bad contact
	{
		if (testChannel[idx].fullDebounces >= FULLBATDEBOUNCES) {
			ESP_LOGE(TAG, "%d full voltage max voltage exeeded", idx + 1);
			testChannel[idx].fullDebounces = 0;
		} else
			testChannel[idx].fullDebounces++;
	} else
		testChannel[idx].fullDebounces = 0;

	//	diff = testChannel[idx].maxVoltage - testChannel[idx].voltage;
//	if (diff > CHARGEDDROP) {
//		ESP_LOGE(TAG, "%d full voltage dropped %f ", idx + 1, diff);
//		return true;
//	}
	if (testChannel[idx].maxVoltage < testChannel[idx].voltage)
		testChannel[idx].maxVoltage = testChannel[idx].voltage;
//	ESP_LOGI(TAG, "%d Vbat: %4.3f Vmax: %4.3f V", idx + 1, testChannel[idx].voltage, testChannel[idx].maxVoltage);
	return false;
}

void testTask(void *pvParameter) {
	TickType_t xLastWakeTime;
	char LCDline[400];
	int len;
	bool toggle= false;

	int measTimer[NR_CHANNELS];
	log_t log;
	int logTimer = 0;

	while (!currentRegulatorStarted)
		vTaskDelay(100);

	for (int n = 0; n < NR_CHANNELS; n++) {
		if (testChannel[n].status == STATUS_INA_ERROR) {
			ESP_LOGE(TAG, "ina %d error", n+1);
		} else {
			ESP_LOGI(TAG, "ina %d initialized", n+1);
		}
		testChannel[n].noBatDebounces = NOBATDEBOUNCES;
	}
	xLastWakeTime = xTaskGetTickCount();
	while (1) {
		memset(LCDline, 0, LCD_COLS + 1); // clr buffer
	//	printf("%f\n", testChannel[0].voltage);
		for (int n = 0; n < NR_CHANNELS; n++) {
			len = sprintf(LCDline, "%d ", n + 1);
			switch (testChannel[n].status) {
			case STATUS_INA_ERROR:
				strcpy(LCDline + len, "* INA fout *");
				break;

			case STATUS_NO_BAT:
				//			ESP_LOGI(TAG, "%d no battery %4.3f V", n + 1, testChannel[n].voltage);
				if (testChannel[n].voltage < ERRORVOLTAGE) {
					sprintf(LCDline + len, "*Fout*  %4.3f V", testChannel[n].voltage);
					testChannel[n].setCurrent = MAXCURRENT; // try to awake deep decharrged battery
					testChannel[n].status = STATUS_LOW_VOLTAGE1;
					testChannel[n].retries = 3; // try 3 times to awake
					measTimer[n] = PULSETIME;
				} else {
					if (noBat(n))
						strcpy(LCDline + len, "Geen batterij");
					else {
						strcpy(LCDline + len, "");
						testChannel[n].measuredCapacity = 0;
						testChannel[n].chargedCapacity = 0;
						testChannel[n].dechargedCapacity = 0;
						testChannel[n].maxVoltage = 0;
						testChannel[n].setCurrent = 0;
						testChannel[n].status = STATUS_SETUP;
						testChannel[n].isTested = false;
						tLog[n]->clear();
						measTimer[n] = 1;
					}
				}
				log.voltage[n] = 2.0; // limit for chart
				break;

			case STATUS_SETUP:
				log.voltage[n] = testChannel[n].voltage;
				// handled in main
				break;

			case STATUS_DECHARGING1:
				if (function == FUNCTION_CHARGING) {
					testChannel[n].status = STATUS_SETUP;
					testChannel[n].setCurrent = 0;
				} else {
					//		ESP_LOGI(TAG, "%d decharging %d mA %4.3f V", n + 1, -testChannel[n].averagedCurrent, testChannel[n].voltage);
					if (noBat(n)) {
						testChannel[n].status = STATUS_NO_BAT;
						testChannel[n].setCurrent = 0;
						vTaskDelay(2000);
					} else {
						log.voltage[n] = testChannel[n].voltage;
						sprintf(LCDline + len, "O %3dmA %4.3fV", -testChannel[n].averagedCurrent, testChannel[n].voltage);
						testChannel[n].dechargedCapacity += -testChannel[n].averagedCurrent; // in mAs
						if ((testChannel[n].voltage < DECHARDEDVOLATAGE) && (testChannel[n].voltage > NOBATVOLTAGEDECHARGING)) {
							//		testChannel[n].measuredCapacity = testChannel[n].outCharge / 3600; // to mAh
							testChannel[n].setCurrent = 0;

							if (function == FUNCTION_DECHARGING) {
								testChannel[n].status = STATUS_DECHARGED;
							} else
								testChannel[n].status = STATUS_WAIT0;
						}
					}
				}
				break;

			case STATUS_WAIT0:
				if (testChannel[n].averagedCurrent < NOCURRENT) {
					testChannel[n].status = STATUS_CHARGING;
					testChannel[n].chargeTimer = MAXCHARGETIME * 3600;
				}
				break;

			case STATUS_CHARGING:
				//	ESP_LOGI(TAG, "%d charging %3d mA %4.3f %4.3f V", n + 1, testChannel[n].averagedCurrent, testChannel[n].voltage,testChannel[n].maxVoltage );
				if (function == FUNCTION_DECHARGING) {
					testChannel[n].status = STATUS_WAIT1;
					testChannel[n].setCurrent = 0;
				} else {
					if (testChannel[n].isTested) // recharging
						sprintf(LCDline + len, "T*%4dmAh* %4.3fV", testChannel[n].measuredCapacity, testChannel[n].voltage);
					else
						sprintf(LCDline + len, "L %3dmA %4.3fV", testChannel[n].averagedCurrent, testChannel[n].voltage);

					testChannel[n].chargedCapacity += testChannel[n].averagedCurrent; // in mAs
					if (noBat(n))
						testChannel[n].status = STATUS_NO_BAT;
					else {
						if (measTimer[n]-- == 0) {
							testChannel[n].setCurrent = 0; // measure voltage without current
							measTimer[n] = 1;
							testChannel[n].status = STATUS_CHARGING_MEAS1;
						}
						if (testChannel[n].chargeTimer > 0)
							testChannel[n].chargeTimer--;
						else {
					//		ESP_LOGI(TAG, "%d max chargingtime reached", n + 1);
							if (testChannel[n].isTested) { // recharging after test
								testChannel[n].status = STATUS_CHARGED;
								testChannel[n].setCurrent = 5; // trickle
							} else {
								testChannel[n].setCurrent = 0;
								testChannel[n].status = STATUS_WAIT1; // go on with test
							}
						}
					}
				}
				break;

			case STATUS_CHARGING_MEAS1:
				//	ESP_LOGI(TAG, "%d charging meas1  %3d mA %4.3f %4.3f V", n + 1, testChannel[n].averagedCurrent, testChannel[n].voltage,testChannel[n].maxVoltage );

				if (!testChannel[n].isTested) // recharging
					sprintf(LCDline + len, "M %3dmA %4.3fV", testChannel[n].averagedCurrent, testChannel[n].voltage);

				testChannel[n].chargedCapacity += testChannel[n].averagedCurrent; // in mAs

//				if ( testChannel[n].averagedCurrent < 5) {
				if (measTimer[n]-- == 0) { // wait until the current is zero
					avgm[n]->clear();
					measTimer[n] = MEASTIME;
					testChannel[n].status = STATUS_CHARGING_MEAS2;
				}
				break;

			case STATUS_CHARGING_MEAS2:
				//	ESP_LOGI(TAG, "%d charging meas 2%3d mA %4.3f %4.3f V", n + 1, testChannel[n].averagedCurrent, testChannel[n].voltage,testChannel[n].maxVoltage );
				if (!testChannel[n].isTested) // recharging
					sprintf(LCDline + len, "Meten.. %4.3fV", testChannel[n].voltage);
				avgm[n]->write(testChannel[n].voltage * 1000.0);
				if (measTimer[n]-- == 0) {
					measTimer[n] = MEASINTERVAL;
					testChannel[n].voltage = avgm[n]->average() / 1000.0;
					log.voltage[n] = testChannel[n].voltage; // log decharging is currentless
					tLog[n]->write(testChannel[n].voltage * 1000);
					//	ESP_LOGI(TAG, "%d Vmeas %4.3f", n + 1, testChannel[n].voltage);
					if (isFull(n)) {
						if (testChannel[n].isTested) { // recharging after test
							testChannel[n].status = STATUS_CHARGED;
							testChannel[n].setCurrent = 10; // trickle
						} else {
							testChannel[n].setCurrent = 0;
							testChannel[n].status = STATUS_WAIT1;
						}
					} else {
						testChannel[n].status = STATUS_CHARGING;
						testChannel[n].setCurrent = testChannel[n].chargeCurrent;
					}
				}
				break;

			case STATUS_WAIT1:
				sprintf(LCDline + len, "O %3dmA %4.3fV", testChannel[n].averagedCurrent, testChannel[n].voltage);
				//	ESP_LOGI(TAG, "%d wait1 %d mA %4.3f V", n + 1, testChannel[n].current, testChannel[n].voltage);
				if (testChannel[n].current < NOCURRENT) {
					testChannel[n].measuredCapacity = 0;
					if (function == FUNCTION_CHARGING) {
						testChannel[n].status = STATUS_CHARGED;
					} else {
						testChannel[n].setCurrent = -testChannel[n].deChargeCurrent;
						testChannel[n].status = STATUS_DECHARGING2;
						testChannel[n].dechargedCapacity = 0; // clear from first decharge step
					}
				}
				break;

			case STATUS_DECHARGING2:
				if (function == FUNCTION_CHARGING) {
					testChannel[n].status = STATUS_SETUP;
					testChannel[n].setCurrent = 0;
				} else {
					//		ESP_LOGI(TAG, "%d decharging %d mA %4.3f V", n + 1, -testChannel[n].averagedCurrent, testChannel[n].voltage);
					log.voltage[n] = testChannel[n].voltage;

					sprintf(LCDline + len, "O %3dmA %4.3fV", -testChannel[n].averagedCurrent, testChannel[n].voltage);
					testChannel[n].dechargedCapacity += -testChannel[n].averagedCurrent; // in mAs
					if (noBat(n))
						testChannel[n].status = STATUS_NO_BAT;
					else {
						if ((testChannel[n].voltage < DECHARDEDVOLATAGE) && (testChannel[n].voltage > NOBATVOLTAGEDECHARGING)) {
							testChannel[n].measuredCapacity = testChannel[n].dechargedCapacity / 3600; // to mAh
							testChannel[n].setCurrent = 0;
							if (function == FUNCTION_DECHARGING)
								testChannel[n].status = STATUS_DECHARGED;
							else
								testChannel[n].status = STATUS_WAIT2;
						}
					}
				}
				break;

			case STATUS_WAIT2:
				sprintf(LCDline + len, "O %3dmA %4.3fV", -testChannel[n].averagedCurrent, testChannel[n].voltage);
				//		ESP_LOGI(TAG, "%d wait2 %d mA %4.3f V", n + 1, testChannel[n].current, testChannel[n].voltage);
				if (testChannel[n].current < NOCURRENT) {
					testChannel[n].setCurrent = testChannel[n].chargeCurrent;
					testChannel[n].chargedCapacity = 0;
					testChannel[n].dechargedCapacity = 0;
					testChannel[n].isTested = true;
					//	testChannel[n].status = STATUS_TESTED;
					testChannel[n].status = STATUS_CHARGING; // recharge
					testChannel[n].chargeTimer = MAXCHARGETIME * 3600;
					testChannel[n].maxVoltage = 0;
					testChannel[n].noBatDebounces = 0;
				}
				break;

			case STATUS_CHARGED:
				//		ESP_LOGI(TAG, "%d ready %4d mA %4.3f V", n + 1, testChannel[n].averagedCurrent, testChannel[n].voltage);
				if (function == FUNCTION_DECHARGING)
					testChannel[n].status = STATUS_WAIT1;
				else {
					log.voltage[n] = testChannel[n].voltage;
					sprintf(LCDline + len, "*%4dmAh * Vol", testChannel[n].measuredCapacity);

					if (noBat(n)) {
						testChannel[n].status = STATUS_NO_BAT;
					}
				}
				break;

			case STATUS_DECHARGED:
				if (function == FUNCTION_CHARGING)
					testChannel[n].status = STATUS_SETUP;
				else {
					log.voltage[n] = testChannel[n].voltage;
					sprintf(LCDline + len, "%1.2fV Ontladen", testChannel[n].voltage);
			//		ESP_LOGI(TAG, "%d max chargingtime reached", n + 1);
					if (noBat(n))
						testChannel[n].status = STATUS_NO_BAT;
				}
				break;

			case STATUS_LOW_VOLTAGE1:
				log.voltage[n] = testChannel[n].voltage;
				sprintf(LCDline + len, "Puls %d  %4.3fV", testChannel[n].retries, testChannel[n].voltage);
				if (testChannel[n].voltage > NOBATVOLTAGE)
					testChannel[n].status = STATUS_NO_BAT;

				if (measTimer[n]-- == 0 ){
					testChannel[n].setCurrent =0;
					testChannel[n].status = STATUS_LOW_VOLTAGE2;
				}
				break;

			case STATUS_LOW_VOLTAGE2:
				log.voltage[n] = testChannel[n].voltage;
				sprintf(LCDline + len, "Puls %d  %4.3fV",testChannel[n].retries, testChannel[n].voltage);
				if (testChannel[n].voltage < ERRORVOLTAGE) {
					if ( testChannel[n].retries == 0)
						sprintf(LCDline + len, "* Fout * %4.3fV ", testChannel[n].voltage);
					else {
						 testChannel[n].retries--;
						 testChannel[n].setCurrent = MAXCURRENT;
						 testChannel[n].status = STATUS_LOW_VOLTAGE1;
						 measTimer[n] = PULSETIME;
					}
				}
				else
					 testChannel[n].status = STATUS_NO_BAT;
				break;

			case STATUS_TESTMODE:
				log.voltage[n] = testChannel[n].voltage;
				sprintf(LCDline + len, "Tm %3dmA %4.3fV", testChannel[n].averagedCurrent, testChannel[n].voltage);
				toggle = !toggle;
				if (toggle)
					testChannel[n].setCurrent = 100;
				else
					testChannel[n].setCurrent = 0;
				break;

			case STATUS_CALIBRATION:
				log.voltage[n] = testChannel[n].voltage;
				sprintf(LCDline + len, "Cal %3dmA %4.3fV", testChannel[n].averagedCurrent, testChannel[n].voltage);
				break;
			}
			if (xSemaphoreTakeRecursive(LCDsemphr, (TickType_t) 0) == pdTRUE) {
				LCDprintLine( n+1 , LCDline);
				xSemaphoreGiveRecursive(LCDsemphr);
			}
		}
		if (logTimer <= 0) {
			addToLog(log);
			logTimer = LOGINTERVAL;
		} else {
			logTimer--;
		}
		xTaskDelayUntil(&xLastWakeTime, 1000);
	}
}

// called from CGI

int getFunctionScript(char *pBuffer, int count) {
	int len = 0;
	switch (scriptState) {
	case 0:
		scriptState++;
		len += sprintf(pBuffer + len, "%s", functionText[function]);
		break;
	default:
		break;
	}
	return (len);
}

int getSensorNameScript(char *pBuffer, int count) {
	int len = 0;
	switch (scriptState) {
	case 0:
		scriptState++;
		len += sprintf(pBuffer + len, "Actueel,Nieuw\n");
		len += sprintf(pBuffer + len, "%s\n", userSettings.moduleName);
		break;
	default:
		break;
	}
	return (len);
}

int getInfoValuesScript(char *pBuffer, int count) {
	int len = 0;
	char str[10];
	switch (scriptState) {
	case 0:
		scriptState++;
		len += sprintf(pBuffer + len, "%s\n", "Positie,Actueel,Offset,Gain");
		for (int n = 0; n < NR_CHANNELS; n++) {
			sprintf(str, "%d", n + 1);
			len += sprintf(pBuffer + len, "%s spanning,%3.3f, -, -\n", str, testChannel[n].voltage); // no gain and offset needed for voltage
			len += sprintf(pBuffer + len, "%s stroom ,%3.2f,%3.2f,%3.4f\n", str, testChannel[n].current - userSettings.currentOffset[n], userSettings.currentOffset[n],
					userSettings.currentGain[n]);
		}
		break;
	default:
		break;
	}
	return (len);
}

// only build javascript table

int getCalValuesScript(char *pBuffer, int count) {
	int len = 0;
	switch (scriptState) {
	case 0:
		scriptState++;
		len += sprintf(pBuffer + len, "%s", "Positie,Referentie,Stel in,Herstel\n");
		len += sprintf(pBuffer + len, "%s", "Positie 1\n Positie 2\n Positie 3\n Positie 4\n");
		break;
	default:
		break;
	}
	return (len);
}

int saveSettingsScript(char *pBuffer, int count) {
	saveSettings();
	return (0);
}

int cancelSettingsScript(char *pBuffer, int count) {
	loadSettings();
	return (0);
}

int getRTMeasValuesScript(char *pBuffer, int count) {
	int len = 0;

	switch (scriptState) {
	case 0:
		scriptState++;
		len = sprintf(pBuffer + len, "%d,", static_cast<int>(timeStamp++));
		for (int n = 0; n < NR_CHANNELS; n++) {
			if (testChannel[n].voltage > NOBATVOLTAGE)
				len += sprintf(pBuffer + len, "2.0");	// limit for chart
			else
				len += sprintf(pBuffer + len, "%4.4f,", testChannel[n].voltage);
		}
		len += sprintf(pBuffer + len, "\n");

		break;
	default:
		break;
	}
	return (len);
}

int getChargeTableScript(char *pBuffer, int count) {
	int len = 0;
	switch (scriptState) {
	case 0:
		scriptState++;
		len += sprintf(pBuffer + len, "%s", "Meting,Positie 1,Positie 2,Positie 3,Positie 4\n");  // horizontal
		len += sprintf(pBuffer + len, "%s", "Status,Gemeten mAh:,Spanning (V):,Geladen mAh:,Ontladen mAh:,Temperatuur\n"); // vertical
		break;
	default:
		break;
	}
	return (len);
}
int getChargeValuesScript(char *pBuffer, int count) {
	int len = 0;

	switch (scriptState) {
	case 0:
		scriptState++;
		for (int n = 0; n < NR_CHANNELS; n++) {
			len += sprintf(pBuffer + len, "%s,", statusText[static_cast<int>(testChannel[n].status)]);
			len += sprintf(pBuffer + len, "%4d,", testChannel[n].measuredCapacity);
			len += sprintf(pBuffer + len, "%3.3f,", testChannel[n].voltage);
			len += sprintf(pBuffer + len, "%d,", testChannel[n].chargedCapacity / 3600);
			len += sprintf(pBuffer + len, "%d,", testChannel[n].dechargedCapacity / 3600);
			len += sprintf(pBuffer + len, "%.02f ℃\n", temperature);
			//	len += sprintf(pBuffer + len, "%3.3f,", static_cast<float>(tLog[n]->getValue(1)) / 1000.0); //  5 min ago
			//	len += sprintf(pBuffer + len, "%3.3f\n", static_cast<float>(tLog[n]->getValue(12)) / 1000.0); //  1 hour ago
		}
		break;
	case 2:
		scriptState++;
		break;

	default:
		break;
	}

	if (len > 0)
		len += sprintf(pBuffer + len, ";");
	return (len);
}

// these functions only work for one user!

int getNewMeasValuesScript(char *pBuffer, int count) {
	int left = 0;
	int len = 0;
	if (dayLogRxIdx != (dayLogTxIdx)) {  // something to send?
		do {
			len += sprintf(pBuffer + len, "%d,", static_cast<int>(dayLog[dayLogRxIdx].timeStamp));
			for (int n = 0; n < NR_CHANNELS; n++) {
				len += sprintf(pBuffer + len, "%1.4f,", dayLog[dayLogRxIdx].voltage[n]);
			}
			len += sprintf(pBuffer + len, "\n");
			dayLogRxIdx++;
			if (dayLogRxIdx > MAXDAYLOGVALUES)
				dayLogRxIdx = 0;
			left = count - len;

		} while ((dayLogRxIdx != dayLogTxIdx) && (left > 40));
	}
	return (len);
}

calValues_t calValues;
// @formatter:off
char tempName[MAX_STRLEN];
int calCurrent;
int stopCal;
int dummy;

const CGIdesc_t writeVarDescriptors[] = {
    { "Positie 1", &calValues.current[0], FLT, 1 },
	{ "Positie 2", &calValues.current[1], FLT, 1 },
	{ "Positie 3", &calValues.current[2], FLT, 1 },
	{ "Positie 4", &calValues.current[3], FLT, 1 },
	{ "moduleName",tempName, STR, 1 },
	{ "setCurrent", &calCurrent, INT, 1 },
	{ "stopCal", &stopCal, INT, 1 },
	{ "clearLog", &dummy, INT, 1 },
	{ "setFunction", tempName, STR, 1 },
};

#define NR_WRITEVARDESCRIPTORS (sizeof (writeVarDescriptors)/ sizeof (CGIdesc_t))

// @formatter:on

// parse items send by senditem (HTML post)

void parseCGIWriteData(char *buf, int received) {
	int idx = -1;
	if (strncmp(buf, "setCal:", 7) == 0) {  //
		idx = readActionScript(&buf[7], writeVarDescriptors, NR_WRITEVARDESCRIPTORS);
		if (idx >= 0) {
			if (testChannel[idx].averagedCurrent != 0) {
				float measuredCurrent = static_cast<float>(testChannel[idx].averagedCurrent) / userSettings.currentGain[idx];
				userSettings.currentGain[idx] = calValues.current[idx] / measuredCurrent;
				saveSettings();
			}
		}
	}
	if (idx < 0) {
		if (strncmp(buf, "setName:", 8) == 0) {
			idx = readActionScript(&buf[8], writeVarDescriptors, NR_WRITEVARDESCRIPTORS);
			if (idx >= 0) {
				if (strcmp(tempName, userSettings.moduleName) != 0) {
					strcpy(userSettings.moduleName, tempName);
					ESP_ERROR_CHECK(mdns_hostname_set(userSettings.moduleName));
					ESP_LOGI(TAG, "Hostname set to %s", userSettings.moduleName);
					saveSettings();
				}
			}
		}
	}

	if (idx < 0) { // none of above
		idx = readActionScript(buf, writeVarDescriptors, NR_WRITEVARDESCRIPTORS);
		switch (idx) {
		case 5: // setCurrent -> switch to calibration mode
			for (int n = 0; n < NR_CHANNELS; n++) {
				testChannel[n].status = STATUS_CALIBRATION;
				testChannel[n].setCurrent = calCurrent;
			}
			break;
		case 6:
			for (int n = 0; n < NR_CHANNELS; n++) {
				testChannel[n].status = STATUS_NO_BAT;
				testChannel[n].setCurrent = 0;
			}
			break;
		case 7: // clearLog
			clearLog();
			break;
		case 8: // setFunction
			for (int n = 0; n < NR_FUNCTIONS; n++) {
				if (strcmp(functionText[n], tempName) == 0) {
					function = (function_t) n;
					saveSettings();
				}
			}
			break;
		default:
			break;

		}
	}
}

