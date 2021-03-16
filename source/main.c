#include "common.h"

#include "arm/arm.h"

#include "hw/irq.h"
#include "hw/pxi.h"
#include "hw/timer.h"

#include "virt/irq.h"
#include "virt/manager.h"

#define PXIMSG_CMD_REG(m)	(((m) & (BIT(24) - 1)))
#define PXIMSG_CMD_DEV(m)	(((m) >> 24) & 0x7F)
#define PXIMSG_CMD_TYPE(m)	(((m) >> 31) & 0x01)

static void onPxiRecv(u32 irqn) {
	while(!pxiRecvEmpty()) {
		u32 msg, dev, cmd, reg, data;

		msg = pxiRecvMsg();

		cmd = PXIMSG_CMD_TYPE(msg);
		dev = PXIMSG_CMD_DEV(msg);
		reg = PXIMSG_CMD_REG(msg);

		switch(cmd) {
		case 0: // READ REGISTER
			data = virtDevReadReg(dev, reg);
			while(pxiSendFull());
			pxiSendMsg(data);
			break;

		case 1: // WRITE REGISTER
			while(pxiRecvEmpty());
			data = pxiRecvMsg();
			virtDevWriteReg(dev, reg, data);
			break;
		}
	}
}

void NORETURN arm9linuxfwEntry(void)
{
	irqReset();

	pxiReset();
	timerReset(false);

	virtDevInitAll();

	// Enable interrupts, wait for new commands and process buffers
	irqEnable(IRQ_PXI_SYNC, NULL);
	irqEnable(IRQ_PXI_TX, NULL);
	irqEnable(IRQ_PXI_RX, onPxiRecv);

	armEnableInterrupts();

	while(1) {
		DBG_ASSERT(!armInCritical());

		if (!virtDevProcessPending())
			armWaitForInterrupt();
	}
}
