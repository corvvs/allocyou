#include "yoyo_internal.h"

// [arena の初期化に使う関数]

// arena を初期化する.
// multi_thread が true ならマルチスレッドモードとして初期化を行い, mutex を作成する.
// mutex の作成に失敗した場合は false を返す.
bool	init_arena(t_yoyo_arena* arena, bool multi_thread) {
	if (multi_thread) {
		if (pthread_mutex_init(&arena->tiny.lock, NULL)) {
			DEBUGERR("failed to init tiny lock: errno: %d (%s)", errno, strerror(errno));
			return false;
		}
		if (pthread_mutex_init(&arena->small.lock, NULL)) {
			DEBUGERR("failed to init small lock: errno: %d (%s)", errno, strerror(errno));
			pthread_mutex_destroy(&arena->tiny.lock);
			return false;
		}
		if (pthread_mutex_init(&arena->large.lock, NULL)) {
			DEBUGERR("failed to init large lock: errno: %d (%s)", errno, strerror(errno));
			pthread_mutex_destroy(&arena->tiny.lock);
			pthread_mutex_destroy(&arena->small.lock);
			return false;
		}
	}
	arena->multi_thread = multi_thread;
	arena->initialized = true;
	arena->tiny.head = NULL;
	arena->small.head = NULL;
	arena->large.head = NULL;
	DEBUGINFO("initialized arena: %p, multi_thread: %s", arena, multi_thread ? "y" : "n");
	return true;
}

// 初期化済みの arena を実初期化に戻す.
// 普通は使わないはず.
void	destroy_arena(t_yoyo_arena* arena) {
	if (!arena->initialized) {
		DEBUGWARN("skip: arena %p is not intialized", arena);
		return;
	}
	if (arena->multi_thread) {
		pthread_mutex_destroy(&arena->tiny.lock);
		pthread_mutex_destroy(&arena->small.lock);
		pthread_mutex_destroy(&arena->tiny.lock);
	}
	arena->initialized = false;
}

