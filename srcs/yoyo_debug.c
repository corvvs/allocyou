#include "yoyo_internal.h"

// zone の状態を出力する.
// ロックは必要なら取っておくこと.
void	print_zone_state(const t_yoyo_zone* zone) {
	DEBUGINFO(
		"ZONE %p: MT: %s, class: %d, blocks: zone %u, heap %u, free %u, used %u",
		zone, zone->multi_thread ? "Y" : "N",
		zone->zone_type, zone->blocks_zone, zone->blocks_heap, zone->blocks_free, zone->blocks_used
	);
}

// zone のビットマップの統計情報を出力する
// ロックは必要なら取っておくこと.
void	print_zone_bitmap_state(const t_yoyo_zone* zone) {
	unsigned int n_head = 0;
	unsigned int n_used = 0;
	for (unsigned int i = 0; i < zone->blocks_heap; ++i) {
		const bool h = is_head(zone, i);
		n_head += !!h;
		n_used += !!(h && is_used(zone, i));
	}
	DEBUGINFO(
		"ZONE %p: MT: %s, class: %d, is_head: %u, is_used: %u",
		zone, zone->multi_thread ? "Y" : "N", zone->zone_type,
		n_head, n_used
	);
}

// addr が malloc が返した領域であると仮定して, 情報を出力する
// ロックは必要なら取っておくこと.
void	print_memory_state(const void* addr) {
	const t_yoyo_chunk*	chunk = addr - CEILED_CHUNK_SIZE;
	if (IS_LARGE_CHUNK(chunk)) {
		// chunk is LARGE
		const t_yoyo_large_chunk* large_chunk = (void*)chunk - CEILED_LARGE_CHUNK_SIZE;
		DEBUGINFO(
			"chunk at %p is LARGE: %p, subarena: %p, total bytes: %zu B",
			addr, large_chunk, large_chunk->subarena, large_chunk->memory_byte
		);
	} else {
		// chunk is TINY / SMALL
		DEBUGINFO(
			"chunk at %p: %p, blocks: %zu",
			addr, chunk, chunk->blocks
		);
	}
}
