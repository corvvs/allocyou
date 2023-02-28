#include "yo_internal.h"

// 整合性チェック

extern t_yo_malloc_root	g_root;

static double	get_fragmentation_rate(t_block_header *list) {
	size_t	blocks = 0;
	size_t	headers = 0;
	while (list != NULL) {
		blocks += list->blocks;
		headers += 1;
		list = list_next_head(list);
	}
	return blocks > 0 ? (double)headers / (double)blocks : 0;
}

static void	check_zone_consistency(t_yo_zone *zone) {
	t_block_header	*h;
	size_t blocks_in_use = 0;
	size_t blocks_free = 0;
	h = zone->frees;
	while (h) {
		// DEBUGOUT("(%zu, %p, %p)", h->blocks, h, h->next);
		assert(h->blocks > 0);
		blocks_free += h->blocks + 1;
		h = list_next_head(h);
	}
	h = zone->allocated;
	while (h) {
		assert(h->blocks > 0);
		blocks_in_use += h->blocks + 1;
		h = list_next_head(h);
	}
	const ssize_t diff_blocks = zone->cons.total_blocks - (blocks_free + blocks_in_use);
	DEBUGOUT("total: %zu(%zukB), free: %zu(%zukB), in use: %zu(%zukB), diff: %zd blocks",
			zone->cons.total_blocks, zone->cons.total_blocks * BLOCK_UNIT_SIZE / 1024,
			blocks_free, blocks_free * BLOCK_UNIT_SIZE / 1024,
			blocks_in_use, blocks_in_use * BLOCK_UNIT_SIZE / 1024,
			diff_blocks);
	(void)get_fragmentation_rate;
	DEBUGOUT("fragmentation: %1.4f%%", get_fragmentation_rate(zone->frees) * 100);
	if (diff_blocks) {
		DEBUGERR("consistency KO!!: zone %p", zone);
		assert(zone->cons.total_blocks == blocks_free + blocks_in_use);
	}
	if (zone->cons.free_blocks != blocks_free) {
		DEBUGERR("consistency KO!!: zone %p", zone);
		assert(zone->cons.free_blocks == blocks_free);
	}
	if (zone->cons.used_blocks != blocks_in_use) {
		DEBUGERR("consistency KO!!: zone %p", zone);
		assert(zone->cons.used_blocks == blocks_in_use);
	}
}

void	check_consistency(void) {
	if (g_root.tiny.frees != NULL) {
		DEBUGSTR("check consistency: TINY");
		check_zone_consistency(&g_root.tiny);
		DEBUGSTR("-> ok.");
	}
	if (g_root.small.frees != NULL) {
		DEBUGSTR("check consistency: SMALL");
		check_zone_consistency(&g_root.small);
		DEBUGSTR("-> ok.");
	}
}
