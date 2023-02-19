#include "yo_internal.h"

void	yo_free(void *addr) {
	SPRINT_START;
	yo_free_actual(addr);
	SPRINT_END;
}

void*	yo_malloc(size_t n) {
	SPRINT_START;
	void	*mem = yo_malloc_actual(n);
	SPRINT_END;
	return mem;
}
