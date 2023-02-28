#include "yoyo_internal.h"

// [zone の初期化時に使う関数]

// zone 全体のバイト数から heap のバイト数を計算する.
size_t	heap_bytes_for_zone_bytes(size_t zone_bytes) {
	const size_t d = 4 * (zone_bytes - sizeof(t_yoyo_zone));
	const size_t n = 4 * BLOCK_UNIT_SIZE + 1;
	const size_t h = FLOOR_BY(d / n, 8);
	printf("d = %zu, n = %zu, h = %zu\n", d, n, h);
	return h * BLOCK_UNIT_SIZE;
}

// ビットマップフィールドのバイト数を計算する.
size_t	bitmap_bytes_for_zone_bytes(size_t zone_bytes) {
	const size_t h = heap_bytes_for_zone_bytes(zone_bytes);
	return h / BLOCK_UNIT_SIZE / 8;
}

// zone を初期化する.
// 失敗した場合は false を返す.
bool	init_zone(t_yoyo_arena* arena, t_yoyo_zone* zone, t_yoyo_zone_class zone_class) {
	if (arena->multi_thread) {
		if (pthread_mutex_init(&zone->lock, NULL)) {
			DEBUGERR("errno: %d (%s)", errno, strerror(errno));
			return false;
		}
	}
	zone->multi_thread = arena->multi_thread;
	const size_t zone_bytes = zone_bytes_for_zone_class(zone_class);
	const size_t heap_bytes = heap_bytes_for_zone_bytes(zone_bytes);
	const size_t bitmap_bytes = bitmap_bytes_for_zone_bytes(zone_bytes);
	zone->next = NULL;
	zone->frees = NULL;
	zone->free_prev = NULL;
	zone->blocks_zone = zone_bytes / BLOCK_UNIT_SIZE;
	zone->blocks_heap = heap_bytes / BLOCK_UNIT_SIZE;
	zone->blocks_free = zone->blocks_heap;
	zone->blocks_used = 0;
	zone->offset_bitmap_heads = sizeof(t_yoyo_zone);
	zone->offset_bitmap_used = zone->offset_bitmap_heads + bitmap_bytes;
	zone->offset_heap = zone_bytes - heap_bytes;
	return true;
}
