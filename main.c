#include "includes/yoyo_structure.h"
#include "includes/yoyo_internal.h"
#include "includes/yoyo_malloc.h"

#include <stdio.h>

__attribute__((constructor))
static void init_yoyo() {
	init_realm(true);
}

void	malloc_tiny_basic() {
	void* mem;
	mem = yoyo_malloc(1);
	yoyo_dprintf(STDOUT_FILENO, "%p\n", mem);
	mem = yoyo_malloc(10);
	yoyo_dprintf(STDOUT_FILENO, "%p\n", mem);
	mem = yoyo_malloc(100);
	yoyo_dprintf(STDOUT_FILENO, "%p\n", mem);
	show_alloc_mem();
}

void	malloc_tiny_all() {
	for (int i = 0; i < 1025; ++i) {
		void* mem = yoyo_malloc(992);
		yoyo_dprintf(STDOUT_FILENO, "%d -> %p\n", i, mem);
	}
}

void	malloc_large_basic() {
	void* mem;
	mem = yoyo_malloc(100000);
	yoyo_dprintf(STDOUT_FILENO, "%p\n", mem);
	mem = yoyo_malloc(500000);
	yoyo_dprintf(STDOUT_FILENO, "%p\n", mem);
	mem = yoyo_malloc(100000000);
	yoyo_dprintf(STDOUT_FILENO, "%p\n", mem);
}

void	free_tiny_basic() {
	void* mem1 = yoyo_malloc(1);
	yoyo_dprintf(STDOUT_FILENO, "mem1 = %p\n", mem1);
	void* mem2 = yoyo_malloc(1);
	yoyo_dprintf(STDOUT_FILENO, "mem2 = %p\n", mem2);
	void* mem3 = yoyo_malloc(1);
	yoyo_dprintf(STDOUT_FILENO, "mem3 = %p\n", mem3);
	show_alloc_mem();
	yoyo_free(mem1);
	yoyo_free(mem3);
	yoyo_free(mem2);
	show_alloc_mem();
}

void	free_large_basic() {
	show_alloc_mem();
	void* mem1 = yoyo_malloc(100000);
	yoyo_dprintf(STDOUT_FILENO, "mem1 = %p\n", mem1);
	void* mem2 = yoyo_malloc(500000);
	yoyo_dprintf(STDOUT_FILENO, "mem2 = %p\n", mem2);
	void* mem3 = yoyo_malloc(12500000);
	yoyo_dprintf(STDOUT_FILENO, "mem3 = %p\n", mem2);
	show_alloc_mem();
	print_memory_state(mem1);
	print_memory_state(mem2);
	print_memory_state(mem3);
	yoyo_free(mem2);
	yoyo_free(mem3);
	yoyo_free(mem1);
	show_alloc_mem();
}

void	realloc_basic() {
	void*	mem1 = yoyo_realloc(NULL, 60);
	yoyo_dprintf(STDOUT_FILENO, "mem1 = %p\n", mem1);
	void*	mem2 = yoyo_realloc(mem1, 20);
	yoyo_dprintf(STDOUT_FILENO, "mem2 = %p\n", mem2);
	void*	mem3 = yoyo_realloc(mem2, 24);
	yoyo_dprintf(STDOUT_FILENO, "mem3 = %p\n", mem3);
	show_alloc_mem();
	void*	mem4 = yoyo_realloc(mem3, 4000);
	yoyo_dprintf(STDOUT_FILENO, "mem4 = %p\n", mem4);
	show_alloc_mem();
	void*	mem5 = yoyo_realloc(mem4, 40000);
	yoyo_dprintf(STDOUT_FILENO, "mem5 = %p\n", mem5);
	void*	mem6 = yoyo_realloc(mem5, 20000);
	yoyo_dprintf(STDOUT_FILENO, "mem6 = %p\n", mem6);
	show_alloc_mem();
	void*	mem7 = yoyo_realloc(mem6, 1);
	yoyo_dprintf(STDOUT_FILENO, "mem7 = %p\n", mem7);
	show_alloc_mem();
}

int main() {
	// yoyo_dprintf(STDOUT_FILENO, "%zu\n", sizeof(bool));
	// yoyo_dprintf(STDOUT_FILENO, "%zu\n", sizeof(unsigned int));
	// yoyo_dprintf(STDOUT_FILENO, "%zu\n", sizeof(pthread_mutex_t));
	// yoyo_dprintf(STDOUT_FILENO, "%zu\n", sizeof(t_yoyo_zone));

	// size_t	zone_tiny_bytes = 1024 * 1024;
	// size_t	zone_small_bytes = 8 * 1024 * 1024;
	// yoyo_dprintf(STDOUT_FILENO, "bytes: %zu\n", heap_bytes_for_zone_bytes(zone_tiny_bytes));
	// yoyo_dprintf(STDOUT_FILENO, "bytes: %zu\n", heap_bytes_for_zone_bytes(zone_small_bytes));

	// t_yoyo_zone* tiny = allocate_zone(&g_yoyo_realm.arenas[0], YOYO_ZONE_TINY);
	// yoyo_dprintf(STDOUT_FILENO, "%p\n", tiny);
	// malloc_tiny_basic();
	// malloc_tiny_all();
	// malloc_large_basic();

	free_tiny_basic();
	// free_large_basic();

	// realloc_basic();
}
