#include "common.h"

#include "arm/arm.h"
#include "hw/irq.h"

#define REG_IRQ_BASE (0x10001000)

#define NR_IRQS 32

static irqHandler irqHandlerTable[NR_IRQS];

typedef struct {
	vu32 enable;
	vu32 pending;
} PACKED irqRegs;

static irqRegs *getIrqRegs(void) {
	return (irqRegs*)(REG_IRQ_BASE);
}

static void irqDummyHandler(u32 irqn) {}

void irqReset(void)
{
	DBG_ASSERT(armInCritical());
	irqRegs *regs = getIrqRegs();

	regs->enable = 0;
	do {
		regs->pending = ~0;
	} while(regs->pending != 0);

	for (uint i = 0; i < NR_IRQS; i++)
		irqHandlerTable[i] = irqDummyHandler;
}

void irqEnable(u32 irqn, irqHandler handler)
{
	DBG_ASSERT(armInCritical());
	DBG_ASSERT(irqn < NR_IRQS);

	if (handler == NULL)
		handler = irqDummyHandler;

	irqHandlerTable[irqn] = handler;
	getIrqRegs()->enable |= BIT(irqn);
}

void irqDisable(u32 irqn)
{
	DBG_ASSERT(armInCritical());
	DBG_ASSERT(irqn < NR_IRQS);
	irqRegs *regs = getIrqRegs();

	regs->enable &= BIT(irqn);
	do {
		regs->pending = BIT(irqn);
	} while(regs->pending & BIT(irqn));

	irqHandlerTable[irqn] = NULL;
}

// called by the exception vector table in AHB RAM
void __attribute__((target("arm"),isr("IRQ"))) irqProcess(void)
{
	irqRegs *regs = getIrqRegs();
	do {
		int irqn = TOP_BIT(regs->pending);
		if (irqn < 0)
			break;
		regs->pending = BIT(irqn);
		(irqHandlerTable[irqn])(irqn);
	} while(1);
}
