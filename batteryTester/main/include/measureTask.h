/*
 * tester.h
 *
 *  Created on: Jun 28, 2024
 *      Author: dig
 */

#ifndef MAIN_INCLUDE_MEASURETASK_H_
#define MAIN_INCLUDE_MEASURETASK_H_

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"


#define NR_CHANNELS 4

typedef enum {
  STATUS_NO_BAT,
  STATUS_SETUP,
  STATUS_CHARGING,
  STATUS_CHARGING_MEAS1,
  STATUS_CHARGING_MEAS2,
  STATUS_WAIT1,
  STATUS_DECHARGING,
 // STATUS_TESTED,
  STATUS_WAIT2,
  STATUS_CHARGED,
  STATUS_DECHARGED,
  STATUS_INA_ERROR,
  STATUS_TESTMODE,
  STATUS_CALIBRATION
} testChannelStatus;


typedef enum {
	FUNCTION_TESTING,
	FUNCTION_CHARGING,
	FUNCTION_DECHARGING,
	NR_FUNCTIONS
} function_t;

extern function_t function;

typedef struct {
  testChannelStatus status;
  int setCurrent;      // mA for currentSource
  int chargeCurrent;   // mA
  int deChargeCurrent; // mA
  int current;
  int averagedCurrent;  // mA
  int measuredCapacity; // mAH

  int inCharge;		// mAs
  int outCharge;    // mAs

  int samples;
  float voltage;
  float maxVoltage;
  int fullDebounces;
  int noBatDebounces;
  int chargeTimer;
  bool isTested;
} testChannel_t;

#define MAXCHARGEDVOLATGE 1.7
#define DECHARDEDVOLATAGE 0.9
#define NOBATVOLTAGE 4.0
#define ERRORVOLTAGE 0.6
#define NOCURRENT 2       // mA
#define CHARGEDDROP 0.008 // V full charged

extern testChannel_t testChannel[NR_CHANNELS];
extern bool setupNeeded;
extern QueueHandle_t LCDsemphr;
extern function_t function = FUNCTION_TESTING;

void testTask(void *pvParameter);
void currentRegulatorTask(void *pvParameter);


typedef struct {
	float voltage[NR_CHANNELS];
	float current[NR_CHANNELS];
} calValues_t;

int getChargeValuesScript(char *pBuffer, int count);
int getRTMeasValuesScript(char *pBuffer, int count) ;
int getNewMeasValuesScript(char *pBuffer, int count);
int getLogScript(char *pBuffer, int count);
int getInfoValuesScript (char *pBuffer, int count);
int getCalValuesScript (char *pBuffer, int count);
int saveSettingsScript (char *pBuffer, int count);
int cancelSettingsScript (char *pBuffer, int count);
int calibrateRespScript(char *pBuffer, int count);
int getSensorNameScript (char *pBuffer, int count);
void parseCGIWriteData(char *buf, int received);
void setCurrentScript(char *buf, int received);
void stopCalScript(char *buf, int received);


#endif /* MAIN_INCLUDE_MEASURETASK_H_ */
