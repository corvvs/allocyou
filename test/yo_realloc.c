#include "yo_internal.h"

// n バイトが入るチャンクを新しく確保し, チャンク head の内容をコピーし, head を破棄する.
// 新しいチャンクを返す.
void*	_yo_relocate_chunk(t_block_header* head, size_t n) {
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
	// フリーチャンクは隣接していない(隣接しているなら合体している)はずなので,
	// 1つ後のフリーチャンクだけを調べればよい
	t_block_header*	right = head + head->blocks + 1;
	t_block_header*	prev = find_inf_item(zone->frees, right);
	t_block_header*	cand = prev == NULL ? zone->frees : ADDRESS(prev->next);
	if (cand == NULL || cand != right) {
		DEBUGSTR("no candidate free chunk");
		return NULL;
	}
	size_t	blocks_needed = BLOCKS_FOR_SIZE(n);
	size_t	whole_blocks = head->blocks + cand->blocks + 2;
	// 保存量
	// 前チャンクのブロック + 1 + 後チャンクのブロック + 1 = whole_blocks = const.
	if (whole_blocks < blocks_needed + 1) {
		DEBUGWARN("candidate chunk %p has not enough blocks: %zu", cand, cand->blocks);
		return NULL;
	}
	DEBUGOUT("candidate chunk %p has not enough blocks: %zu", cand, cand->blocks);
	t_block_header*	new_free = head + blocks_needed + 1;
	new_free->blocks = whole_blocks - 2 - blocks_needed;
	new_free->next = NULL;
	head->blocks = blocks_needed;
	remove_item(&zone->frees, cand);
	insert_item(&zone->frees, new_free);
	DEBUGSTR("** EXTENDED **");
	return head;
}

void*	_yo_shrink_chunk(t_block_header* head, size_t n) {
	if (GET_IS_LARGE(head->next)) {
		// LARGEの時はなにもしない
		DEBUGSTR("do nothing for large chunk");
		return head + 1;
	}

	size_t	blocks_needed = BLOCKS_FOR_SIZE(n);
	assert(head->blocks >= blocks_needed);
	if (head->blocks - blocks_needed >= 2) {
		DEBUGOUT("shrinking chunk %zu blocks -> %zu blocks", head->blocks, blocks_needed);
		// shrink することで有効な空きチャンクができるなら shrink する
		// -> head から blocks_needed + 1 ブロック後ろに新しいブロックヘッダを作る 
		head->blocks = blocks_needed;
		t_block_header	*free_head = head + head->blocks + 1;
		free_head->blocks = head->blocks - blocks_needed - 1;
		free_head->next = NULL;
		yo_free_actual(free_head + 1);
	} else {
		DEBUGOUT("** MAINTAIN(%zu, %p) **", head->blocks, head);
	}
	return head + 1;
}
