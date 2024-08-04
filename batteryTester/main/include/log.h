/*
 * log.h
 *
 *  Created on: Nov 3, 2023
 *      Author: dig
 */

#ifndef MAIN_INCLUDE_LOG_H_
#define MAIN_INCLUDE_LOG_H_


#include <stdint.h>
#include "measureTask.h"

#define LOGINTERVAL 				(5*60) // seconds
#define MAXDAYLOGVALUES				((24*60*60)/LOGINTERVAL)

typedef struct {
	int32_t timeStamp;
	float voltage[NR_CHANNELS];
} log_t;

extern int dayLogRxIdx;
extern int dayLogTxIdx;
extern uint32_t  timeStamp;

extern log_t dayLog[ MAXDAYLOGVALUES];

int getDayLogScript(char *pBuffer, int count);
void addToLog( log_t logValue);
void clearLog( void );


#endif /* MAIN_INCLUDE_LOG_H_ */
