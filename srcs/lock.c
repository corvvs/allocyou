#include "internal.h"

// [ロックを取る/解除する]

bool	lock_arena(t_yoyo_arena* arena, t_yoyo_zone_type zone_type) {
	if (!arena->multi_thread) {
		DEBUGOUT("SKIP: in single-thread mode: #%u(%p)", arena->index, arena);
		return true;
	}
	t_yoyo_subarena*	subarena = get_subarena(arena, zone_type);
	DEBUGOUT("TRY LOCKING arena: #%u(%p)", arena->index, arena);
	if (!lock_subarena(subarena)) {
		DEBUGFATAL("FAILED to lock: %d(%s)", errno, strerror(errno));
		return false;
	}
	DEBUGOUT("TRY LOCKED arena: #%u(%p)", arena->index, arena);
	return true;
}

bool	try_lock_arena(t_yoyo_arena* arena, t_yoyo_zone_type zone_type) {
	if (!arena->multi_thread) {
		DEBUGOUT("SKIP: in single-thread mode: #%u(%p)", arena->index, arena);
		return true;
	}
	t_yoyo_subarena*	subarena = get_subarena(arena, zone_type);
	DEBUGOUT("TRY LOCKING subarena: %p", subarena);
	if (!try_lock_subarena(subarena)) {
		// DEBUGWARN("FAILED to try lock: %d(%s)", errno, strerror(errno));
		return false;
	}
	DEBUGOUT("TRY LOCKED subarena: #%u(%p)", arena->index, arena);
	return true;
}

bool	lock_subarena(t_yoyo_subarena* subarena) {
	if (!subarena->multi_thread) {
		// DEBUGOUT("SKIP: in single-thread mode: %p", subarena);
		return true;
	}
	DEBUGOUT("LOCKING subarena: %p", subarena);
	if (pthread_mutex_lock(&subarena->lock)) {
		DEBUGFATAL("FAILED to lock: %d(%s)", errno, strerror(errno));
		return false;
	}
	DEBUGOUT("LOCKED subarena: %p", subarena);
	return true;
}

bool	try_lock_subarena(t_yoyo_subarena* subarena) {
	if (!subarena->multi_thread) {
		// DEBUGOUT("SKIP: in single-thread mode: %p", subarena);
		return true;
	}
	DEBUGOUT("TRY LOCKING subarena: %p", subarena);
	if (pthread_mutex_trylock(&subarena->lock)) {
		// DEBUGWARN("FAILED to try lock: %d(%s)", errno, strerror(errno));
		return false;
	}
	DEBUGOUT("TRY LOCKED subarena: %p", subarena);
	return true;
}

bool	lock_zone(t_yoyo_zone* zone) {
	if (!zone->multi_thread) {
		// DEBUGOUT("SKIP: in single-thread mode: %p", zone);
		return true;
	}
	DEBUGOUT("LOCKING zone: %p", zone);
	if (pthread_mutex_lock(&zone->lock)) {
		DEBUGFATAL("FAILED to lock: %d(%s)", errno, strerror(errno));
		return false;
	}
	DEBUGOUT("LOCKED zone: %p", zone);
	return true;
}

bool	try_lock_zone(t_yoyo_zone* zone) {
	if (!zone->multi_thread) {
		// DEBUGOUT("SKIP: in single-thread mode: %p", zone);
		return true;
	}
	DEBUGOUT("TRY LOCKING zone: %p", zone);
	if (pthread_mutex_trylock(&zone->lock)) {
		// DEBUGWARN("FAILED to try lock: %d(%s)", errno, strerror(errno));
		return false;
	}
	DEBUGOUT("TRY LOCKED: %p", zone);
	return true;
}

bool	unlock_arena(t_yoyo_arena* arena, t_yoyo_zone_type zone_type) {
	if (!arena->multi_thread) {
		// DEBUGOUT("SKIP: in single-thread mode: #%u(%p)", arena->index, arena);
		return true;
	}
	t_yoyo_subarena*	subarena = get_subarena(arena, zone_type);
	DEBUGOUT("UNLOCKING arena: #%u(%p)", arena->index, arena);
	if (!unlock_subarena(subarena)) {
		DEBUGFATAL("FAILED to unlock: %d(%s)", errno, strerror(errno));
		return false;
	}
	DEBUGOUT("UNLOCKED arena: #%u(%p)", arena->index, arena);
	return true;	
}

bool	unlock_subarena(t_yoyo_subarena* subarena) {
	if (!subarena->multi_thread) {
		DEBUGOUT("SKIP: in single-thread mode: %p", subarena);
		return true;
	}
	DEBUGOUT("UNLOCKING subarena: %p", subarena);
	if (pthread_mutex_unlock(&subarena->lock)) {
		DEBUGFATAL("FAILED to unlock: %d(%s)", errno, strerror(errno));
		return false;
	}
	DEBUGOUT("UNLOCKED subarena: %p", subarena);
	return true;
}

bool	unlock_zone(t_yoyo_zone* zone) {
	if (!zone->multi_thread) {
		DEBUGOUT("SKIP: in single-thread mode: %p", zone);
		return true;
	}
	DEBUGOUT("LOCKED zone: %p", zone);
	if (pthread_mutex_unlock(&zone->lock)) {
		DEBUGFATAL("FAILED to unlock: %d(%s)", errno, strerror(errno));
		return false;
	}
	DEBUGOUT("UNLOCKED zone: %p", zone);
	return true;
}
