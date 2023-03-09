#include "internal.h"

size_t	zone_bytes_for_zone_type(t_yoyo_zone_type zone_type) {
	switch (zone_type) {
		case YOYO_ZONE_TINY:
			return FLOOR_BY(ZONE_TINY_BYTE, getpagesize());
		case YOYO_ZONE_SMALL:
			return FLOOR_BY(ZONE_SMALL_BYTE, getpagesize());
		default:
			return 0;
	}
}

size_t	max_chunk_blocks_for_zone_type(t_yoyo_zone_type zone_type) {
	switch (zone_type) {
		case YOYO_ZONE_TINY:
			return TINY_MAX_CHUNK_BLOCK;
		case YOYO_ZONE_SMALL:
			return SMALL_MAX_CHUNK_BLOCK;
		default:
			return SIZE_MAX;
	}
}

t_yoyo_zone_type	zone_type_for_bytes(size_t n) {
	size_t	block_needed = BLOCKS_FOR_SIZE(n);
	if (block_needed > SMALL_MAX_CHUNK_BLOCK) {
		DEBUGWARN("zone for %zu B: LARGE", n);
		return YOYO_ZONE_LARGE;
	} else if (block_needed > TINY_MAX_CHUNK_BLOCK) {
		DEBUGWARN("zone for %zu B: SMALL", n);
		return YOYO_ZONE_SMALL;
	}
	DEBUGWARN("zone for %zu B: TINY", n);
	return YOYO_ZONE_TINY;
}

// head にあるブロックの, その zone (の heap)におけるインデックスを求める.
unsigned int	get_block_index(const t_yoyo_zone* zone, const t_yoyo_chunk* head) {
	const t_yoyo_chunk*	heap_start = (void*)zone + zone->offset_heap;
	return head - heap_start;
}

// TINY / SMALL chunk の所属する zone を返す.
// chunk が LARGE だった場合は NULL を返す.
t_yoyo_zone*	get_zone_of_chunk(const t_yoyo_chunk* chunk) {
	const size_t whole_blocks = chunk->blocks;
	DEBUGOUT("whole_blocks: %zu", whole_blocks);
	assert(whole_blocks > 0);
	const size_t usable_blocks = whole_blocks - 1;
	DEBUGOUT("usable_blocks: %zu", usable_blocks);
	assert(usable_blocks > 0);

	const t_yoyo_zone_type zone_type = zone_type_for_bytes(usable_blocks * BLOCK_UNIT_SIZE);
	DEBUGOUT("zone_type is %d", zone_type);
	if (zone_type == YOYO_ZONE_LARGE) {
		return NULL;
	}

	const uintptr_t zone_addr_mask = ~((zone_type == YOYO_ZONE_TINY ? ZONE_TINY_BYTE : ZONE_SMALL_BYTE) - 1);
	DEBUGOUT("zone_addr_mask is %lx", zone_addr_mask);
	return (t_yoyo_zone*)((uintptr_t)chunk & zone_addr_mask);
}
