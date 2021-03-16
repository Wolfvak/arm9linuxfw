#pragma once

#include "common.h"

#include "arm/bfn.h"

/** ARM assembly wrappers */
#define ARM_MCR(cp, op1, reg, crn, crm, op2) \
	asmv( \
	"MCR " #cp ", " #op1 ", %[R], " #crn ", " #crm ", " #op2 "\n\t" \
	:: [R] "r"(reg) : "memory", "cc")

#define ARM_MRC(cp, op1, reg, crn, crm, op2) \
	asmv( \
	"MRC " #cp ", " #op1 ", %[R], " #crn ", " #crm ", " #op2 "\n\t" \
	: [R] "=r"(reg) :: "memory", "cc")

#define ARM_MSR(cp, reg) \
	asmv( \
	"MSR " #cp ", %[R]\n\t" \
	:: [R] "r"(reg) : "memory", "cc")

#define ARM_MRS(reg, cp) \
	asmv( \
	"MRS %[R], " #cp "\n\t" \
	: [R] "=r"(reg) :: "memory", "cc")

/** ARM processor constants */

/** CPSR interrupt mask - disables interrupts when set */
#define ARM_INT_MASK	0xC0

/** instruction cache size */
#define ARM_ICACHE_SIZE 4096

/** data cache size */
#define ARM_DCACHE_SIZE 4096


/** performs a data sync barrier and makes sure the write
 *  buffer is empty before proceeding to the next instruction */
static inline void armDataSyncBarrier(void) {
	ARM_MCR(p15, 0, 0, c7, c10, 4);
}

/** invalidates the entire instruction cache */
static inline void armInvalidateInstCache(void) {
	ARM_MCR(p15, 0, 0, c7, c5, 0);
}

/** invalidates a range of the instruction cache */
static inline void armInvalidateInstCacheRange(u32 addr, size_t len) {
	((void (*)(u32, u32))(BFN_INVALIDATE_ICACHE_RANGE))(addr, len);
}

/** invalidates the entire data cache */
static inline void armInvalidateDataCache(void) {
	ARM_MCR(p15, 0, 0, c7, c6, 0);
}

/** invalidates a range of the data cache */
static inline void armInvalidateDataCacheRange(u32 addr, size_t len) {
	((void (*)(u32, u32))(BFN_INVALIDATE_DCACHE_RANGE))(addr, len);
}

/** writes back the entire data cache */
static inline void armWritebackDataCache(void) {
	((void (*)(void))(BFN_WRITEBACK_DCACHE))();
}

/** writes back a range of the data cache */
static inline void armWritebackDataCacheRange(u32 addr, size_t len) {
	((void (*)(u32, u32))(BFN_WRITEBACK_DCACHE_RANGE))(addr, len);
}

/** writes back and invalidates the entire data cache */
static inline void armWritebackInvalidateDataCache(void) {
	((void (*)(void))(BFN_WRITEBACK_INVALIDATE_DCACHE))();
}

/** writes back and invalidates a range of the data cache */
static inline void armWritebackInvalidateDataCacheRange(u32 addr, size_t len) {
	((void (*)(u32, u32))(BFN_WRITEBACK_INVALIDATE_DCACHE_RANGE))(addr, len);
}

/** delays program execution by a given number of cycles */
static inline void armDelayCycles(u32 cycles) {
	((void (*)(u32))(BFN_WAITCYCLES))(cycles);
}

/** preserves the interrupt state and disables interrupts */
static inline u32 armEnterCritical(void) {
	u32 sr;
	ARM_MRS(sr, cpsr);
	ARM_MSR(cpsr_c, sr | ARM_INT_MASK);
	return sr & ARM_INT_MASK;
}

/** restores the interrupt state after a critical section */
static inline void armLeaveCritical(u32 stat) {
	u32 sr;
	ARM_MRS(sr, cpsr);
	sr = (sr & ~ARM_INT_MASK) | stat;
	ARM_MSR(cpsr_c, sr);
}

/** enables processor interrupts */
static inline void armEnableInterrupts(void) {
	u32 sr;
	ARM_MRS(sr, cpsr);
	ARM_MSR(cpsr_c, sr & ~ARM_INT_MASK);
}

/** disables processor interrupts */
static inline void armDisableInterrupts(void) {
	u32 sr;
	ARM_MRS(sr, cpsr);
	ARM_MSR(cpsr_c, sr | ARM_INT_MASK);
}

/** returns non-zero if processor interrupts are disabled */
static inline uint armInCritical(void) {
	u32 sr;
	ARM_MRS(sr, cpsr);
	return sr & ARM_INT_MASK;
}

/** sleeps until an interrupt triggers wakeup */
static inline void armWaitForInterrupt(void) {
	armDataSyncBarrier();
	ARM_MCR(p15, 0, 0, c7, c0, 4);
}

/** performs an atomic word swap */
static inline u32 armSWP(u32 val, u32 *addr) {
	u32 old;
	asmv(
		"swp %0, %1, [ %2 ]\n\t"
		: "=r"(old) : "r"(val), "r"(addr) : "memory"
	);
	return old;
}

/** atomic byte swap */
static inline u32 armSWPB(u8 val, u8 *addr) {
	u32 old;
	asmv(
		"swpb %0, %1, [ %2 ]\n\t"
		: "=r"(old) : "r"(val), "r"(addr) : "memory"
	);
	return old;
}

/** process id register getter and setter */
static inline u32 armGetPID(void) {
	u32 pid;
	ARM_MRC(p15, 0, pid, c13, c0, 1);
	return pid;
}

static inline void armSetPID(u32 pid) {
	ARM_MCR(p15, 0, pid, c13, c0, 1);
}
