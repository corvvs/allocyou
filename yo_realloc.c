#include "yo_internal.h"

void*	yo_relocate(void* addr, size_t n) {
	DEBUGSTR("REALOCATE");
	void*	relocated = yo_actual_malloc(n);
	if (relocated == NULL) {
		return (NULL);
	}
	t_block_header*	head_to = relocated;
	--head_to;
	DEBUGOUT("head_to: (%zu, %p, %p)", head_to->blocks, head_to, head_to->next);
	t_block_header*	head_from = addr;
	--head_from;
	const size_t	bytes_current = YO_MIN(head_from->blocks, head_to->blocks) * BLOCK_UNIT_SIZE;
	yo_memcpy(relocated, addr, bytes_current);
	yo_actual_free(addr);
	return relocated;
}

void	yo_extend_chunk(t_yo_zone* zone, t_block_header* head, size_t n) {
	const size_t	blocks_needed = BLOCKS_FOR_SIZE(n);
	const size_t	blocks_current = head->blocks;
	assert(yo_zone_for_bytes(n) == yo_zone_for_addr(head + 1));
	assert(blocks_needed > blocks_current);
	t_block_header*	adjacent = head + head->blocks + 1;
	t_listcursor	cursor = find_fit_cursor(zone->frees, adjacent);
	assert(cursor.curr == adjacent);
	const size_t	blocks_capable = blocks_current + adjacent->blocks + 1;
	assert(blocks_capable >= blocks_needed);
	if (blocks_capable - blocks_needed > 1) {
		DEBUGSTR("SPLIT");
		t_block_header*	new_free = head + blocks_needed + 1;
		*new_free = (t_block_header) {
			.blocks = blocks_capable - (blocks_needed + 1),
			.next = cursor.curr->next,
		};
		head->blocks = blocks_needed;
		if (cursor.prev != NULL) {
			concat_item(cursor.prev, new_free);
		} else {
			zone->frees = new_free;
		}
		nullify_chunk(cursor.curr);
	} else {
		DEBUGSTR("EXHAUST");
		remove_item(&cursor.prev, cursor.curr);
		assimilate_chunk(head, cursor.curr);
	}
}

void	yo_shrink_chunk(t_block_header* head, size_t n) {
	assert(!GET_IS_LARGE(head->next));

	size_t	blocks_needed = BLOCKS_FOR_SIZE(n);
	assert(head->blocks >= blocks_needed);
	if (head->blocks < blocks_needed + 2) {
		DEBUGOUT("** MAINTAIN(%zu, %p, %p) **", head->blocks, head, head->next);
		return;
	}
	DEBUGOUT("SHRINK chunk %zu blocks -> %zu blocks", head->blocks, blocks_needed);
	t_block_header	*new_free = head + blocks_needed + 1;
	*new_free = (t_block_header) {
		.blocks	= head->blocks - (blocks_needed + 1),
		.next	= COPYFLAGS(NULL, head->next),
	};
	DEBUGOUT("new_free: (%zu, %p, %p)", new_free->blocks, new_free, new_free->next);
	head->blocks = blocks_needed;
	yo_actual_free(new_free + 1);
}
