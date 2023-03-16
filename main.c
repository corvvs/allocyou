#include "includes/structure.h"
#include "includes/internal.h"
#include "includes/malloc.h"

#include "test_malloc.h"

#include <stdio.h>

__attribute__((constructor))
static void init_yoyo() {
	init_realm(true);
}

void	malloc_tiny_basic() {
	void* mem;
	mem = malloc(1);
	yoyo_dprintf(STDOUT_FILENO, "%p\n", mem);
	mem = malloc(10);
	yoyo_dprintf(STDOUT_FILENO, "%p\n", mem);
	mem = malloc(100);
	yoyo_dprintf(STDOUT_FILENO, "%p\n", mem);
	show_alloc_mem();
}

void	malloc_tiny_all() {
	for (int i = 0; i < 1025; ++i) {
		void* mem = malloc(992);
		yoyo_dprintf(STDOUT_FILENO, "%d -> %p\n", i, mem);
	}
}

void	malloc_large_basic() {
	void* mem;
	mem = malloc(100000);
	yoyo_dprintf(STDOUT_FILENO, "%p\n", mem);
	mem = malloc(500000);
	yoyo_dprintf(STDOUT_FILENO, "%p\n", mem);
	mem = malloc(100000000);
	yoyo_dprintf(STDOUT_FILENO, "%p\n", mem);
}

void	free_tiny_basic() {
	void* mem1 = malloc(1);
	yoyo_dprintf(STDOUT_FILENO, "mem1 = %p\n", mem1);
	void* mem2 = malloc(1);
	yoyo_dprintf(STDOUT_FILENO, "mem2 = %p\n", mem2);
	void* mem3 = malloc(1);
	yoyo_dprintf(STDOUT_FILENO, "mem3 = %p\n", mem3);
	show_alloc_mem();
	free(mem1);
	free(mem3);
	free(mem2);
	show_alloc_mem();
}

void	free_large_basic() {
	show_alloc_mem();
	void* mem1 = malloc(100000);
	yoyo_dprintf(STDOUT_FILENO, "mem1 = %p\n", mem1);
	void* mem2 = malloc(500000);
	yoyo_dprintf(STDOUT_FILENO, "mem2 = %p\n", mem2);
	void* mem3 = malloc(12500000);
	yoyo_dprintf(STDOUT_FILENO, "mem3 = %p\n", mem2);
	show_alloc_mem();
	print_memory_state(mem1);
	print_memory_state(mem2);
	print_memory_state(mem3);
	free(mem2);
	free(mem3);
	free(mem1);
	show_alloc_mem();
}

void	realloc_basic() {
	void*	mem1 = realloc(NULL, 60);
	yoyo_dprintf(STDOUT_FILENO, "mem1 = %p\n", mem1);
	void*	mem2 = realloc(mem1, 20);
	yoyo_dprintf(STDOUT_FILENO, "mem2 = %p\n", mem2);
	void*	mem3 = realloc(mem2, 24);
	yoyo_dprintf(STDOUT_FILENO, "mem3 = %p\n", mem3);
	show_alloc_mem();
	void*	mem4 = realloc(mem3, 4000);
	yoyo_dprintf(STDOUT_FILENO, "mem4 = %p\n", mem4);
	show_alloc_mem();
	void*	mem5 = realloc(mem4, 40000);
	yoyo_dprintf(STDOUT_FILENO, "mem5 = %p\n", mem5);
	void*	mem6 = realloc(mem5, 20000);
	yoyo_dprintf(STDOUT_FILENO, "mem6 = %p\n", mem6);
	show_alloc_mem();
	void*	mem7 = realloc(mem6, 1);
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

	// free_tiny_basic();
	// free_large_basic();

	// realloc_basic();
	// test_mass_basic();
	// test_multithread_basic();
	test_multithread_realloc();
}
