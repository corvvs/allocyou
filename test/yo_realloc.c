#include "yo_internal.h"

// n バイトが入るチャンクを新しく確保し, チャンク head の内容をコピーし, head を破棄する.
// 新しいチャンクを返す.
void*	_yo_relocate_chunk(t_block_header* head, size_t n) {
	DEBUGSTR("** RELOCATE!! **");
	void*	new_mem = yo_malloc(n);
	if (new_mem == NULL) {
		DEBUGOUT("malloc(%zu) failed", n);
		return NULL;
	}
	t_block_header*	new_head = new_mem;
	--new_head;
	DEBUGOUT("yo_memcpy(%p, %p, %zu)", new_head + 1, head + 1, head->blocks * BLOCK_UNIT_SIZE);
	yo_memcpy(new_head + 1, head + 1, head->blocks * BLOCK_UNIT_SIZE);
	DEBUGOUT("freeing %p", head);
	COPYFLAGS(new_head->next, head->next);
	yo_free_actual(head);
	return new_mem;
}


void*	_yo_relocate(void* addr, size_t n) {
	DEBUGSTR("REALOCATE");
	void*	relocated = yo_malloc_actual(n);
	if (relocated == NULL) {
		return (NULL);
	}
	t_block_header*	head_to = relocated;
	--head_to;
	DEBUGOUT("head_to: (%zu, %p, %p)", head_to->blocks, head_to, head_to->next);
	t_block_header*	head_from = addr;
	--head_from;
	const size_t	bytes_current = YO_MIN(head_from->blocks, head_to->blocks) * BLOCK_UNIT_SIZE;
	DEBUGSTR("BEFORE MEMCPY");
	check_consistency();
	yo_memcpy(relocated, addr, bytes_current);
	DEBUGSTR("AFTER MEMCPY");
	check_consistency();
	yo_free_actual(addr);
	DEBUGSTR("AFTER FREE addr");
	check_consistency();
	return relocated;
}

void	_yo_extend_chunk(t_yo_zone* zone, t_block_header* head, size_t n) {
	const size_t	blocks_needed = BLOCKS_FOR_SIZE(n);
	const size_t	blocks_current = head->blocks;
	assert(blocks_needed > blocks_current);
	t_block_header*	left_adjacent = head + head->blocks + 1;
	t_listcursor	cursor = find_fit_cursor(zone->frees, left_adjacent);
	assert(cursor.curr == left_adjacent);
	const size_t	blocks_capable = blocks_current + left_adjacent->blocks + 1;
	assert(blocks_needed <= blocks_capable);
	if (blocks_capable - 1 <= blocks_needed) {
		DEBUGSTR("EXHAUST");
		remove_item(&cursor.prev, cursor.curr);
		assimilate_chunk(head, cursor.curr);
	} else {
		DEBUGSTR("SEPARATE");
		t_block_header*	new_free = head + blocks_needed + 1;
		*new_free = (t_block_header) {
			.blocks = blocks_capable - blocks_needed - 1,
			.next = cursor.curr->next,
		};
		head->blocks = blocks_needed;
		if (cursor.prev != NULL) {
			concat_item(cursor.prev, new_free);
		} else {
			zone->frees = new_free;
		}
		nullify_chunk(cursor.curr);
	}
}

void	_yo_shrink_chunk(t_block_header* head, size_t n) {
	assert(!GET_IS_LARGE(head->next));

	size_t	blocks_needed = BLOCKS_FOR_SIZE(n);
	assert(head->blocks >= blocks_needed);
	if (head->blocks - blocks_needed < 2) {
		DEBUGOUT("** MAINTAIN(%zu, %p) **", head->blocks, head);
		return;
	}
	DEBUGOUT("shrinking chunk %zu blocks -> %zu blocks", head->blocks, blocks_needed);
	t_block_header	*new_free = head + blocks_needed + 1;
	*new_free = (t_block_header) {
		.blocks	= head->blocks - (blocks_needed + 1),
		.next	= COPYFLAGS(NULL, head->next),
	};
	DEBUGOUT("new_free: (%zu, %p, %p)", new_free->blocks, new_free, new_free->next);
	head->blocks = blocks_needed;
	yo_free_actual(new_free + 1);
}
