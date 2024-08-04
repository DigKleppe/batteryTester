/*
 * timeLog.h
 *
 *
 *  Created on: Feb 8, 2018
 *      Author: dig
 */

#ifndef MAIN_INCLUDE_TIMELOG_H_
#define MAIN_INCLUDE_TIMELOG_H_

#include <stdint.h>

class TimeLog {
public:
	TimeLog(uint32_t size = 5);
  int32_t write(int32_t value);
  int32_t getValue(int32_t);
  void *setSize(uint32_t);
  void clear();

private:
  int32_t *pBuffer;
  uint32_t bufSize;
  uint32_t bufValues;
  uint32_t bufWriteIndex;
};

#endif
