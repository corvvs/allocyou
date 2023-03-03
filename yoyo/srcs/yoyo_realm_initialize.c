#include "yoyo_internal.h"

// n個までの arena を破棄する
static void	destroy_n_arenas(unsigned int n) {
	for (unsigned int i = 0; i < n; ++i) {
		destroy_arena(&g_yoyo_realm.arenas[i]);
	}
}

// realm を初期化する.
// スタートアップルーチンから実行される前提.
bool	init_realm(bool multi_thread) {
	if (g_yoyo_realm.initialized) {
		DEBUGWARN("%s", "skip: realm is already initialized.");
		return true;
	}
	g_yoyo_realm.arena_count = multi_thread ? ARENA_MAX : 1;
	for (unsigned int i = 0; i < g_yoyo_realm.arena_count; ++i) {
		const bool succeeded = init_arena(i, multi_thread);
		if (!succeeded) {
			DEBUGERR("FAILED to init arena #%u", i);
			destroy_n_arenas(i);
			return false;
		}
	}
	g_yoyo_realm.initialized = true;
	DEBUGINFO("initialized realm: arena_count = %u, multi_thread: %s", g_yoyo_realm.arena_count, multi_thread ? "y" : "n");
	return true;
}
