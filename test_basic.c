#include "malloc.h"
#include "internal.h"
#include "test_malloc.h"

#include <limits.h>
#include <errno.h>

void	malloc_tiny_basic() {
	void* mem;
	mem = malloc(1);
	EXPECT_IS_NOT_NULL(mem);
	free(mem);
	mem = malloc(10);
	EXPECT_IS_NOT_NULL(mem);
	free(mem);
	mem = malloc(100);
	EXPECT_IS_NOT_NULL(mem);
	free(mem);
}

void	malloc_tiny_basic2() {
	void*	mem1 = malloc(1);
	EXPECT_IS_NOT_NULL(mem1);
	void*	mem2 = malloc(10);
	EXPECT_IS_NOT_NULL(mem2);
	void*	mem3 = malloc(100);
	EXPECT_IS_NOT_NULL(mem3);
	ft_memset(mem1, 'a', 1);
	ft_memset(mem2, 'a', 10);
	ft_memset(mem3, 'a', 100);

	free(mem1);
	free(mem2);
	free(mem3);
}

void	malloc_tiny_all() {
	int	n = 1025;
	void**	mems = malloc(sizeof(void*) * n);
	EXPECT_IS_NOT_NULL(mems);
	for (int i = 0; i < n; ++i) {
		mems[i] = malloc(992);
	}
	for (int i = 0; i < n; ++i) {
		free(mems[i]);
	}
	free(mems);
}

void	malloc_large_basic() {
	void* mem[3];
	mem[0] = malloc(100000);
	EXPECT_IS_NOT_NULL(mem[0]);
	mem[1] = malloc(500000);
	EXPECT_IS_NOT_NULL(mem[1]);
	mem[2] = malloc(100000000);
	EXPECT_IS_NOT_NULL(mem[2]);
}

void	realloc_basic() {
	void*	mem1 = realloc(NULL, 60);
	EXPECT_IS_NOT_NULL(mem1);
	void*	mem2 = realloc(mem1, 20);
	EXPECT_IS_NOT_NULL(mem2);
	EXPECT_EQ(mem1, mem2);
	void*	mem3 = realloc(mem2, 24);
	EXPECT_IS_NOT_NULL(mem3);
	EXPECT_EQ(mem2, mem3);
	void*	mem4 = realloc(mem3, 4000);
	EXPECT_IS_NOT_NULL(mem4);
	EXPECT_NE(mem3, mem4);
	void*	mem5 = realloc(mem4, 40000);
	EXPECT_IS_NOT_NULL(mem5);
	EXPECT_NE(mem4, mem5);
	void*	mem6 = realloc(mem5, 20000);
	EXPECT_IS_NOT_NULL(mem6);
	EXPECT_EQ(mem5, mem6);
	void*	mem7 = realloc(mem6, 1);
	EXPECT_IS_NOT_NULL(mem7);
	EXPECT_NE(mem6, mem7);
}

void	malloc_usable_size_basic(void) {
	void*	mem = malloc(1);
	EXPECT_NE_I(malloc_usable_size(mem), 1);
	EXPECT_EQ_I(malloc_usable_size(mem), 16);
	mem = realloc(mem, 10);
	EXPECT_EQ_I(malloc_usable_size(mem), 16);
	mem = realloc(mem, 16);
	EXPECT_EQ_I(malloc_usable_size(mem), 16);
	mem = realloc(mem, 17);
	EXPECT_EQ_I(malloc_usable_size(mem), 32);
	mem = realloc(mem, 1000);
	EXPECT_EQ_I(malloc_usable_size(mem), 1008);
	mem = realloc(mem, 1);
	EXPECT_EQ_I(malloc_usable_size(mem), 16);
	mem = realloc(mem, 9999);
	EXPECT_EQ_I(malloc_usable_size(mem), 10000);
	mem = realloc(mem, 10000);
	EXPECT_EQ_I(malloc_usable_size(mem), 10000);
}

void	memalign_basic_sub(size_t alignment, size_t size) {
	void* mem = memalign(alignment, size);
	EXPECT_IS_NOT_NULL(mem);
	// yoyo_dprintf(STDOUT_FILENO, "mem = %p, usable = %zu\n", mem, malloc_usable_size(mem));
	EXPECT_EQ_I((uintptr_t)mem % alignment, 0);
	EXPECT_GE_I(malloc_usable_size(mem), size);
	free(mem);
}

void	memalign_basic(void) {
	for (size_t n = 1; n < 10; ++n) {
		size_t	a = 1u << n;
		for (size_t size = a; size < 10000; size *= a) {
			// yoyo_dprintf(STDOUT_FILENO, "(alignment, size) = (%zu, %zu)\n", a, size);
			memalign_basic_sub(a, size);
		}
	}
}

void	history_basic(void) {
	char*	mem = malloc(1);
	mem[0] = 'a';
	write(1, mem, 1);
	free(mem);
}

