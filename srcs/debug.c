#include "internal.h"

// zone の状態を出力する.
// ロックは必要なら取っておくこと.
void	print_zone_state(const t_yoyo_zone* zone) {
	(void)zone;
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
	(void)n_head;
	(void)n_used;
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
		(void)large_chunk;
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

// g_yoyo_realm.debug.scribbler が0でない時, 
// チャンクのヘッダ以外を g_yoyo_realm.debug.scribbler で埋める.
// ただし complement が true の場合は g_yoyo_realm.debug.scribbler をビット反転する.
void	fill_chunk_by_scribbler(void* mem, bool complement) {
	if (!g_yoyo_realm.debug.scribbler) {
		return;
	}
	t_yoyo_chunk*	chunk = addr_to_actual_header(mem);
	assert(chunk != NULL);
	assert(chunk->blocks >= 2);
	unsigned char	filler = g_yoyo_realm.debug.scribbler;
	if (complement) {
		filler = ~filler;
	}
	yo_memset(mem, filler, (chunk->blocks - 1) * BLOCK_UNIT_SIZE);
}

void	init_debug(void) {
	t_yoyo_debug* debug = &g_yoyo_realm.debug;

	// YOYO_ENVKEY_SCRIBLLER: 確保/解放したチャンク(のヘッダ以外)を特定のバイトで埋める.
	{
		char*	value = getenv(YOYO_ENVKEY_SCRIBLLER);
		if (value != NULL) {
			debug->scribbler = *value;
		} else {
			debug->scribbler = 0;
		}
	}

	// YOYO_ENVKEY_HISTORY
	{
		char*	value = getenv(YOYO_ENVKEY_HISTORY);
		debug->take_history = value != NULL;
	}

	// YOYO_ENVKEY_HISTORY_LIMIT
	{
		char*	value = getenv(YOYO_ENVKEY_HISTORY_LIMIT);
		debug->history_unlimited = (value != NULL && yo_strcmp(value, "none") == 0);
	}

	// YOYO_ENVKEY_SINGLE_THEAD
	{
		char*	value = getenv(YOYO_ENVKEY_SINGLE_THEAD);
		debug->single_theard_mode = value != NULL;
	}
}
