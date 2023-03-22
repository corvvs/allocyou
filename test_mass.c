#include "malloc.h"
#include "internal.h"


#define N 1000
void	test_mass_malloc_and_free(void) {
	char*	mems[N];
	for (int i = 0; i < N; ++i) {
		mems[i] = malloc(rand() % 123 + 23);
	}
	for (int i = 0; i < N; ++i) {
		free(mems[N - 1 - i]);
	}
	for (int i = 0; i < N; ++i) {
		mems[i] = malloc(rand() % 123 + 123);
	}
	for (int i = 0; i < N; ++i) {
		free(mems[i]);
	}

}
#undef N


#define N 1000

void	test_mass_history(void) {
	size_t n = 0;
	for (int i = 0; i < N; ++i) {
		void* mem = malloc(rand() % 2000 + 1);
		n += malloc_usable_size(mem);
		free(mem);
	}
	yoyo_dprintf(STDOUT_FILENO, "total = %zu B\n", n);
	show_alloc_mem_ex();
}

#undef N
