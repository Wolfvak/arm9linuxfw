#pragma once

//#define CORGI

#define BIT(x)      (1ULL << (x))

#ifndef __ASSEMBLER__

#include <cstdint>
#include <cstddef>
#include <cstdarg>

/* Extra type definitions / aliases */
typedef unsigned int uint;

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t   s8;
typedef int16_t  s16;
typedef int32_t  s32;
typedef int64_t  s64;

typedef volatile u8  vu8;
typedef volatile u16 vu16;
typedef volatile u32 vu32;
typedef volatile u64 vu64;

typedef volatile s8  vs8;
typedef volatile s16 vs16;
typedef volatile s32 vs32;
typedef volatile s64 vs64;

/* GCC specific macros */
#define asmv __asm__ volatile

#define APACKED(x) __attribute__((packed, aligned(x)))

#define PACKED	APACKED(4)
#define UNUSED	__attribute__((unused))

#define NORETURN	__attribute__((noreturn))
#define ALIGNED(n)	__attribute__((aligned(n)))

#define LIKELY(x)	__builtin_expect((x), 1)
#define UNLIKELY(x)	__builtin_expect((x), 0)


/* C convenience macros */
#define MMIO_REG(b, o, t)	((volatile t*)((b) + (o)))
#define COUNT_OF(x)	(sizeof(x) / sizeof(*(x)))

#ifndef NDEBUG
#define ASSERT(x)	do{if(!UNLIKELY(x)){__builtin_trap();__builtin_unreachable();}}while(0)
#else
#define ASSERT(x)	(void)0
#endif

static inline void DBGFAULT(u32 c) {
	u32 *fb = (u32*)0x18000000;
	while(fb < (u32*)0x18100000)
		*(fb++) = c;
}

template<class P, class M>
P *CONTAINER_OF(M *ptr, const M P::*mem)
{ return (P*)((char*)ptr - ((size_t)&(((P*)0)->*mem))); }

template<typename T>
unsigned ArrayDistance(const T *a, const T *b)
{
	uintptr_t p1, p2;
	p1 = (uintptr_t)a;
	p2 = (uintptr_t)b;
	return (p2 - p1) / sizeof(T);
}


/* arithmetic helpers */
#define TOP_BIT(n)	((n) ? (31 - __builtin_clz(n)) : -1)
#define LOW_BIT(n)	((n) ? __builtin_ctz(n) : -1)

#define ALIGN_DOWN(n, a)	(((n) / (a)) * (a))
#define ALIGN_UP(n, a)	ALIGN_DOWN((n) + (a) - 1, (a))

#define NUM_IS_POW2(x)	(!((x) & ((x) - 1)))
#define POW2_MOD(x, y)	((x) & ((y) - 1))

// only works if n < 2*d
#define FASTRBMOD(n, d)	(((n) < (d)) ? (n) : ((d) - (n)))

#define MIN(x, y)	std::min(x, y)
#define MAX(x, y)	std::max(x, y)

static constexpr u32 ExtractBits(u32 *arr, u32 start, u32 n)
{
	if (n > 32)
		return -1;

	u32 mask = BIT(n) - 1;
	u32 off = start >> 5;
	u32 shift = start & 0x1F;

	u32 ret = arr[off] >> shift;
	if ((n + shift) > 32)
		ret |= arr[off+1] << (32 - shift);
	return ret & mask;
}

#ifdef CORGI
void CORGI_LOGF(const char *fmt, ...);
#else
static inline void CORGI_LOGF(const char *fmt, ...) {}
#endif

#define CORGI_LOG(s)	CORGI_LOGF(s)

static inline void simplewait(vu32 n)
{ while(n--) asmv("mov r0, r0\n\t":::"memory"); }

#endif // __ASSEMBLER__
