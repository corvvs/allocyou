#include "yoyo_malloc.h"
#include "yoyo_internal.h"


void*	yoyo_malloc(size_t n) {
	DEBUGOUT("** bytes: %zu **", n);
	SPRINT_START;
	void	*mem = actual_malloc(n);
	SPRINT_END("malloc");
	DEBUGSTR("** malloc end **");
	return mem;
}

void	yoyo_free(void* addr) {
	free(addr);
}
void*	yoyo_realloc(void* addr, size_t n) {
	return realloc(addr, n);
}
