#include "common.h"
#include "hw/timer.h"

#define TIMER_BASE	(0x10003000)

#define TIMER_VAL(n)	*MMIO_REG(TIMER_BASE, (n) * 4, u16)
#define TIMER_CNT(n)	*MMIO_REG(TIMER_BASE + 0x02, (n) * 4, u16)

namespace HW::TIMER {
	void Reset(void)
	{
		for (auto i = 0; i < 4; i++) {
			TIMER_CNT(i) = 0;
			TIMER_VAL(i) = 0;
		}
	}

	void Start(void)
	{
		for (auto i = 0; i < 2; i++) {
			TIMER_CNT(i) = 0;
			TIMER_VAL(i) = 0;
		}

		//TIMER_CNT(1) = BIT(2) | BIT(6) | BIT(7); // count-up, IRQ, start
		TIMER_CNT(0) = BIT(6) | BIT(7); // IRQ, start
	}

	void Stop(void)
	{
		TIMER_CNT(0) = 0;
		TIMER_CNT(1) = 0;
	}

	u32 ReadTicks(void)
	{
		return (TIMER_VAL(1) << 16) | TIMER_VAL(0);
	}
}
