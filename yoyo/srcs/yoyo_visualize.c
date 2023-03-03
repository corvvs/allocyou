#include "yoyo_internal.h"

extern t_yoyo_realm	g_yoyo_realm;

static void	visualize_chunk(const t_yoyo_chunk* chunk) {
	printf("\t\tchunk @ %p: %zu blocks (%zu B)\n", chunk, chunk->blocks, chunk->blocks * BLOCK_UNIT_SIZE);
}

// 指定した zone を可視化する
static void	visualize_locked_zone(t_yoyo_zone* zone) {
	printf("\t\t[zone: %p]\n", zone);
	printf("\t\tusing %u blocks / %u blocks\n", zone->blocks_used, zone->blocks_zone);
	// 使用中チャンクを表示していく
	unsigned int	block_index = 0;
	while (block_index < zone->blocks_heap) {
		t_yoyo_chunk*	chunk = get_chunk_by_index(zone, block_index);
		if (chunk == NULL) { break; }
		if (is_used(zone, block_index)) {
			// 表示
			visualize_chunk(chunk);
		}
		assert(2 <= chunk->blocks);
		block_index += chunk->blocks;
	}
}

// TINY / SMALL サブアリーナを, 配下の zone を順次ロックしながら可視化していく.
static void	visualize_locked_tiny_small_subarena(const char* zone_name, t_yoyo_arena* arena, t_yoyo_zone_type zone_type) {
	assert(zone_type == YOYO_ZONE_TINY || zone_type == YOYO_ZONE_SMALL);
	t_yoyo_normal_arena* subarena = zone_type == YOYO_ZONE_TINY ? &arena->tiny : &arena->small;
	if (subarena->head == NULL) {
		return;
	}
	printf("\t< %s >\n", zone_name);
	t_yoyo_zone*	zone = subarena->head;
	size_t	n_zone = 0;
	while (zone != NULL) {
		if (!lock_zone(zone)) {
			return;
		}
		visualize_locked_zone(zone);
		t_yoyo_zone*	next = zone->next;
		if (!unlock_zone(zone)) {
			return;
		}
		zone = next;
		n_zone += 1;
	}
	printf("\t%zu zones in this subarena\n", n_zone);
}

static void	visualize_large_chunk(const t_yoyo_large_chunk* large_chunk) {
	const t_yoyo_chunk* chunk = (void*)large_chunk + CEILED_LARGE_CHUNK_SIZE;
	printf(
		"\tlarge chunk @ %p: %zu B (usable: %zu B)\n",
		large_chunk, large_chunk->memory_byte, chunk->blocks * BLOCK_UNIT_SIZE - CEILED_CHUNK_SIZE
	);
}

// LARGE サブアリーナを可視化する
static void	visualize_locked_large_subarena(const char* zone_name, t_yoyo_arena* arena, t_yoyo_zone_type zone_type) {
	assert(zone_type == YOYO_ZONE_LARGE);
	t_yoyo_large_arena* subarena = &arena->large;
	if (subarena->allocated == NULL) {
		return;
	}
	printf("\t< %s >\n", zone_name);
	t_yoyo_large_chunk*	large_chunk = subarena->allocated;
	size_t	n_large_chunk = 0;
	while (large_chunk != NULL) {
		visualize_large_chunk(large_chunk);
		large_chunk = large_chunk->large_next;
		n_large_chunk += 1;
	}
	printf("\t%zu large chunk in this subarena\n", n_large_chunk);
}

// 指定した subarena を可視化する
static void	visualize_subarena(
	const char* zone_name,
	t_yoyo_arena* arena, t_yoyo_zone_type zone_type,
	void (visualizer)(const char* zone_name, t_yoyo_arena*, t_yoyo_zone_type)
) {
	if (!lock_arena(arena, zone_type)) {
		return;
	}
	visualizer(zone_name, arena, zone_type);
	if (!unlock_arena(arena, zone_type)) {
		return;
	}
}

// 指定した arena を可視化する
static void	visualize_arena(t_yoyo_arena* arena) {
	printf("[arena #%u (%s)]\n", arena->index, arena->multi_thread ? "multi-thread mode" : "single-thread mode");
	visualize_subarena("TINY", arena, YOYO_ZONE_TINY, visualize_locked_tiny_small_subarena);
	visualize_subarena("SMALL", arena, YOYO_ZONE_SMALL, visualize_locked_tiny_small_subarena);
	visualize_subarena("LARGE", arena, YOYO_ZONE_LARGE, visualize_locked_large_subarena);
}

void	actual_show_alloc_mem(void) {
	if (!g_yoyo_realm.initialized) {
		DEBUGERR("%s", "REALM IS NOT INITIALIZED");
		return;
	}
	for (unsigned int i = 0; i < g_yoyo_realm.arena_count; ++i) {
		visualize_arena(&g_yoyo_realm.arenas[i]);
	}
}
