#include "yo_internal.h"

static void*	set_for_zone(void *addr, t_yo_zone_class zone) {
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
void	*yo_allocate_heap(size_t n, t_yo_zone* zone) {
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
		.next = set_for_zone(NULL, zone->zone_class),
	};
	DEBUGINFO("allocated %zu blocks(%zukB) for addr %p", n, BLOCKS_TO_KB(n), mapped);
	const size_t	blocks_allocated = n + 1;
	zone->cons.total_blocks += blocks_allocated;
	return mapped;
}

void	yo_deallocate_heap(t_block_header* heap, t_yo_zone* zone) {
	const size_t	blocks_freed = heap->blocks + 1;
	const size_t	bytes_freed = blocks_freed * BLOCK_UNIT_SIZE;
	if (munmap(heap, bytes_freed) < 0) {
		DEBUGOUT("FAILED: errno = %d, %s", errno, strerror(errno));
		return;
	}
	zone->cons.total_blocks -= blocks_freed;
}