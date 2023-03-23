#include "internal.h"

// チャンクの先頭1ブロック分を16進ダンプする.
// !! 前提: チャンクが適切に排他制御されている !!
static void	dump_chunk_body(const t_yoyo_chunk* chunk) {
	// [in hex]
	// すべてのチャンクは少なくとも1ブロック分の使用可能領域を持つので, 1ブロック分は必ずダンプできる.
	if (g_yoyo_realm.debug.xd_blocks < 1) {
		return;
	}
	assert(chunk->blocks >= 2);
	const size_t			xd_bytes = g_yoyo_realm.debug.xd_blocks * BLOCK_UNIT_SIZE;
	const size_t			chunk_bytes = (chunk->blocks - 1) * BLOCK_UNIT_SIZE;
	const size_t			max_bytes = xd_bytes < chunk_bytes ? xd_bytes : chunk_bytes;
	const unsigned char*	body = (void*)chunk + CEILED_CHUNK_SIZE;
	size_t					i_ascii = 0;
	for (size_t i = 0; i < max_bytes; ++i) {
		unsigned char ch = body[i];
		if (i % 16 == 0) {
			yoyo_dprintf(STDOUT_FILENO, "\t\txd: ");
		} else {
			yoyo_dprintf(STDOUT_FILENO, i % (BLOCK_UNIT_SIZE / 2) == 0 ? "  " : " ");
		}
		// [in HEX]
		yoyo_dprintf(STDOUT_FILENO, "%x", ch / 16);
		yoyo_dprintf(STDOUT_FILENO, "%x", ch % 16);
		if (!((i + 1) % 16 == 0 || i + 1 == max_bytes)) {
			continue;
		}
		// [in ASCII]
		yoyo_dprintf(STDOUT_FILENO, "  |");
		for (; i_ascii <= i; ++i_ascii) {
			unsigned char ch = body[i_ascii];
			yoyo_dprintf(STDOUT_FILENO, "%c", yo_isprint(ch) ? ch : '.');
		}
		yoyo_dprintf(STDOUT_FILENO, "|\n");
	}
}

// チャンク情報を表示する
static void	visualize_chunk(const t_yoyo_chunk* chunk, bool exec_hexdump) {
	yoyo_dprintf(STDOUT_FILENO, "\t\tchunk @ %p: %zu blocks (%zu B)\n",
		chunk, chunk->blocks, chunk->blocks * BLOCK_UNIT_SIZE);
	if (exec_hexdump) {
		dump_chunk_body(chunk);
	}
}

// 指定した zone を可視化する
static void	visualize_locked_zone(t_yoyo_zone* zone, bool exec_hexdump) {
	// [zone 情報を表示]
	yoyo_dprintf(STDOUT_FILENO, "\t\tzone %p:", zone);
	if (zone->blocks_used > 0) {
		yoyo_dprintf(STDOUT_FILENO, " using %u blocks / %u blocks\n", zone->blocks_used, zone->blocks_zone);
	} else {
		yoyo_dprintf(STDOUT_FILENO, " %u blocks ALL FREE\n", zone->blocks_zone);
	}
	// [使用中チャンク情報を表示]
	unsigned int	block_index = 0;
	while (block_index < zone->blocks_heap) {
		t_yoyo_chunk*	chunk = get_chunk_by_index(zone, block_index);
		if (chunk == NULL) { break; }
		if (is_used(zone, block_index)) {
			visualize_chunk(chunk, exec_hexdump);
		}
		assert(2 <= chunk->blocks);
		block_index += chunk->blocks;
	}
}

// TINY / SMALL サブアリーナを, 配下の zone を順次ロックしながら可視化していく.
static void	visualize_locked_tiny_small_subarena(
	const char* zone_name,
	t_yoyo_arena* arena,
	t_yoyo_zone_type zone_type,
	bool exec_hexdump
) {
	assert(zone_type == YOYO_ZONE_TINY || zone_type == YOYO_ZONE_SMALL);
	t_yoyo_normal_arena* subarena = zone_type == YOYO_ZONE_TINY ? &arena->tiny : &arena->small;
	if (subarena->head == NULL) {
		return;
	}
	// zone リスト(subarena->head)をアドレスの昇順にソートする
	sort_zone_list(&subarena->head);

	yoyo_dprintf(STDOUT_FILENO, "\t< %s >\n", zone_name);
	t_yoyo_zone*	zone = subarena->head;
	size_t	n_zone = 0;
	while (zone != NULL) {
		if (!lock_zone(zone)) {
			return; // 致命的
		}
		visualize_locked_zone(zone, exec_hexdump);
		t_yoyo_zone*	next = zone->next;
		if (!unlock_zone(zone)) {
			return; // 致命的
		}
		zone = next;
		n_zone += 1;
	}
	yoyo_dprintf(STDOUT_FILENO, "\t%zu %s in this subarena\n",
		n_zone, n_zone > 1 ? "zones" : "zone");
}

