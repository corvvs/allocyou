#include "yoyo_internal.h"

size_t	zone_bytes_for_zone_class(t_yoyo_zone_class zone_class) {
	switch (zone_class) {
		case YOYO_ZONE_TINY:
			return ZONE_TINY_BYTE;
		case YOYO_ZONE_SMALL:
			return ZONE_SMALL_BYTE;
		default:
			return 0;
	}
}


t_yoyo_zone_class	zone_class_for_bytes(size_t n) {
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
