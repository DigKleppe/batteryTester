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
  STATUS_WAIT1,
  STATUS_DECHARGING,
  STATUS_TESTED,
  STATUS_WAIT2,
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
  int samples;
  int measuredPower;     // mW actual 
  int inPower;	// mW charged pPower
  int outPower;	// mW decharged Power
  float voltage;
  float maxVoltage;
  float currentCalibration;
} testChannel_t;

extern testChannel_t testChannel[NR_CHANNELS];

#define MAXCHARGEDVOLATGE 1.3 //  1.9
#define DECHARDEDVOLATAGE 1.1 //   0.9
#define NOBATVOLTAGE 4.0
#define NOCURRENT	2 // mA
#define CHARGEDDROP	0.015 // V full charged 

void testTask (void *pvParameter);
void currentRegulatorTask (void *pvParameter);



#endif /* MAIN_INCLUDE_TESTER_H_ */
