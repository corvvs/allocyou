#include "yoyo_internal.h"

size_t	zone_bytes_for_zone_type(t_yoyo_zone_type zone_type) {
	switch (zone_type) {
		case YOYO_ZONE_TINY:
			return ZONE_TINY_BYTE;
		case YOYO_ZONE_SMALL:
			return ZONE_SMALL_BYTE;
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
unsigned int	get_block_index(t_yoyo_zone* zone, t_yoyo_chunk* head) {
	t_yoyo_chunk*	heap_start = (void*)zone + zone->offset_heap;
	return head - heap_start;
}
