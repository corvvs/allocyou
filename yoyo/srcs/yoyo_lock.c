#include "yoyo_internal.h"

// [ロックを取る/解除する]

bool	lock_arena(t_yoyo_arena* arena, t_yoyo_zone_class zone_class) {
	if (!arena->multi_thread) {
		DEBUGOUT("skip: in single-thread mode: #%u(%p)", arena->index, arena);
		return true;
	}
	t_yoyo_subarena*	subarena = get_subarena(arena, zone_class);
	if (pthread_mutex_lock(&subarena->lock)) {
		DEBUGERR("failed to lock: %d(%s)", errno, strerror(errno));
		return false;
	}
	DEBUGOUT("locked: #%u(%p)", arena->index, arena);
	return true;
}

bool	try_lock_arena(t_yoyo_arena* arena, t_yoyo_zone_class zone_class) {
	if (!arena->multi_thread) {
		DEBUGOUT("skip: in single-thread mode: #%u(%p)", arena->index, arena);
		return true;
	}
	t_yoyo_subarena*	subarena = get_subarena(arena, zone_class);
	if (pthread_mutex_trylock(&subarena->lock)) {
		DEBUGERR("failed to try lock: %d(%s)", errno, strerror(errno));
		return false;
	}
	DEBUGOUT("locked: #%u(%p)", arena->index, arena);
	return true;
}

bool	lock_zone(t_yoyo_zone* zone) {
	if (!zone->multi_thread) {
		DEBUGOUT("skip: in single-thread mode: %p", zone);
		return true;
	}
	if (pthread_mutex_lock(&zone->lock)) {
		DEBUGERR("failed to lock: %d(%s)", errno, strerror(errno));
		return false;
	}
	DEBUGOUT("locked: %p", zone);
	return true;
}

bool	unlock_arena(t_yoyo_arena* arena, t_yoyo_zone_class zone_class) {
	if (!arena->multi_thread) {
		DEBUGOUT("skip: in single-thread mode: #%u(%p)", arena->index, arena);
		return true;
	}
	t_yoyo_subarena*	subarena = get_subarena(arena, zone_class);
	if (pthread_mutex_unlock(&subarena->lock)) {
		DEBUGERR("failed to unlock: %d(%s)", errno, strerror(errno));
		return false;
	}
	DEBUGOUT("unlocked: #%u(%p)", arena->index, arena);
	return true;	
}

bool	unlock_zone(t_yoyo_zone* zone) {
	if (!zone->multi_thread) {
		DEBUGOUT("skip: in single-thread mode: %p", zone);
		return true;
	}
	if (pthread_mutex_unlock(&zone->lock)) {
		DEBUGERR("failed to unlock: %d(%s)", errno, strerror(errno));
		return false;
	}
	DEBUGOUT("unlocked: %p", zone);
	return true;
}
