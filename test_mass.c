#include "malloc.h"
#include "internal.h"


#define N 1000
void	test_mass_basic(void) {
	char*	mems[N];
	for (int i = 0; i < N; ++i) {
		mems[i] = yoyo_malloc(rand() % 123 + 23);
	}
	for (int i = 0; i < N; ++i) {
		yoyo_free(mems[N - 1 - i]);
	}
	for (int i = 0; i < N; ++i) {
		mems[i] = yoyo_malloc(rand() % 123 + 123);
	}
	for (int i = 0; i < N; ++i) {
		yoyo_free(mems[i]);
	}

}
#undef N

