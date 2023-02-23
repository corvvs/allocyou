#include "yo_internal.h"

#ifdef USE_LIBC
# define actual_malloc malloc
# define actual_free free
# define actual_realloc realloc
#else
# define actual_malloc yo_malloc_actual
# define actual_free yo_free_actual
# define actual_realloc yo_realloc_actual
#endif

void*	yo_malloc(size_t n) {
	DEBUGOUT("** bytes: %zu **", n);
	SPRINT_START;
	void	*mem = actual_malloc(n);
	SPRINT_END;
	DEBUGSTR("** malloc end **");
	check_consistency();
	return mem;
}

void	yo_free(void *addr) {
	DEBUGOUT("** addr: %p **", addr);
	SPRINT_START;
	actual_free(addr);
	SPRINT_END;
	DEBUGSTR("** free end **");
	check_consistency();
}

void*	yo_realloc(void *addr, size_t n) {
	DEBUGOUT("** addr: %p, bytes: %zu **", addr, n);
	SPRINT_START;
	void	*mem = actual_realloc(addr, n);
	SPRINT_END;
	DEBUGSTR("** realloc end **");
	check_consistency();
	return mem;
}
