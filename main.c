#include "test_malloc.h"


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


int main() {
	EXEC_TEST(malloc_tiny_basic);
	EXEC_TEST(malloc_tiny_all);
	EXEC_TEST(malloc_large_basic);

	EXEC_TEST(realloc_basic);

	EXEC_TEST(test_mass_malloc_and_free);
	EXEC_TEST(test_multithread_basic);
	EXEC_TEST(test_multithread_realloc);

	EXEC_TEST(test_extreme_malloc);
	EXEC_TEST(test_extreme_realloc);

	EXEC_TEST(test_tiny_fine);
	EXEC_TEST(test_realloc_fine);
}
