#include "test_malloc.h"



int main() {
	EXEC_TEST(malloc_tiny_basic);
	EXEC_TEST(malloc_tiny_all);
	EXEC_TEST(malloc_large_basic);
	EXEC_TEST(malloc_usable_size_basic);

	EXEC_TEST(realloc_basic);

	EXEC_TEST(test_mass_malloc_and_free);
	EXEC_TEST(test_multithread_basic);
	EXEC_TEST(test_multithread_realloc);

	EXEC_TEST(test_extreme_malloc);
	EXEC_TEST(test_extreme_realloc);

	EXEC_TEST(test_tiny_fine);
	EXEC_TEST(test_realloc_fine);
}
