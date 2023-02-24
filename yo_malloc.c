#include "yo_internal.h"

// ユーザに直接見せるラッパー関数の定義
// 実際の定義は yo_actual_***.c を見ること.

#ifdef USE_LIBC
# define actual_malloc malloc
# define actual_free free
# define actual_realloc realloc
#else
# define actual_malloc yo_actual_malloc
# define actual_free yo_actual_free
# define actual_realloc yo_actual_realloc
#endif

extern t_yo_malloc_root	g_root;

void*	yo_malloc(size_t n) {
	DEBUGOUT("** bytes: %zu **", n);
	SPRINT_START;
	void	*mem = actual_malloc(n);
	SPRINT_END("malloc");
	DEBUGSTR("** malloc end **");
	check_consistency();
	return mem;
}

void	yo_free(void *addr) {
	DEBUGOUT("** addr: %p **", addr);
	SPRINT_START;
	actual_free(addr);
	SPRINT_END("free");
	DEBUGSTR("** free end **");
	check_consistency();
}

void*	yo_realloc(void *addr, size_t n) {
	DEBUGOUT("** addr: %p, bytes: %zu **", addr, n);
	SPRINT_START;
	void	*mem = actual_realloc(addr, n);
	SPRINT_END("realloc");
	DEBUGSTR("** realloc end **");
	check_consistency();
	return mem;
}

void	show_alloc_mem(void) {
	actual_show_alloc_mem();
}
