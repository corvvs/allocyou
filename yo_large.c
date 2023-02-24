#include "yo_internal.h"

// LARGE ゾーンに対する処理

extern t_yo_malloc_root	g_root;

void*	yo_large_malloc(size_t n) {
	assert(yo_zone_for_bytes(n) == YO_ZONE_LARGE);
	size_t	blocks_needed = BLOCKS_FOR_SIZE(n);
	DEBUGOUT("** bytes: B:%zu, blocks: %zu **", n, blocks_needed);
	t_block_header	*head = yo_allocate_heap(blocks_needed, YO_ZONE_LARGE);
	if (head == NULL) {
		return NULL;
	}
	DEBUGWARN("(1) head->next = %p", head->next);
	insert_item(&g_root.large, head);
	DEBUGWARN("(2) head->next = %p", head->next);
	return head + 1;
}

void	yo_large_free(void* addr) {
	assert(yo_zone_for_addr(addr) == YO_ZONE_LARGE);
	t_block_header*	head = addr;
	--head;
	remove_item(&g_root.large, head);
	size_t bytes = (head->blocks + 1) * BLOCK_UNIT_SIZE;
	if (munmap(head, bytes) < 0) {
		DEBUGOUT("FAILED: errno = %d, %s", errno, strerror(errno));
		return;
	}
	return;
}
