#include "yoyo_internal.h"

bool	init_realm(bool multi_thread) {
	if (g_yoyo_realm.initialized) {
		DEBUGWARN("%s", "skip: realm is already initialized.");
		return true;
	}
	g_yoyo_realm.arena_count = multi_thread ? ARENA_MAX : 1;
	for (unsigned int i = 0; i < g_yoyo_realm.arena_count; ++i) {
		const bool ok = init_arena(i, multi_thread);
		if (!ok) {
			for (unsigned int j = 0; j < i; ++j) {
				destroy_arena(&g_yoyo_realm.arenas[j]);
			}
			return false;
		}
	}
	g_yoyo_realm.initialized = true;
	DEBUGINFO("initialized realm: arena_count = %u, multi_thread: %s", g_yoyo_realm.arena_count, multi_thread ? "y" : "n");
	return true;
}
