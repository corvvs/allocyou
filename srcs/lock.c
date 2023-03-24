#include "internal.h"

// [ロックを取る/解除する]

bool	lock_arena(t_yoyo_arena* arena, t_yoyo_zone_type zone_type) {
	if (!arena->multi_thread) {
		DEBUGOUT("SKIP: in single-thread mode: #%u(%p)", arena->index, arena);
		return true;
	}
	t_yoyo_subarena*	subarena = get_subarena(arena, zone_type);
	DEBUGOUT("LOCKING: #%u(%p)", arena->index, arena);
	if (!lock_subarena(subarena)) {
		DEBUGFATAL("FAILED to lock: %d(%s)", errno, strerror(errno));
		return false;
	}
	DEBUGOUT("LOCKED: #%u(%p)", arena->index, arena);
	return true;
}

bool	try_lock_arena(t_yoyo_arena* arena, t_yoyo_zone_type zone_type) {
	if (!arena->multi_thread) {
		DEBUGOUT("SKIP: in single-thread mode: #%u(%p)", arena->index, arena);
		return true;
	}
	t_yoyo_subarena*	subarena = get_subarena(arena, zone_type);
	if (!try_lock_subarena(subarena)) {
		// DEBUGWARN("FAILED to try lock: %d(%s)", errno, strerror(errno));
		return false;
	}
	DEBUGOUT("LOCKED: #%u(%p)", arena->index, arena);
	return true;
}

bool	lock_subarena(t_yoyo_subarena* subarena) {
	if (!subarena->multi_thread) {
		// DEBUGOUT("SKIP: in single-thread mode: %p", subarena);
		return true;
	}
	if (pthread_mutex_lock(&subarena->lock)) {
		DEBUGFATAL("FAILED to lock: %d(%s)", errno, strerror(errno));
		return false;
	}
	DEBUGOUT("LOCKED: %p", subarena);
	return true;
}

bool	try_lock_subarena(t_yoyo_subarena* subarena) {
	if (!subarena->multi_thread) {
		// DEBUGOUT("SKIP: in single-thread mode: %p", subarena);
		return true;
	}
	if (pthread_mutex_trylock(&subarena->lock)) {
		// DEBUGWARN("FAILED to try lock: %d(%s)", errno, strerror(errno));
		return false;
	}
	DEBUGOUT("LOCKED: %p", subarena);
	return true;
}

bool	lock_zone(t_yoyo_zone* zone) {
	if (!zone->multi_thread) {
		// DEBUGOUT("SKIP: in single-thread mode: %p", zone);
		return true;
	}
	if (pthread_mutex_lock(&zone->lock)) {
		DEBUGFATAL("FAILED to lock: %d(%s)", errno, strerror(errno));
		return false;
	}
	// DEBUGOUT("LOCKED: %p", zone);
	return true;
}

bool	try_lock_zone(t_yoyo_zone* zone) {
	if (!zone->multi_thread) {
		// DEBUGOUT("SKIP: in single-thread mode: %p", zone);
		return true;
	}
	if (pthread_mutex_trylock(&zone->lock)) {
		// DEBUGWARN("FAILED to try lock: %d(%s)", errno, strerror(errno));
		return false;
	}
	// DEBUGOUT("LOCKED: %p", zone);
	return true;
}

bool	unlock_arena(t_yoyo_arena* arena, t_yoyo_zone_type zone_type) {
	if (!arena->multi_thread) {
		// DEBUGOUT("SKIP: in single-thread mode: #%u(%p)", arena->index, arena);
		return true;
	}
	t_yoyo_subarena*	subarena = get_subarena(arena, zone_type);
	DEBUGOUT("UNLOCKING: #%u(%p)", arena->index, arena);
	if (pthread_mutex_unlock(&subarena->lock)) {
		DEBUGFATAL("FAILED to unlock: %d(%s)", errno, strerror(errno));
		return false;
	}
	DEBUGOUT("UNLOCKED: #%u(%p)", arena->index, arena);
	return true;	
}

bool	unlock_subarena(t_yoyo_subarena* subarena) {
	if (!subarena->multi_thread) {
		DEBUGOUT("SKIP: in single-thread mode: %p", subarena);
		return true;
	}
	if (pthread_mutex_unlock(&subarena->lock)) {
		DEBUGFATAL("FAILED to unlock: %d(%s)", errno, strerror(errno));
		return false;
	}
	DEBUGOUT("UNLOCKED: %p", subarena);
	return true;
}

bool	unlock_zone(t_yoyo_zone* zone) {
	if (!zone->multi_thread) {
		DEBUGOUT("SKIP: in single-thread mode: %p", zone);
		return true;
	}
	if (pthread_mutex_unlock(&zone->lock)) {
		DEBUGFATAL("FAILED to unlock: %d(%s)", errno, strerror(errno));
		return false;
	}
	DEBUGOUT("UNLOCKED: %p", zone);
	return true;
}
