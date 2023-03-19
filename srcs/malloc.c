#include "malloc.h"
#include "internal.h"


__attribute__((constructor))
static void	yoyo_init() {
	DEBUGSTR("INIT");
	init_realm(true);	
}

void*	malloc(size_t n) {
	DEBUGOUT("** bytes: %zu **", n);
	SPRINT_START;
	void	*mem = yoyo_actual_malloc(n);
	SPRINT_END("malloc");
	DEBUGOUT("** malloc end, returning %p for %zu B **", mem, n);
	return mem;
}

void	free(void* addr) {
	DEBUGOUT("** addr: %p **", addr);
	SPRINT_START;
	yoyo_actual_free(addr);
	SPRINT_END("free");
	DEBUGSTR("** free end **");
}

void*	realloc(void* addr, size_t n) {
	DEBUGOUT("** addr: %p, n: %zu **", addr, n);
	SPRINT_START;
	void*	mem = yoyo_actual_realloc(addr, n);
	SPRINT_END("realloc");
	DEBUGOUT("** realloc end, returning %p for %p, %zu B **", mem, addr, n);
	return mem;
}

void*	calloc(size_t count, size_t size) {
	DEBUGOUT("** count: %zu, size: %zu **", count, size);
	SPRINT_START;
	void	*mem = yoyo_actual_calloc(count, size);
	SPRINT_END("malloc");
	DEBUGOUT("** calloc end, returning %p for %zu x %zu B **", mem, count, size);
	return mem;
}

__attribute__((visibility("default")))
void	show_alloc_mem(void) {
	DEBUGSTR("** show_alloc_mem **");
	SPRINT_START;
	actual_show_alloc_mem();
	SPRINT_END("show_alloc_mem");
	DEBUGSTR("** show_alloc_mem end **");
}

__attribute__((destructor))
static void	yoyo_exit() {
	show_alloc_mem();
	DEBUGSTR("EXIT");
}
