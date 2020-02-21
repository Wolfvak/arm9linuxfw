#include "common.h"

#include "hw/pxi.h"

#define PXI_BASE	(0x10008000)
#define PXI_FIFO_WIDTH	16

#define REG_PXI_SYNC_CNT	*MMIO_REG(PXI_BASE, 0x03, u8)
#define REG_PXI_SYNC_RX	*MMIO_REG(PXI_BASE, 0x00, u8)
#define REG_PXI_SYNC_TX	*MMIO_REG(PXI_BASE, 0x01, u8)

#define REG_PXI_CNT	*MMIO_REG(PXI_BASE, 0x04, u16)
#define REG_PXI_TX	*MMIO_REG(PXI_BASE, 0x08, u32)
#define REG_PXI_RX	*MMIO_REG(PXI_BASE, 0x0C, u32)

#define PXI_CNT_TX_EMPTY	BIT(0)
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

namespace HW::PXI {
	void Reset(void)
	{
		REG_PXI_SYNC_CNT = 0;

		REG_PXI_CNT = PXI_CNT_TX_FLUSH | PXI_CNT_FIFO_ERRACK| PXI_CNT_FIFO_ENABLE;

		for (int i = 0; i < PXI_FIFO_WIDTH; i++)
			REG_PXI_RX;

		REG_PXI_CNT = 0;
		REG_PXI_CNT = PXI_CNT_RX_AVAIL_IRQ | PXI_CNT_FIFO_ERRACK |
							PXI_CNT_FIFO_ENABLE;
		//REG_PXI_SYNC_CNT = PXI_SYNC_CNT_IRQ_ENABLE;

		REG_PXI_SYNC_TX = 0;
	}

	uint RecvEmpty(void)
	{
		return (REG_PXI_CNT & PXI_CNT_RX_EMPTY);
	}

	void Send(u32 w)
	{
		while(REG_PXI_CNT & PXI_CNT_TX_FULL);
		REG_PXI_TX = w;
	}

	u32 Recv(void)
	{
		while(REG_PXI_CNT & PXI_CNT_RX_EMPTY);
		return REG_PXI_RX;
	}

	void SendData(const u32 *data, uint n)
	{
		while(n--) {
			while(REG_PXI_CNT & PXI_CNT_TX_FULL);
			REG_PXI_TX = *(data++);
		}
	}

	void RecvData(u32 *data, uint n)
	{
		while(n--) {
			while(REG_PXI_CNT & PXI_CNT_RX_EMPTY);
			*(data++) = REG_PXI_RX;
		}
	}

	u8 RecvSync(void)
	{
		return REG_PXI_SYNC_RX;
	}

	void SendSync(u8 data)
	{
		REG_PXI_SYNC_TX = data;
	}

	void TriggerSync(void)
	{
		REG_PXI_SYNC_CNT |= PXI_SYNC_CNT_IRQ_TRIG;
	}
}
