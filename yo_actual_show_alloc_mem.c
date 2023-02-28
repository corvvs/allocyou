#include "yo_internal.h"

static void	show_zone(t_yo_zone *zone) {
	if (zone->zone_class == YO_ZONE_LARGE) {
		DEBUGSTRN("\tallocated: "); show_list(zone->allocated);
		DEBUGOUT( "\t           %zu blocks (%zu kB)", zone->cons.used_blocks, BLOCKS_TO_KB(zone->cons.used_blocks));
		DEBUGOUT( "\tfree:      %zu blocks (%zu kB)", zone->cons.free_blocks, BLOCKS_TO_KB(zone->cons.free_blocks));
		DEBUGOUT( "\ttotal:     %zu blocks (%zu kB)", zone->cons.total_blocks, BLOCKS_TO_KB(zone->cons.total_blocks));
	} else {
		DEBUGSTRN("\tallocated: "); show_list(zone->allocated);
		DEBUGOUT( "\t           %zu blocks (%zu kB)", zone->cons.used_blocks, BLOCKS_TO_KB(zone->cons.used_blocks));
		DEBUGSTRN("\tfree:      "); show_list(zone->frees);
		DEBUGOUT( "\t           %zu blocks (%zu kB)", zone->cons.free_blocks, BLOCKS_TO_KB(zone->cons.free_blocks));
		DEBUGOUT( "\ttotal:     %zu blocks (%zu kB)", zone->cons.total_blocks, BLOCKS_TO_KB(zone->cons.total_blocks));
	}
}

void	actual_show_alloc_mem(void) {
	g_root.tiny.zone_class = YO_ZONE_TINY;
	g_root.small.zone_class = YO_ZONE_SMALL;
	g_root.large.zone_class = YO_ZONE_LARGE;
	DEBUGSTR("TINY:");
	show_zone(&g_root.tiny);
	DEBUGSTR("SMALL:");
	show_zone(&g_root.small);
	DEBUGSTR("LARGE:  ");
	show_zone(&g_root.large);
}

