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

// 
void*	_yo_try_extend_chunk(t_yo_zone* zone, t_block_header* head, size_t n) {
	// フリーチャンクが隣接していない(隣接しているなら合体している)ことを仮定すると,
	// 1つ後のフリーチャンクだけを調べればよい
	t_block_header*	right_free = head + head->blocks + 1;
	t_listcursor cursor = find_fit_cursor(zone->frees, right_free);
	if (cursor.curr == NULL) {
		DEBUGSTR("no RIGHT FREE chunk");
		return NULL;
	}
	size_t	blocks_needed = BLOCKS_FOR_SIZE(n);
	size_t	whole_blocks = head->blocks + right_free->blocks + 2;
	// 保存量
	// 前チャンクのブロック + 1 + 後チャンクのブロック + 1 = whole_blocks = const.
	if (whole_blocks < blocks_needed + 1) {
		DEBUGWARN("right_free chunk %p has not enough blocks: %zu", right_free, right_free->blocks);
		return NULL;
	}
	DEBUGOUT("candidate chunk %p has not enough blocks: %zu", right_free, right_free->blocks);
	t_block_header*	new_free = head + blocks_needed + 1;
	new_free->blocks = whole_blocks - 2 - blocks_needed;
	new_free->next = NULL;
	head->blocks = blocks_needed;
	remove_item(&zone->frees, right_free);
	insert_item(&zone->frees, new_free);
	DEBUGSTR("** EXTENDED **");
	return head;
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
	head->blocks = blocks_needed;
	t_block_header	*new_free = head + blocks_needed + 1;
	*new_free = (t_block_header) {
		.blocks	= head->blocks - (blocks_needed + 1),
		.next	= NULL,
	};
	yo_free_actual(new_free + 1);
}
