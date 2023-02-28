#include "yoyo_malloc.h"

void*	yoyo_malloc(size_t n) {
	return malloc(n);
}

void	yoyo_free(void* addr) {
	free(addr);
}
void*	yoyo_realloc(void* addr, size_t n) {
	return realloc(addr, n);
}
