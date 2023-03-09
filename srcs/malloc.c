#include "malloc.h"
#include "internal.h"


void*	yoyo_malloc(size_t n) {
	DEBUGOUT("** bytes: %zu **", n);
	SPRINT_START;
	void	*mem = actual_malloc(n);
	SPRINT_END("malloc");
	DEBUGSTR("** malloc end **");
	return mem;
}

void	yoyo_free(void* addr) {
	DEBUGOUT("** addr: %p **", addr);
	SPRINT_START;
	actual_free(addr);
	SPRINT_END("free");
	DEBUGSTR("** free end **");
}

void*	yoyo_realloc(void* addr, size_t n) {
	DEBUGOUT("** addr: %p, n: %zu **", addr, n);
	SPRINT_START;
	void*	mem = actual_realloc(addr, n);
	SPRINT_END("realloc");
	DEBUGSTR("** realloc end **");
	return mem;
}

void	show_alloc_mem(void) {
	DEBUGSTR("** show_alloc_mem **");
	SPRINT_START;
	actual_show_alloc_mem();
	SPRINT_END("show_alloc_mem");
	DEBUGSTR("** show_alloc_mem end **");
}
