#pragma once

#include "arm/arm.h"

typedef u8 event_t;

#define EVENT_INITIALIZE	0

static inline void eventInitialize(event_t *ev) {
	*ev = 0;
}

static inline void eventTrigger(event_t *ev) {
	*ev = 1;
}

static inline bool eventTest(event_t *ev) {
	return armSWPB(0, ev) != 0;
}

#define eventClear	eventInitialize
#define eventWait(ev)	\
	while(!eventTest(ev)) { armWaitForInterrupt(); }
