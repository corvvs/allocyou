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
}
