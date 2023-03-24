#include "internal.h"

static void	release_entirely_free_zone(t_yoyo_normal_arena* subarena) {
	t_yoyo_zone**	current_lot = &(subarena->head);
	while (*current_lot != NULL) {
		t_yoyo_zone* zone = *current_lot;
		if (!lock_zone(zone)) {
			break;
		}
		const bool is_releasable = zone->blocks_free == zone->blocks_heap;
		if (!unlock_zone(zone)) {
			break;
		}
		if (is_releasable) {
			// 解放可能
			*current_lot = zone->next;
			YOYO_DPRINTF("RELEASE zone: %p\n", zone);
			yoyo_unmap_memory(zone, zone->blocks_zone * BLOCK_UNIT_SIZE);
			zone = *current_lot;
		} else {
			if (zone != NULL) {
				current_lot = &(zone->next);
			}
		}
	}
}

// 指定したサブアリーナについて未使用zoneを解放する
static bool release_zones_in_subarena(t_yoyo_normal_arena* subarena) {
		if (!lock_subarena((t_yoyo_subarena*)subarena)) {
			return false;
		}
		release_entirely_free_zone(subarena);
		if (!unlock_subarena((t_yoyo_subarena*)subarena)) {
			return false;
		}
		return true;
}

void	yoyo_actual_release_memory(void) {
	if (!g_yoyo_realm.initialized) {
		DEBUGERR("%s", "REALM IS NOT INITIALIZED");
		return;
	}
	// [各アリーナのTINY / SMALLサブアリーナについて, 未使用zoneを解放していく]
	for (unsigned int i = 0; i < g_yoyo_realm.arena_count; ++i) {
		t_yoyo_arena*			arena = &g_yoyo_realm.arenas[i];
		if (!release_zones_in_subarena(&arena->tiny)) {
			return;
		}
		if (!release_zones_in_subarena(&arena->small)) {
			return;
		}
	}
}
