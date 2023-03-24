#include "test_malloc.h"

void	special_64(void) {
	int		s = 0;
	char*	mem = NULL;
	for (int i = 0; i < 2; ++i) {
		for (size_t i = 1; i < 68; ++i) {
			mem = malloc(i * BLOCK_UNIT_SIZE);
			mem[0] = 1;
			s = s + *((char *)mem);
			// free(mem);
		}
	}
	yoyo_dprintf(STDOUT_FILENO, "%d\n", s);
}

int main() {
	EXEC_TEST(special_64);
	EXEC_TEST(malloc_tiny_basic);
	EXEC_TEST(malloc_tiny_all);
	EXEC_TEST(malloc_large_basic);
	EXEC_TEST(malloc_usable_size_basic);
	EXEC_TEST(memalign_basic);
	EXEC_TEST(history_basic);
	EXEC_TEST(test_mass_history);
	EXEC_TEST(realloc_basic);
	EXEC_TEST(test_mass_malloc_and_free);
	EXEC_TEST(test_multithread_basic);
	EXEC_TEST(test_multithread_realloc);
	EXEC_TEST(test_singlethread_realloc);
	EXEC_TEST(test_extreme_malloc);
	EXEC_TEST(test_extreme_realloc);
	EXEC_TEST(test_tiny_fine);
	EXEC_TEST(test_realloc_fine);
}
