#include "common.h"

#include "arm/arm.h"

#include "hw/pxi.h"

#define REG_PXI_BASE	(0x10008000)
#define PXI_FIFO_WIDTH	(16)

#define PXI_CNT_TX_FULL		BIT(1)
#define PXI_CNT_TX_FLUSH	BIT(3)

#define PXI_CNT_RX_EMPTY	BIT(8)
#define PXI_CNT_RX_FULL		BIT(9)

#define PXI_CNT_TX_AVAIL_IRQ	BIT(2)
#define PXI_CNT_RX_AVAIL_IRQ	BIT(10)

#define PXI_CNT_FIFO_ERRACK	BIT(14)
#define PXI_CNT_FIFO_ENABLE	BIT(15)

#define PXI_SYNC_CNT_IRQ_TRIG	BIT(5)
#define PXI_SYNC_CNT_IRQ_ENABLE	BIT(7)

typedef struct {
	vu8 syncRx;
	vu8 syncTx;
	u8 unused;
	vu8 syncCnt;

	vu32 cnt;
	vu32 tx;
	vu32 rx;
} PACKED pxiRegs;

static pxiRegs *getPxiRegs(void) {
	return (pxiRegs*)(REG_PXI_BASE);
}

void pxiReset(void)
{
	DBG_ASSERT(armInCritical());

	pxiRegs *regs = getPxiRegs();

	regs->syncCnt = 0;
	regs->cnt = PXI_CNT_TX_FLUSH | \
		PXI_CNT_FIFO_ERRACK | PXI_CNT_FIFO_ENABLE;

	for (uint i = 0; i < PXI_FIFO_WIDTH; i++)
		regs->rx;

	regs->cnt = PXI_CNT_RX_AVAIL_IRQ | \
		PXI_CNT_FIFO_ERRACK | PXI_CNT_FIFO_ENABLE;
	regs->syncCnt = PXI_SYNC_CNT_IRQ_ENABLE;
}

uint pxiRecvEmpty(void)
{
	return getPxiRegs()->cnt & PXI_CNT_RX_EMPTY;
}

uint pxiSendFull(void)
{
	return getPxiRegs()->cnt & PXI_CNT_TX_FULL;
}

u32 pxiRecvMsg(void)
{
	return getPxiRegs()->rx;
}

void pxiSendMsg(u32 msg)
{
	getPxiRegs()->tx = msg;
}

u8 pxiRecvSync(void)
{
	return getPxiRegs()->syncRx;
}

void pxiSendSync(u8 data)
{
	getPxiRegs()->syncTx = data;
}

void pxiTriggerSync(void)
{
	getPxiRegs()->syncCnt = PXI_SYNC_CNT_IRQ_ENABLE | PXI_SYNC_CNT_IRQ_TRIG;
}
