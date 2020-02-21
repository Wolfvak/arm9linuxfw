#pragma once

#include "common.h"

namespace HW::IRQC {
	using IRQ_Handler = void (*)(u32);

	void Reset(void);
	void Enable(u32 irqn, IRQ_Handler hndl);
	void Disable(u32 irqn);
}
