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

// 一時ファイルにオンディスクログを書き込むための準備
static void	prepare_ondisk_log_temp(t_yoyo_debug* debug) {
	char	path[] = "/tmp/yoyo_malloc.log.XXXXXX";
	errno = 0;
	int fd = mkstemp(path);
	if (fd < 0) {
		DEBUGFATAL("failed to mkstemp: %d, %s", errno, strerror(errno));
		return;
	}
	debug->fd_ondisk_log = fd;
	yoyo_dprintf(STDERR_FILENO, "** taking on-disk log into %s **\n", path);
}

#include <fcntl.h>

// 通常ファイルにオンディスクログを書き込むための準備
static void	prepare_ondisk_log_file(t_yoyo_debug* debug, const char* path) {
	int fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
	if (fd < 0) {
		DEBUGFATAL("failed to open %s: %d, %s", path, errno, strerror(errno));
		return;
	}
	debug->fd_ondisk_log = fd;
	yoyo_dprintf(STDERR_FILENO, "** taking on-disk log into %s **\n", path);
}

void	init_debug(void) {
	t_yoyo_debug* debug = &g_yoyo_realm.debug;

	{ // YOYO_ENVKEY_SCRIBLLER
		char*	value = getenv(YOYO_ENVKEY_SCRIBLLER);
		if (value != NULL) {
			debug->scribbler = *value;
		} else {
			debug->scribbler = 0;
		}
	}

	{ // YOYO_ENVKEY_HISTORY
		char*	value = getenv(YOYO_ENVKEY_HISTORY);
		debug->take_history = value != NULL;
	}

	{ // YOYO_ENVKEY_HISTORY_LIMIT
		char*	value = getenv(YOYO_ENVKEY_HISTORY_LIMIT);
		debug->history_unlimited = (value != NULL && yo_strcmp(value, "none") == 0);
	}

	{ // YOYO_ENVKEY_SINGLE_THEAD
		char*	value = getenv(YOYO_ENVKEY_SINGLE_THEAD);
		debug->single_theard_mode = value != NULL;
	}

	{
		char*	value = getenv(YOYO_ENVKEY_HISTORY_ONDISK);
		debug->take_ondisk_log = value != NULL;
		if (debug->take_ondisk_log) {
			if (yo_strcmp(value, "") == 0) {
				// path が空文字列 -> 一時ファイル作成
				prepare_ondisk_log_temp(debug);
			} else {
				// path が空でない文字列 -> 通常ファイル
				prepare_ondisk_log_file(debug, value);
			}
		}
	}
}
