#pragma once

#include "common.h"

namespace HW::TIMER {
	static const u64 BaseFrequency = 67027964ULL;

	static constexpr u32 TicksToMS(u32 t) {
		return ((u64)t * 1000ULL) / BaseFrequency;
	}

	static constexpr u32 MSToTicks(u32 m) {
		return ((u64)m * BaseFrequency) / 1000ULL;
	}

	static const u32 MaxTicks = 0xFFFFFFFF / BaseFrequency;
	static const u32 MaxMs = TicksToMS(MaxTicks);

	#define IRQ_CTR(n)	((n) + IRQ_CTR0)

	enum {
		IRQ_CTR0 = 0x08,
		IRQ_CTR1 = 0x09,
		IRQ_CTR2 = 0x0A,
		IRQ_CTR3 = 0x0B
	};

	void Reset(void);

	void Start(void);
	void Stop(void);

	u32 ReadTicks(void);
}
