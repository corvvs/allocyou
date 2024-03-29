#include "internal.h"

t_yoyo_realm	g_yoyo_realm;
// __thread int	yoyo_thread_id;
// __thread int	yoyo_invokation_id;
// __thread const char*	yoyo_invokation_name;

// n個までの arena を破棄する
static void	destroy_n_arenas(unsigned int n) {
	for (unsigned int i = 0; i < n; ++i) {
		destroy_arena(&g_yoyo_realm.arenas[i]);
	}
}

// realm を初期化する.
// スタートアップルーチンから実行される前提.
bool	init_realm(void) {

	static pthread_mutex_t	init_mutex = PTHREAD_MUTEX_INITIALIZER;

	if (pthread_mutex_lock(&init_mutex)) {
		DEBUGFATAL("FAILED to lock for init: %d", errno);
		return false;
	}

	if (g_yoyo_realm.initialized) {
		DEBUGWARN("%s", "skip: realm is already initialized.");
		pthread_mutex_unlock(&init_mutex); // pthread_mutex_lock が通ったのなら, unlock は失敗しないはず
		return true;
	}

	// [BONUS: デバッグパラメータ管理構造体の初期化]
	init_debug();
	const bool multi_thread = !g_yoyo_realm.debug.single_theard_mode;

	// [アリーナの初期化]
	g_yoyo_realm.arena_count = multi_thread ? ARENA_MAX : 1;
	for (unsigned int i = 0; i < g_yoyo_realm.arena_count; ++i) {
		const bool succeeded = init_arena(i, multi_thread);
		if (!succeeded) {
			DEBUGFATAL("FAILED to init arena #%u", i);
			destroy_n_arenas(i);
			pthread_mutex_unlock(&init_mutex);
			return false;
		}
	}

	// [BONUS: 履歴管理構造体の初期化]
	if (!init_history(multi_thread)) {
		pthread_mutex_unlock(&init_mutex);
		return false;
	}

	g_yoyo_realm.initialized = true;
	DEBUGINFO("initialized realm: arena_count = %u, multi_thread: %s", g_yoyo_realm.arena_count, multi_thread ? "y" : "n");
	pthread_mutex_unlock(&init_mutex); // pthread_mutex_lock が通ったのなら, unlock は失敗しないはず
	return true;
}
