#include "yo_internal.h"

// LARGE ゾーンに対する処理

extern t_yo_malloc_root	g_root;

void*	yo_large_malloc(size_t n) {
	t_yo_zone*	zone = yo_retrieve_zone(yo_zone_for_bytes(n));
	assert(zone != NULL); assert(zone->zone_class == YO_ZONE_LARGE);
	const size_t	blocks_needed = blocks_for_size(n);
	DEBUGOUT("** bytes: B:%zu, blocks: %zu **", n, blocks_needed);
	t_block_header	*head = yo_allocate_heap(blocks_needed, zone);
	if (head == NULL) {
		return NULL;
	}
	{
		SPRINT_START;
		insert_item(&zone->allocated, head);
		SPRINT_END("insert_to_allocated_large");
	}
	const size_t	blocks_allocated = head->blocks + 1;
	zone->cons.used_blocks += blocks_allocated;
	return head + 1;
}

void	yo_large_free(void* addr) {
	t_yo_zone*	zone = yo_retrieve_zone(yo_zone_for_addr(addr));
	assert(zone != NULL); assert(zone->zone_class == YO_ZONE_LARGE);
	t_block_header*	head = addr;
	--head;
	remove_item(&zone->allocated, head);
	const size_t	blocks_freed = head->blocks + 1;
	zone->cons.used_blocks -= blocks_freed;
	yo_deallocate_heap(head, zone);
	return;
}
