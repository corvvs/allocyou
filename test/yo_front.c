#include "yo_internal.h"

void*	yo_malloc(size_t n) {
	DEBUGOUT("** bytes: %zu **", n);
	SPRINT_START;
	void	*mem = yo_malloc_actual(n);
	SPRINT_END;
	DEBUGSTR("** malloc end **");
	check_consistency();
	return mem;
}

void	yo_free(void *addr) {
	DEBUGOUT("** addr: %p **", addr);
	SPRINT_START;
	yo_free_actual(addr);
	SPRINT_END;
	DEBUGSTR("** free end **");
	check_consistency();
}

void*	yo_realloc(void *addr, size_t n) {
	DEBUGOUT("** addr: %p, bytes: %zu **", addr, n);
	SPRINT_START;
	void	*mem = yo_realloc_actual(addr, n);
	SPRINT_END;
	DEBUGSTR("** realloc end **");
	check_consistency();
	return mem;
}
