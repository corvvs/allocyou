#include "yo_internal.h"

static void	show_zone(t_yo_zone *zone) {
	DEBUGSTRN("  allocated: "); show_list(zone->allocated);
	DEBUGSTRN("  free:      "); show_list(zone->frees);
}

void	actual_show_alloc_mem(void) {
	DEBUGSTR("TINY:");
	show_zone(&g_root.tiny);
	DEBUGSTR("SMALL:");
	show_zone(&g_root.small);
	DEBUGSTR("LARGE:  ");
	DEBUGSTRN("  used: "); show_list(g_root.large);
}

