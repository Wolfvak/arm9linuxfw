#include "common.h"

#include "hw/timer.h"

#define REG_TIMER_BASE	(0x10003000)

#define CNT_START	BIT(7)
#define CNT_IRQEN	BIT(6)
#define CNT_COUNTUP	BIT(2)

#define CNT_NO_PRESCALER	0
#define CNT_PRESCALER_64	1
#define CNT_PRESCALER_256	2
#define CNT_PRESCALER_1024	3

typedef struct {
	vu16 val;
	vu16 cnt;
} PACKED timerRegs;

static timerRegs *getTimerRegs(u32 tmr) {
	return &((timerRegs*)(REG_TIMER_BASE))[tmr];
}

void timerReset(bool irqen)
{
	for (uint i = 0; i < 4; i++) {
		timerRegs *regs = getTimerRegs(i);
		regs->cnt = 0;
		regs->val = 0;
	}

	for (uint i = 3; i >= 1; i--)
		getTimerRegs(i)->cnt = CNT_START | CNT_COUNTUP | CNT_NO_PRESCALER;
	getTimerRegs(0)->cnt = CNT_START |
		(irqen ? CNT_IRQEN : 0) | CNT_NO_PRESCALER;
}

u64 timerReadTicks(void)
{
	u32 hi;
	u16 quart, lo_start, lo_end;
	timerRegs *tmr0 = getTimerRegs(0);

	do { // retry on overflow
		lo_start = tmr0->val;
		quart = getTimerRegs(1)->val;
		hi = getTimerRegs(2)->val | (getTimerRegs(3)->val << 16);
		lo_end = tmr0->val;
	} while(UNLIKELY(lo_start > lo_end));

	return ((u64)hi << 32) | (quart << 16) | lo_end;
}
