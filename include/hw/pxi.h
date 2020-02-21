#pragma once

#include "common.h"

namespace HW::PXI {
	enum {
		IRQ_SYNC_TRIGGERED = 0xC,
		IRQ_TX_NOT_FULL = 0xD,
		IRQ_RX_NOT_EMPTY = 0xE,
	};

	void Reset(void);

	uint RecvEmpty(void);

	u32 Recv(void);
	void Send(u32 w);

	void SendData(const u32 *data, uint n);
	void RecvData(u32 *data, uint n);

	u8 RecvSync(void);
	void SendSync(u8 data);
	void TriggerSync(void);
}
