#include "yoyo_internal.h"

// [arena の初期化に使う関数]

// arena を初期化する.
// multi_thread が true ならマルチスレッドモードとして初期化を行い, mutex を作成する.
// mutex の作成に失敗した場合は false を返す.
bool	init_arena(unsigned int index, bool multi_thread) {
	t_yoyo_arena* arena = &g_yoyo_realm.arenas[index];
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
	arena->index = index;
	arena->multi_thread = multi_thread;
	arena->initialized = true;
	arena->tiny.head = NULL;
	arena->tiny.multi_thread = multi_thread;
	arena->small.head = NULL;
	arena->small.multi_thread = multi_thread;
	arena->large.allocated = NULL;
	arena->large.multi_thread = multi_thread;
	DEBUGINFO("initialized arena: #%u(%p), multi_thread: %s", index, arena, multi_thread ? "y" : "n");
	return true;
}

// 初期化済みの arena を未初期化に戻す.
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

t_yoyo_subarena*	get_subarena(t_yoyo_arena* arena, t_yoyo_zone_type zone_type) {
	switch (zone_type) {
		case YOYO_ZONE_TINY:
			return (t_yoyo_subarena*)&arena->tiny;
		case YOYO_ZONE_SMALL:
			return (t_yoyo_subarena*)&arena->small;
		case YOYO_ZONE_LARGE:
			return (t_yoyo_subarena*)&arena->large;
	}
}
