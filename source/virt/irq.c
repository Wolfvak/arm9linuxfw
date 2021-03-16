#include "common.h"

#include "arm/arm.h"
#include "hw/pxi.h"
#include "sys/event.h"
#include "virt/irq.h"

#define VIRQ_BANKS	(VIRQ_MAX / 32)

static event_t eventVirq;
static u32 pendingVirq[VIRQ_MODES][VIRQ_BANKS];

void virtIrqReset(void)
{
	eventInitialize(&eventVirq);

	for (uint m = 0; m < VIRQ_MODES; m++) {
		for (uint b = 0; b < VIRQ_BANKS; b++) {
			pendingVirq[m][b] = 0;
		}
	}
}

u32 virtIrqSet(u32 devId, uint mode)
{
	u32 ret = 0;
	uint bankId = devId / 32;
	uint bitOff = BIT(devId % 32);

	if ((bankId < VIRQ_BANKS) && (mode < VIRQ_MODES)) {
		ret = pendingVirq[mode][bankId];
		pendingVirq[mode][bankId] |= bitOff;
		eventTrigger(&eventVirq);
	}

	return ret;
}

u32 virtIrqGet(u32 bankId, uint mode)
{
	if ((bankId < VIRQ_BANKS) && (mode < VIRQ_MODES))
		return armSWP(0, &pendingVirq[mode][bankId]);
	return 0;
}

/* only trigger the ARM11 if absolutely necessary */
void virtIrqSync(void)
{
	if (eventTest(&eventVirq))
		pxiTriggerSync();
}
