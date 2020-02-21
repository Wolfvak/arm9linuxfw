#include "common.h"

#include "arm/arm.h"

#ifdef CORGI
static constexpr char numlut[] = {
	'0','1','2','3','4','5','6','7',
	'8','9','A','B','C','D','E','F'
};
static inline void CORGI_OUTC(char c)
{ *MMIO_REG(0x10008000, 0x0C, u8) = c; }
void
hexfmt(u32 n)
{
	for (int i = 7; i >= 0; i--)
		CORGI_OUTC(numlut[(n >> (i * 4)) & 0xF]);
}
static void
decfmt(s32 n)
{
	char str[12];
	int i = 0;
	if (n < 0) {
		CORGI_OUTC('-');
		n = -n;
	}
	do {
		str[i++] = (n % 10) + '0';
		n /= 10;
	} while(n);
	do { CORGI_OUTC(str[--i]); } while(i > 0);
}
void CORGI_LOGF(const char *fmt, ...)
{
	va_list va;
	va_start(va, fmt);

	while(*fmt) {
		char c = *(fmt++);
		if (c != '%') {
			CORGI_OUTC(c);
		} else {
			switch(*(fmt++)) {
				default:
					CORGI_OUTC('%');
					CORGI_OUTC(fmt[-1]);
					break;

				case 'x':
				case 'X':
					hexfmt(va_arg(va, u32));
					break;

				case 'd':
				case 'D':
					decfmt(va_arg(va, s32));
					break;

				case 'c':
					CORGI_OUTC(va_arg(va, int));
					break;

				case 's':
				{
					const char *str = va_arg(va, const char*);
					if (!str) str = "(null)";
					while(*str) CORGI_OUTC(*(str++));
					break;
				}
			}
		}
	}

	va_end(va);
}
#endif

extern "C" void __cxa_pure_virtual()
{
    ASSERT(0);
}

#ifndef CORGI
static const u8 num_font[] = { 0b00000000, 0b00011000, 0b00100100, 0b00101100, 0b00110100,
0b00100100, 0b00011000, 0b00000000, 0b00000000, 0b00011000, 0b00101000, 0b00001000, 0b00001000,
0b00001000, 0b00111100, 0b00000000, 0b00000000, 0b00011000, 0b00100100, 0b00000100, 0b00001000,
0b00010000, 0b00111100, 0b00000000, 0b00000000, 0b00111000, 0b00000100, 0b00011000, 0b00000100,
0b00000100, 0b00111000, 0b00000000, 0b00000000, 0b00100100, 0b00100100, 0b00111100, 0b00000100,
0b00000100, 0b00000100, 0b00000000, 0b00000000, 0b00111100, 0b00100000, 0b00111000, 0b00000100,
0b00000100, 0b00111000, 0b00000000, 0b00000000, 0b00011100, 0b00100000, 0b00111000, 0b00100100,
0b00100100, 0b00011000, 0b00000000, 0b00000000, 0b00111100, 0b00000100, 0b00000100, 0b00001000,
0b00010000, 0b00010000, 0b00000000, 0b00000000, 0b00011000, 0b00100100, 0b00011000, 0b00100100,
0b00100100, 0b00011000, 0b00000000, 0b00000000, 0b00011000, 0b00100100, 0b00011100, 0b00000100,
0b00000100, 0b00111000, 0b00000000, 0b00000000, 0b00011000, 0b00100100, 0b00111100, 0b00100100,
0b00100100, 0b00100100, 0b00000000, 0b00000000, 0b00111000, 0b00100100, 0b00111000, 0b00100100,
0b00100100, 0b00111000, 0b00000000, 0b00000000, 0b00011100, 0b00100000, 0b00100000, 0b00100000,
0b00100000, 0b00011100, 0b00000000, 0b00000000, 0b00110000, 0b00101000, 0b00100100, 0b00100100,
0b00101000, 0b00110000, 0b00000000, 0b00000000, 0b00111100, 0b00100000, 0b00111100, 0b00100000,
0b00100000, 0b00111100, 0b00000000, 0b00000000, 0b00111100, 0b00100000, 0b00111100, 0b00100000,
0b00100000, 0b00100000, 0b00000000};
#define PX_TO_ADDR(x, y)	(((239 - (y)) + (240 * (x))) * 3)
void draw_char(u8 *fb, int c, int x, int y)
{
	for (int _y = 0; _y < 8; _y++) {
		for (int _x = 0; _x < 8; _x++) {
			u8 *fbpos = fb + PX_TO_ADDR(x + _x, y + _y);

			u8 mask = (num_font[(c * 8) + _y] >> (8 - _x)) & 1;
			u8 col = mask ? 0xff : 0;

			*(fbpos++) = col;
			*(fbpos++) = col;
			*(fbpos++) = col;
		}
	}
}
void draw_hex(u8 *fb, u32 num, int x, int y)
{
	x += 7*8;
	for (int i = 0; i < 8; i++) {
		draw_char(fb, num & 0xf, x, y);
		num >>= 4;
		x -= 8;
	}
}
static void draw_regs(u32 *regs)
{
	u8 *fb = (u8*)(0x18119400);

	for (int i = 0; i < 16; i++)
		draw_hex(fb, regs[i], 16, (i+1) * 8);
}
#endif

extern "C" void handle_fatal_error(int, u32*);
void handle_fatal_error(int src, u32 *regs)
{
	CORGI_LOGF("BUG!\n%x\n", src);

	for (int i = 0; i < 17; i++)
		CORGI_LOGF("%x: %x\n", i, regs[i]);

	while(1) {
		#ifndef CORGI
		draw_regs(regs);
		#else
		ARM::DisableInterrupts();
		ARM::WaitForInterrupt();
		#endif
	}
}
