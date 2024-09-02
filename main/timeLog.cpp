/*
 * timeLog.cpp
 *
 *  Created on: Aug 4, 2024
 *      Author: dig
 */


#include "include/timeLog.h"
#include <stdlib.h>

#ifdef USE_FREERTOS
#include "freertos/FreeRTOS.h"
#endif

TimeLog::TimeLog(uint32_t size) {
	bufWriteIndex = 0;
	bufValues = 0;
	bufSize = size;
	pBuffer = (int32_t*) malloc(bufSize * 4);
}

void TimeLog::clear() {
	bufWriteIndex = 0;
	bufValues = 0;
}

void* TimeLog::setSize(uint32_t size) {
	if (size > 0) {
		bufSize = size;
		bufWriteIndex = 0;
		bufValues = 0;
		free(pBuffer);
		pBuffer = (int32_t*) malloc(bufSize * 4);
		return pBuffer;
	}
	return NULL;
}

// write cyclic buffer
int32_t TimeLog::write(int32_t value) {
	if (pBuffer == NULL) {
		return -1;
	} else {
		if (bufValues < bufSize)
			bufValues++;

		*(pBuffer + bufWriteIndex) = value;
		bufWriteIndex++;
		if (bufWriteIndex == bufSize)
			bufWriteIndex = 0;
	}
	return 0;
}

int32_t TimeLog::getValue( int32_t past) {
	if (past > bufValues)
		return -1;
	int idx = bufWriteIndex - past;
	if ( idx < 0)
		idx+= bufSize;

	return pBuffer[idx];
}



