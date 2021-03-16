#include "common.h"

#include "hw/irq.h"
#include "hw/ndma.h"

#include "sys/event.h"

#define NUM_CHANNELS	(8)

#define NDMA_CNT_IRQEN	BIT(30)
#define NDMA_CNT_START	BIT(31)

#define REG_NDMA_BASE	(0x10002000)

typedef struct {
	vu32 src;
	vu32 dst;
	vu32 tcnt;
	vu32 wcnt;
	vu32 clk_cnt;
	vu32 fill_data;
	vu32 cnt;
} PACKED ndmaChanRegs;

static event_t ndmaXferEvent[NUM_CHANNELS];

static vu32 *getNdmaGlobalCnt(void) {
	return (vu32*)(REG_NDMA_BASE);
}

static ndmaChanRegs *getNdmaChanRegs(u32 chan) {
	return &((ndmaChanRegs*)(REG_NDMA_BASE + 0x04))[chan];
}

static void ndmaIrqHandler(u32 irqn) {
	u32 channel = irqn - IRQ_NDMA0;
	eventTrigger(&ndmaXferEvent[channel]);
}

void ndmaReset(u32 arbitration_flags)
{
	vu32 *globalCnt = getNdmaGlobalCnt();

	*globalCnt = 0;
	for (uint i = 0; i < NUM_CHANNELS; i++) {
		ndmaChanRegs *regs = getNdmaChanRegs(i);
		regs->cnt = 0;
		regs->clk_cnt = 0;
	}
	*globalCnt = arbitration_flags | BIT(0); // enable
}

void ndmaClockControl(u32 chan, u32 control)
{
	DBG_ASSERT(chan < NUM_CHANNELS);
	getNdmaChanRegs(chan)->clk_cnt = control;
}

void ndmaXferAsync(u32 chan, u32 dst, u32 src, u32 len, u32 flags)
{
	ndmaChanRegs *regs;
	DBG_ASSERT(chan < NUM_CHANNELS);

	regs = getNdmaChanRegs(chan);
	eventClear(&ndmaXferEvent[chan]);

	regs->src = src;
	regs->dst = dst;
	regs->fill_data = src;

	regs->tcnt = 0; // total length for repeats
	regs->wcnt = len / 4; // number of physical blocks
	regs->cnt = flags | NDMA_CNT_IRQEN | NDMA_CNT_START;
}

bool ndmaXferDone(u32 chan)
{
	DBG_ASSERT(chan < NUM_CHANNELS);
	return eventTest(&ndmaXferEvent[chan]);
}

void ndmaXferWait(u32 chan)
{
	DBG_ASSERT(chan < NUM_CHANNELS);
	eventWait(&ndmaXferEvent[chan]);
}
