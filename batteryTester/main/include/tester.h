/*
 * tester.h
 *
 *  Created on: Jun 28, 2024
 *      Author: dig
 */

#ifndef MAIN_INCLUDE_TESTER_H_
#define MAIN_INCLUDE_TESTER_H_

#define NR_CHANNELS 4

typedef enum {
  STATUS_NO_BAT,
  STATUS_CHARGING,
  STATUS_DECHARGING,
  STATUS_TESTED,
  STATUS_CHARGED,
  STATUS_INA_ERROR
} testChannelStatus;

typedef struct {
  testChannelStatus status;
  int setCurrent; // mA for currentSource
  int chargeCurrent; // mA 
  int deChargeCurrent; // mA
  int current;
  int averagedCurrent;  // mA
  int measuredCapacity; // mAH
  int measuredPower;     // mW actual 
  int inPower;	// mW charged pPower
  int outPower;	// mW decharged Power
  float voltage;
} testChannel_t;

extern testChannel_t testChannel[NR_CHANNELS];

#define CHARGEDVOLATGE 2.5
#define DECHARDEDVOLATAGE 0.9
#define NOBATVOLTAGE 4.0

void testTask (void *pvParameter);
void currentRegulatorTask (void *pvParameter);


#endif /* MAIN_INCLUDE_TESTER_H_ */
