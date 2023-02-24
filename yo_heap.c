#include "yo_internal.h"

static void*	set_for_zone(void *addr, t_yo_zone_class zone)
{
	switch (zone) {
		case YO_ZONE_TINY:
			return SET_IS_TINY(addr);
		case YO_ZONE_SMALL:
			return SET_IS_SMALL(addr);
		case YO_ZONE_LARGE:
			return SET_IS_LARGE(addr);
	}
}

// n + 1 個分のブロックを mmap で確保して返す
// 失敗した場合は NULL が返る
void	*yo_allocate_heap(size_t n, t_yo_zone_class zone) {
	void	*mapped = mmap(
		NULL,
		(n + 1) * BLOCK_UNIT_SIZE,
		PROT_READ | PROT_WRITE,
		MAP_ANON | MAP_PRIVATE,
		-1, 0);
	if (mapped == MAP_FAILED) {
		return NULL;
	}
	// 確保できたら先頭にブロックヘッダを詰める
	t_block_header	*header = mapped;
	*header = (t_block_header) {
		.blocks = n,
		.next = set_for_zone(NULL, zone),
	};
	DEBUGOUT("allocated %zu blocks for addr %p", n, mapped);
	return mapped;
}
