#include "malloc.h"
#include "internal.h"
#include "test_malloc.h"

#include <limits.h>
#include <errno.h>

static void	test_extreme_malloc_single(size_t n) {
	errno = 0;
	void*	mem = malloc(n);
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

static void	test_extreme_realloc_single(size_t n, size_t m) {
	errno = 0;
	void*	mem = realloc(NULL, n);
	if (mem == NULL) {
		EXPECT_EQ_I(errno, ENOMEM);
	} else {
		EXPECT_EQ_I(errno, 0);
	}
	errno = 0;
	void*	mem2 = realloc(mem, m);
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
			yoyo_dprintf(STDOUT_FILENO, "<< %zu B -> %zu B >>\n", sizes[i], sizes[j]);
			test_extreme_realloc_single(sizes[i], sizes[j]);
		}
	}
}