static void	visualize_large_chunk(const t_yoyo_large_chunk* large_chunk, bool exec_hexdump) {
	const t_yoyo_chunk* chunk = (void*)large_chunk + CEILED_LARGE_CHUNK_SIZE;
	yoyo_dprintf(STDOUT_FILENO, 
		"\tlarge chunk @ %p: %zu B (usable: %zu B)\n",
		large_chunk, large_chunk->memory_byte, chunk->blocks * BLOCK_UNIT_SIZE - CEILED_CHUNK_SIZE
	);
	if (exec_hexdump) {
		dump_chunk_body(chunk);
	}
}

// LARGE サブアリーナを可視化する
static void	visualize_locked_large_subarena(
	const char* zone_name,
	t_yoyo_arena* arena,
	t_yoyo_zone_type zone_type,
	bool exec_hexdump
) {
	(void)zone_type;
	assert(zone_type == YOYO_ZONE_LARGE);
	t_yoyo_large_arena* subarena = &arena->large;
	if (subarena->allocated == NULL) {
		return;
	}
	yoyo_dprintf(STDOUT_FILENO, "\t< %s >\n", zone_name);
	t_yoyo_large_chunk*	large_chunk = subarena->allocated;
	size_t	n_large_chunk = 0;
	while (large_chunk != NULL) {
		visualize_large_chunk(large_chunk, exec_hexdump);
		large_chunk = large_chunk->large_next;
		n_large_chunk += 1;
	}
	yoyo_dprintf(STDOUT_FILENO, "\t%zu large chunk in this subarena\n", n_large_chunk);
}

// 指定した subarena を可視化する
static void	visualize_subarena(
	const char* zone_name,
	t_yoyo_arena* arena,
	t_yoyo_zone_type zone_type,
	void (visualizer)(const char* zone_name, t_yoyo_arena*, t_yoyo_zone_type, bool),
	bool exec_hexdump
) {
	if (!lock_arena(arena, zone_type)) {
		return;
	}
	visualizer(zone_name, arena, zone_type, exec_hexdump);
	if (!unlock_arena(arena, zone_type)) {
		return;
	}
}

// 指定した arena を可視化する
static void	visualize_arena(t_yoyo_arena* arena, bool exec_hexdump) {
	yoyo_dprintf(STDOUT_FILENO, "[arena #%u (%s mode)]\n", arena->index, arena->multi_thread ? "multi-thread" : "single-thread");
	visualize_subarena("TINY", arena, YOYO_ZONE_TINY, visualize_locked_tiny_small_subarena, exec_hexdump);
	visualize_subarena("SMALL", arena, YOYO_ZONE_SMALL, visualize_locked_tiny_small_subarena, exec_hexdump);
	visualize_subarena("LARGE", arena, YOYO_ZONE_LARGE, visualize_locked_large_subarena, exec_hexdump);
}

void	actual_show_alloc_mem(void) {
	if (!g_yoyo_realm.initialized) {
		DEBUGERR("%s", "REALM IS NOT INITIALIZED");
		return;
	}
	for (unsigned int i = 0; i < g_yoyo_realm.arena_count; ++i) {
		visualize_arena(&g_yoyo_realm.arenas[i], false);
	}
}

void	actual_show_alloc_mem_ex(void) {
	if (!g_yoyo_realm.initialized) {
		DEBUGERR("%s", "REALM IS NOT INITIALIZED");
		return;
	}
	// [履歴の表示]
	show_history();

	// [ダンプ付きメモリステータスの表示]
	for (unsigned int i = 0; i < g_yoyo_realm.arena_count; ++i) {
		visualize_arena(&g_yoyo_realm.arenas[i], true);
	}
}
