#pragma once

#include "common.h"

namespace ARM {
	#define MCR(cp, op1, reg, crn, crm, op2) \
		asmv( \
		"MCR " #cp ", " #op1 ", %[R], " #crn ", " #crm ", " #op2 "\n\t" \
		:: [R] "r"(reg) : "memory", "cc")

	#define MRC(cp, op1, reg, crn, crm, op2) \
		asmv( \
		"MRC " #cp ", " #op1 ", %[R], " #crn ", " #crm ", " #op2 "\n\t" \
		: [R] "=r"(reg) :: "memory", "cc")

	#define MSR(cp, reg) \
		asmv( \
		"MSR " #cp ", %[R]\n\t" \
		:: [R] "r"(reg) : "memory", "cc")

	#define MRS(reg, cp) \
		asmv( \
		"MRS %[R], " #cp "\n\t" \
		: [R] "=r"(reg) :: "memory", "cc")

	enum Mode {
		User = 0x10,
		FIQ = 0x11,
		IRQ = 0x12,
		Supervisor = 0x13,
		Abort = 0x17,
		Undefined = 0x18,
		System = 0x1F,
	};

	#define ModeMask 0xF
	#define IntDisableMask	0xC0

	static constexpr u32 ICacheSize(void) {
		return 0x1000;
	}

	static constexpr u32 DCacheSize(void) {
		return 0x1000;
	}

	static inline void DSB(void) {
		MCR(p15, 0, 0, c7, c10, 4);
	}

	static inline void InvIC(void) {
		MCR(p15, 0, 0, c7, c5, 0);
	}

	static inline void InvIC_Range(void *start, size_t len) {
		u32 addr = (u32)start & ~0x1F;
		len >>= 5;

		if (len >= ICacheSize()) {
			InvIC();
			return;
		}

		do {
			MCR(p15, 0, addr, c7, c5, 1);
			addr += 0x20;
		} while(len--);
	}

	static inline void InvDC(void) {
		MCR(p15, 0, 0, c7, c6, 0);
	}

	static inline void InvDC_Range(void *start, size_t len) {
		u32 addr = (u32)start & ~0x1F;
		len >>= 5;

		if (len >= DCacheSize()) {
			InvDC();
			return;
		}

		do {
			MCR(p15, 0, addr, c7, c6, 1);
			addr += 0x20;
		} while(len--);
	}

	static inline void WbDC(void) {
		u32 seg = 0;
		do {
			u32 ind = 0;
			do {
				MCR(p15, 0, seg | ind, c7, c10, 2);
				ind += 0x20;
			} while(ind < 0x400);
			seg += 0x40000000;
		} while(seg != 0);
	}

	static inline void WbDC_Range(void *start, size_t len) {
		u32 addr = (u32)start & ~0x1F;
		len >>= 5;

		if (len >= DCacheSize()) {
			WbDC();
			return;
		}

		do {
			MCR(p15, 0, addr, c7, c10, 1);
			addr += 0x20;
		} while(len--);
	}

	static inline void WbInvDC(void) {
		u32 seg = 0;
		do {
			u32 ind = 0;
			do {
				MCR(p15, 0, seg | ind, c7, c14, 2);
				ind += 0x20;
			} while(ind < 0x400);
			seg += 0x40000000;
		} while(seg != 0);
	}

	static inline void WbInvDC_Range(void *start, size_t len) {
		u32 addr = (u32)start & ~0x1F;
		len >>= 5;

		if (len >= DCacheSize()) {
			WbInvDC();
			return;
		}

		do {
			MCR(p15, 0, addr, c7, c14, 1);
			addr += 0x20;
		} while(len--);
	}

	static inline u32 CPSR_Get(void) {
		u32 sr;
		MRS(sr, cpsr);
		return sr;
	}

	static inline void CPSR_Set(u32 sr) {
		MSR(cpsr, sr);
	}

	static inline void CPSR_c_Set(u32 sr) {
		MSR(cpsr_c, sr);
	}

	static inline void EnableInterrupts(void) {
		CPSR_c_Set(CPSR_Get() & ~IntDisableMask);
	}

	static inline void DisableInterrupts(void) {
		CPSR_c_Set(CPSR_Get() | IntDisableMask);
	}

	static inline u32 EnterCritical(void) {
		u32 stat = CPSR_Get();
		CPSR_c_Set(stat | IntDisableMask);
		return stat & IntDisableMask;
	}

	static inline void LeaveCritical(u32 flags) {
		CPSR_c_Set((CPSR_Get() & ~IntDisableMask) | flags);
	}

	static inline bool InCritical(void) {
		return (CPSR_Get() & IntDisableMask) != 0;
	}

	static inline void WaitForInterrupt(void) {
		DSB();
		MCR(p15, 0, 0, c7, c0, 4);
	}

	static inline u32 Swap(u32 nword, u32 *addr) {
		u32 loaded = 0;
		asmv(
			"swp %[ld], %[og], [ %[ad] ]\n\t"
			: [ld] "+r"(loaded) : [og] "r"(nword), [ad] "r"(addr) : "memory"
		);
		return loaded;
	}

	static inline u8 SwapByte(u8 nbyte, u8 *addr) {
		u8 loaded = 0;
		asmv(
			"swpb %[ld], %[og], [ %[ad] ]\n\t"
			: [ld] "+r"(loaded) : [og] "r"(nbyte), [ad] "r"(addr) : "memory"
		);
		return loaded;
	}

	class CriticalSection {
	public:
		CriticalSection(void) : irqf(ARM::EnterCritical()) { }
		~CriticalSection(void) { ARM::LeaveCritical(this->irqf); }
		u32 irqf;
	};
}
