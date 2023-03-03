#include "yoyo_internal.h"

// [zone の初期化時に使う関数]

// zone 全体のバイト数から heap のバイト数を計算する.
static size_t	heap_bytes_for_zone_bytes(size_t zone_bytes) {
	const size_t d = 4 * (zone_bytes - sizeof(t_yoyo_zone));
	const size_t n = 4 * BLOCK_UNIT_SIZE + 1;
	const size_t h = FLOOR_BY(d / n, 8);
	DEBUGOUT("d = %zu, n = %zu, h = %zu", d, n, h);
	return h * BLOCK_UNIT_SIZE;
}

// ビットマップフィールドのバイト数を計算する.
static size_t	bitmap_bytes_for_zone_bytes(size_t zone_bytes) {
	const size_t h = heap_bytes_for_zone_bytes(zone_bytes);
	return h / BLOCK_UNIT_SIZE / 8;
}

// zone を初期化する.
// 失敗した場合は false を返す.
static bool	init_zone(const t_yoyo_arena* arena, t_yoyo_zone* zone, t_yoyo_zone_type zone_type) {
	if (arena->multi_thread) {
		if (pthread_mutex_init(&zone->lock, NULL)) {
			DEBUGERR("errno: %d (%s)", errno, strerror(errno));
			return false;
		}
		DEBUGOUT("%s", "@multi_thread");
	}
	zone->multi_thread = arena->multi_thread;
	zone->zone_type = zone_type;
	const size_t zone_bytes = zone_bytes_for_zone_type(zone_type);
	const size_t heap_bytes = heap_bytes_for_zone_bytes(zone_bytes);
	const size_t bitmap_bytes = bitmap_bytes_for_zone_bytes(zone_bytes);
	DEBUGOUT("heap: %zu B bitmap: %zu B", heap_bytes, bitmap_bytes);
	zone->next = NULL;
	zone->frees = NULL;
	zone->free_prev = NULL;
	zone->blocks_zone = zone_bytes / BLOCK_UNIT_SIZE;
	zone->blocks_heap = heap_bytes / BLOCK_UNIT_SIZE;
	DEBUGOUT("blocks_zone: %u", zone->blocks_zone);
	DEBUGOUT("blocks_heap: %u", zone->blocks_heap);
	zone->blocks_free = zone->blocks_heap;
	zone->blocks_used = 0;
	zone->offset_bitmap_heads = sizeof(t_yoyo_zone);
	zone->offset_bitmap_used = zone->offset_bitmap_heads + bitmap_bytes;
	zone->offset_heap = zone_bytes - heap_bytes;
	DEBUGOUT("offset_bitmap_heads: %u B", zone->offset_bitmap_heads);
	DEBUGOUT("offset_bitmap_used:  %u B", zone->offset_bitmap_used);
	DEBUGOUT("offset_heap:         %u B", zone->offset_heap);
	t_yoyo_chunk* head = (void*)zone + zone->offset_heap;
	mark_chunk_as_free(zone, head);
	head->blocks = zone->blocks_free;
	head->next = SET_FLAGS(NULL, 0);
	zone->frees = head;
	return true;
}

// TINY / SMALL zone を mmap で確保し, 初期化して返す.
t_yoyo_zone*	allocate_zone(const t_yoyo_arena* arena, t_yoyo_zone_type zone_type) {
	const size_t zone_bytes = zone_bytes_for_zone_type(zone_type);
	t_yoyo_zone* zone = map_memory(zone_bytes);
	if (zone == NULL) {
		DEBUGERR("failed for class: %d", zone_type);
		return NULL;
	}
	DEBUGOUT("ALLOCATED %zu bytes region at %p", zone_bytes, zone);
	if (!init_zone(arena, zone, zone_type)) {
		unmap_memory(zone, zone_bytes);
		return NULL;
	}
	return zone;
}
