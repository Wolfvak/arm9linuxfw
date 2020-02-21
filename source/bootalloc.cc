#include "common.h"
#include "bootalloc.h"

extern u8 __heap_s, __heap_e;
static u8 *bootalloc_heap_pos;

void *bootalloc(size_t sz)
{
	void *ret = nullptr;

	if ((bootalloc_heap_pos + sz) <= &__heap_e) {
		ret = (void*)bootalloc_heap_pos;
		bootalloc_heap_pos += sz;
	}

	CORGI_LOGF("allocating %d bytes @ %x\n", sz, ret);
	ASSERT(ret != nullptr);
	return ret;
}

extern "C" void bootalloc_init(void)
{ bootalloc_heap_pos = &__heap_s; }
