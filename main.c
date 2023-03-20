#include "includes/structure.h"
#include "includes/internal.h"
#include "includes/malloc.h"

#include "test_malloc.h"

#include <stdio.h>

void	malloc_tiny_basic() {
	void* mem;
	mem = malloc(1);
	yoyo_dprintf(STDOUT_FILENO, "%p\n", mem);
	EXPECT_IS_NOT_NULL(mem);
	free(mem);
	mem = malloc(10);
	yoyo_dprintf(STDOUT_FILENO, "%p\n", mem);
	EXPECT_IS_NOT_NULL(mem);
	free(mem);
	mem = malloc(100);
	yoyo_dprintf(STDOUT_FILENO, "%p\n", mem);
	EXPECT_IS_NOT_NULL(mem);
	free(mem);
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
	yoyo_dprintf(STDOUT_FILENO, "%p\n", mem[0]);
	mem[1] = malloc(500000);
	EXPECT_IS_NOT_NULL(mem[1]);
	yoyo_dprintf(STDOUT_FILENO, "%p\n", mem[1]);
	mem[2] = malloc(100000000);
	EXPECT_IS_NOT_NULL(mem[2]);
	yoyo_dprintf(STDOUT_FILENO, "%p\n", mem[2]);
}

void	realloc_basic() {
	void*	mem1 = realloc(NULL, 60);
	yoyo_dprintf(STDOUT_FILENO, "mem1 = %p\n", mem1);
	void*	mem2 = realloc(mem1, 20);
	yoyo_dprintf(STDOUT_FILENO, "mem2 = %p\n", mem2);
	void*	mem3 = realloc(mem2, 24);
	yoyo_dprintf(STDOUT_FILENO, "mem3 = %p\n", mem3);
	void*	mem4 = realloc(mem3, 4000);
	yoyo_dprintf(STDOUT_FILENO, "mem4 = %p\n", mem4);
	void*	mem5 = realloc(mem4, 40000);
	yoyo_dprintf(STDOUT_FILENO, "mem5 = %p\n", mem5);
	void*	mem6 = realloc(mem5, 20000);
	yoyo_dprintf(STDOUT_FILENO, "mem6 = %p\n", mem6);
	void*	mem7 = realloc(mem6, 1);
	yoyo_dprintf(STDOUT_FILENO, "mem7 = %p\n", mem7);
}

#include <limits.h>
#include <errno.h>

void	test_extreme_malloc_single(size_t n) {
	errno = 0;
	void*	mem = malloc(n);
	printf("n = %zu B, mem = %p\n", n, mem);
	printf("errno = %d, %s\n", errno, strerror(errno));
	if (mem == NULL) {
		EXPECT_EQ_I(errno, ENOMEM);
	} else {
		EXPECT_EQ_I(errno, 0);
	}
	free(mem);
}

void	test_extreme_malloc(void) {
	test_extreme_malloc_single(0);
	test_extreme_malloc_single(UINT_MAX);
	test_extreme_malloc_single(SIZE_MAX / 2);
	test_extreme_malloc_single(SIZE_MAX - 1000);
	test_extreme_malloc_single(SIZE_MAX - 100);
	test_extreme_malloc_single(SIZE_MAX - 10);
	test_extreme_malloc_single(SIZE_MAX - 1);
	test_extreme_malloc_single(SIZE_MAX);
}

void	test_extreme_realloc_single(size_t n, size_t m) {
	errno = 0;
	void*	mem = realloc(NULL, n);
	printf("n = %zu B, mem = %p\n", n, mem);
	printf("errno = %d, %s\n", errno, strerror(errno));
	if (mem == NULL) {
		EXPECT_EQ_I(errno, ENOMEM);
	} else {
		EXPECT_EQ_I(errno, 0);
	}
	errno = 0;
	void*	mem2 = realloc(mem, m);
	printf("m = %zu B, mem2 = %p\n", m, mem2);
	printf("errno = %d, %s\n", errno, strerror(errno));
	if (mem2 == NULL) {
		EXPECT_EQ_I(errno, ENOMEM);
	} else {
		EXPECT_EQ_I(errno, 0);
	}
	if (mem != mem2 && mem2 == NULL) {
		free(mem);
	}
	free(mem2);
}

void	test_extreme_realloc(void) {
	size_t sizes[] = {1, UINT_MAX, SIZE_MAX / 2, SIZE_MAX - 1000, SIZE_MAX - 100, SIZE_MAX - 10, SIZE_MAX - 1, SIZE_MAX};
	// size_t sizes[] = {1, UINT_MAX, SIZE_MAX / 2};
	for (size_t i = 0; i < sizeof(sizes) / sizeof(sizes[0]); ++i) {
		for (size_t j = 0; j < sizeof(sizes) / sizeof(sizes[0]); ++j) {
			printf("<< %zu B -> %zu B >>\n", sizes[i], sizes[j]);
			test_extreme_realloc_single(sizes[i], sizes[j]);
		}
	}
}

int main() {
	// EXEC_TEST(malloc_tiny_basic);
	// EXEC_TEST(malloc_tiny_all);
	// EXEC_TEST(malloc_large_basic);

	// EXEC_TEST(realloc_basic);
	// EXEC_TEST(test_mass_basic);
	// EXEC_TEST(test_multithread_basic);
	// EXEC_TEST(test_multithread_realloc);

	EXEC_TEST(test_extreme_malloc);
	EXEC_TEST(test_extreme_realloc);
}
