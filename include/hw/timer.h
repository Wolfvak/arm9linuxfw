#pragma once

#include "common.h"

#define TIMER_BASE_FREQ 67027964ULL

/** reset TIMER registers and state */
void timerReset(bool irqen);

/** returns the number of ticks passed since startup */
u64 timerReadTicks(void);

static inline u64 timerTicksToMili(u64 ticks) {
	return (ticks * 1000ULL) / TIMER_BASE_FREQ;
}

static inline u64 timerMiliToTicks(u64 milisec) {
	return (milisec * TIMER_BASE_FREQ) / 1000ULL;
}
