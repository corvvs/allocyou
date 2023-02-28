#include "includes/yoyo_structure.h"
#include "includes/yoyo_internal.h"

#include <stdio.h>

__attribute__((constructor))
static void init_yoyo() {
	init_realm(true);
}

int main() {
	printf("%zu\n", sizeof(bool));
	printf("%zu\n", sizeof(unsigned int));
	printf("%zu\n", sizeof(pthread_mutex_t));
	printf("%zu\n", sizeof(t_yoyo_zone));

	size_t	zone_tiny_bytes = 1024 * 1024;
	size_t	zone_small_bytes = 8 * 1024 * 1024;
	printf("bytes: %zu\n", heap_bytes_for_zone_bytes(zone_tiny_bytes));
	printf("bytes: %zu\n", heap_bytes_for_zone_bytes(zone_small_bytes));

	void* tiny = allocate_aligned_memory(ZONE_TINY_BYTE);
	printf("%p\n", tiny);
	printf("%d\n", *(char*)tiny);
	memset(tiny, '9', ZONE_TINY_BYTE);
	// memset(tiny, '9', ZONE_TINY_BYTE + 1); // The signal is caused by a WRITE memory access.
	printf("%d\n", *(char*)tiny);
	void* small = allocate_aligned_memory(ZONE_SMALL_BYTE);
	printf("%p\n", small);
	printf("%d\n", *(char*)small);
	memset(small, '9', ZONE_SMALL_BYTE);
	// memset(small, '9', ZONE_SMALL_BYTE + 1); // The signal is caused by a WRITE memory access.
	printf("%d\n", *(char*)small);
}
