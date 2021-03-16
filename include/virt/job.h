#pragma once

#include "common.h"

typedef struct virtJob_s {
	u16 firstDesc;
	u16 currentDesc;
	u32 totalWritten;
} virtJob_s;

static inline void virtJobAddWritten(virtJob_s *vjob, u32 written) {
	vjob->totalWritten += written;
}
