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
	printf("%p\n", mem);
	mem = yoyo_malloc(10);
	printf("%p\n", mem);
	mem = yoyo_malloc(100);
	printf("%p\n", mem);
}

void	malloc_tiny_all() {
	for (int i = 0; i < 1025; ++i) {
		void* mem = yoyo_malloc(992);
		printf("%d -> %p\n", i, mem);
	}
}

void	malloc_large_basic() {
	void* mem;
	mem = yoyo_malloc(100000);
	printf("%p\n", mem);
	mem = yoyo_malloc(500000);
	printf("%p\n", mem);
	mem = yoyo_malloc(100000000);
	printf("%p\n", mem);
}

void	free_tiny_basic() {
	void* mem1 = yoyo_malloc(1);
	printf("mem1 = %p\n", mem1);
	void* mem2 = yoyo_malloc(1);
	printf("mem2 = %p\n", mem2);
	yoyo_free(mem1);
	yoyo_free(mem2);
}

void	free_large_basic() {
	void* mem1 = yoyo_malloc(100000);
	printf("mem1 = %p\n", mem1);
	void* mem2 = yoyo_malloc(500000);
	printf("mem2 = %p\n", mem2);
	void* mem3 = yoyo_malloc(12500000);
	printf("mem3 = %p\n", mem2);
	yoyo_free(mem2);
	yoyo_free(mem3);
	yoyo_free(mem1);
}

int main() {
	// printf("%zu\n", sizeof(bool));
	// printf("%zu\n", sizeof(unsigned int));
	// printf("%zu\n", sizeof(pthread_mutex_t));
	// printf("%zu\n", sizeof(t_yoyo_zone));

	// size_t	zone_tiny_bytes = 1024 * 1024;
	// size_t	zone_small_bytes = 8 * 1024 * 1024;
	// printf("bytes: %zu\n", heap_bytes_for_zone_bytes(zone_tiny_bytes));
	// printf("bytes: %zu\n", heap_bytes_for_zone_bytes(zone_small_bytes));

	// t_yoyo_zone* tiny = allocate_zone(&g_yoyo_realm.arenas[0], YOYO_ZONE_TINY);
	// printf("%p\n", tiny);
	// malloc_tiny_basic();
	// malloc_tiny_all();
	// malloc_large_basic();

	free_tiny_basic();
	// free_large_basic();
}
