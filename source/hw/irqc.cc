#include "common.h"

#include "arm/arm.h"
#include "hw/irqc.h"

#define REG_IRQ_BASE (0x10001000)

#define REG_IRQ_ENA	MMIO_REG(REG_IRQ_BASE, 0x00, u32)
#define REG_IRQ_ACK	MMIO_REG(REG_IRQ_BASE, 0x04, u32)

#define NR_IRQS 32

namespace HW::IRQC {
	static IRQ_Handler irq_cb[NR_IRQS];

	// no-op handler for unregistered interrupts
	static void dummy_irq_handler(u32 irqn) {}

	void Reset(void)
	{
		ASSERT(ARM::InCritical());

		*REG_IRQ_ENA = 0;
		do {
			*REG_IRQ_ACK = ~0;
		} while(*REG_IRQ_ACK != 0);
		for (unsigned i = 0; i < NR_IRQS; i++)
			irq_cb[i] = dummy_irq_handler;
	}

	void Enable(u32 irqn, IRQ_Handler hndl)
	{
		ASSERT(ARM::InCritical());
		ASSERT(irqn < NR_IRQS);

		if (hndl == nullptr)
			hndl = dummy_irq_handler;
		irq_cb[irqn] = hndl;
		*REG_IRQ_ENA |= BIT(irqn);
	}

	void Disable(u32 irqn)
	{
		ASSERT(ARM::InCritical());
		ASSERT(irqn < NR_IRQS);

		*REG_IRQ_ENA &= ~BIT(irqn);
		irq_cb[irqn] = dummy_irq_handler;
		do {
			*REG_IRQ_ACK = BIT(irqn);
		} while(*REG_IRQ_ACK & BIT(irqn));
	}

	static void HandleInterrupts(void)
	{
		int id;
		while((id = TOP_BIT(*REG_IRQ_ACK)) > 0) {
			*REG_IRQ_ACK = BIT(id); // get pending and acknowledge
			(irq_cb[id])(id);
		}
	}
}

// called by the exception vector table in AHB RAM
// not the fastest dispatch method but
// should be good enough nonetheless
extern "C" void ProcessRootIRQ_FromC(void);
void __attribute__((interrupt("IRQ"))) ProcessRootIRQ_FromC(void)
{
	HW::IRQC::HandleInterrupts();
}
