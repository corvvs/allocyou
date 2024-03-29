#include "malloc.h"
#include "internal.h"


__attribute__((constructor))
static void	yoyo_init() {
	init_realm();
}

void*	malloc(size_t n) {
	DEBUGOUT("** bytes: %zu **", n);
	SPRINT_START;
	void	*mem = yoyo_actual_malloc(n);
	SPRINT_END(__func__);
	DEBUGOUT("** %s end, returning %p for %zu B **", __func__, mem, n);
	take_history(YOYO_OP_MALLOC, mem, n, 0);
	return mem;
}

void	free(void* addr) {
	DEBUGOUT("** addr: %p **", addr);
	SPRINT_START;
	yoyo_actual_free(addr);
	SPRINT_END(__func__);
	DEBUGOUT("** %s end **", __func__);
	take_history(YOYO_OP_FREE, addr, 0, 0);
}

void*	realloc(void* addr, size_t n) {
	DEBUGOUT("** addr: %p, n: %zu **", addr, n);
	SPRINT_START;
	void*	mem = yoyo_actual_realloc(addr, n);
	SPRINT_END(__func__);
	DEBUGOUT("** %s end, returning %p for %p, %zu B **", __func__, mem, addr, n);
	take_history(YOYO_OP_REALLOC, mem, n, (size_t)addr);
	return mem;
}

void*	calloc(size_t count, size_t size) {
	DEBUGOUT("** count: %zu, size: %zu **", count, size);
	SPRINT_START;
	void	*mem = yoyo_actual_calloc(count, size);
	SPRINT_END("malloc");
	DEBUGOUT("** %s end, returning %p for %zu x %zu B **", __func__, mem, count, size);
	take_history(YOYO_OP_CALLOC, mem, count, size);
	return mem;
}

__attribute__((visibility("default")))
void	show_alloc_mem(void) {
	DEBUGOUT("** %s **", __func__);
	SPRINT_START;
	actual_show_alloc_mem();
	SPRINT_END(__func__);
	DEBUGOUT("** %s end **", __func__);
}

__attribute__((visibility("default")))
void	show_alloc_mem_ex(void) {
	DEBUGOUT("** %s **", __func__);
	SPRINT_START;
	actual_show_alloc_mem_ex();
	SPRINT_END(__func__);
	DEBUGOUT("** %s end **", __func__);
}

size_t 	malloc_usable_size(void *ptr) {
	DEBUGOUT("** %s **", __func__);
	SPRINT_START;
	size_t	rv = yoyo_actual_malloc_usable_size(ptr);
	SPRINT_END(__func__);
	DEBUGOUT("** %s end, returning %zu B for %p **", __func__, rv, ptr);
	return rv;
}

void*	memalign(size_t alignment, size_t size) {
	DEBUGOUT("** %s **", __func__);
	SPRINT_START;
	void*	rv = yoyo_actual_memalign(alignment, size);
	SPRINT_END(__func__);
	DEBUGOUT("** %s end, returning %p for (%zu, %zu) **", __func__, rv, alignment, size);
	take_history(YOYO_OP_MEMALIGN, rv, alignment, size);
	return rv;
}

void*	aligned_alloc(size_t alignment, size_t size) {
	DEBUGOUT("** %s **", __func__);
	SPRINT_START;
	void*	rv = yoyo_actual_aligned_alloc(alignment, size);
	SPRINT_END(__func__);
	DEBUGOUT("** %s end, returning %p for (%zu, %zu) **", __func__, rv, alignment, size);
	take_history(YOYO_OP_EXTRA, rv, alignment, size);
	return rv;
}

int		posix_memalign(void **memptr, size_t alignment, size_t size) {
	DEBUGOUT("** %s **", __func__);
	SPRINT_START;
	int	rv = yoyo_actual_posix_memalign(memptr, alignment, size);
	SPRINT_END(__func__);
	DEBUGOUT("** %s end, returning %d for (%p, %zu, %zu) **", __func__, rv, memptr, alignment, size);
	take_history(YOYO_OP_EXTRA, memptr, alignment, size);
	return rv;
}

void	release_memory(void) {
	SPRINT_START;
	yoyo_actual_release_memory();
	SPRINT_END(__func__);
	DEBUGOUT("** %s end **", __func__);
}

__attribute__((destructor))
static void	yoyo_exit() {
	show_alloc_mem_ex();
}
