#include "yo_internal.h"

static double	get_fragmentation_rate(t_block_header *list)
{
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
	DEBUGOUT("total: %zu, free: %zu, in use: %zu, diff: %zd blocks",
			zone->cons.total_blocks, blocks_free, blocks_in_use, diff_blocks);
	DEBUGOUT("fragmentation: %1.4f%%", get_fragmentation_rate(zone->frees) * 100);
	// DEBUGSTRN("  free:      "); show_list(zone->frees);
	if (diff_blocks) {
		// show_alloc_mem();
		// DEBUGSTRN("  free:      "); show_list(zone->frees);
		DEBUGERR("consistency KO!!: zone %p", zone);
		assert(zone->cons.total_blocks == blocks_free + blocks_in_use);
	}
}

void	check_consistency(void) {
	if (g_root.tiny.frees) {
		DEBUGSTR("check consistency: TINY");
		check_zone_consistency(&g_root.tiny);
		DEBUGSTR("-> ok.");
	}
	if (g_root.small.frees) {
		DEBUGSTR("check consistency: SMALL");
		check_zone_consistency(&g_root.small);
		DEBUGSTR("-> ok.");
	}
}
