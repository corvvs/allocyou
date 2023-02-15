#include "yo_internal.h"

// n バイトが入るチャンクを新しく確保し, チャンク head の内容をコピーし, head を破棄する.
// 新しいチャンクを返す.
void*	_yo_relocate_chunk(t_block_header* head, size_t n) {
	void*	new_mem = yo_malloc(n);
	if (new_mem == NULL) {
		DEBUGOUT("malloc(%zu) failed\n", n);
		return NULL;
	}
	t_block_header*	new_head = new_mem;
	--new_head;
	DEBUGOUT("yo_memcpy(%p, %p, %zu)\n", new_head + 1, head + 1, head->blocks * BLOCK_UNIT_SIZE);
	yo_memcpy(new_head + 1, head + 1, head->blocks * BLOCK_UNIT_SIZE);
	DEBUGOUT("freeing %p\n", head);
	yo_free(head);
	return new_mem;
}

// 
void*	_yo_try_extend_chunk(t_block_header* head, size_t n) {
	// フリーチャンクは隣接していない(隣接しているなら合体している)はずなので,
	// 1つ後のフリーチャンクだけを調べればよい
	t_block_header*	right = head + head->blocks + 1;
	t_block_header*	prev = find_item(g_root.frees, right);
	t_block_header*	cand = prev == NULL ? g_root.frees : ADDRESS(prev->next);
	if (cand == NULL || cand != right) {
		DEBUGSTR("no candidate free chunk\n");
		return NULL;
	}
	size_t	blocks_needed = BLOCKS_FOR_SIZE(n);
	size_t	whole_blocks = head->blocks + cand->blocks + 2;
	// 保存量
	// 前チャンクのブロック + 1 + 後チャンクのブロック + 1 = whole_blocks = const.
	if (whole_blocks < blocks_needed + 1) {
		DEBUGWARN("candidate chunk %p has not enough blocks: %zu\n", cand, cand->blocks);
		return NULL;
	}
	DEBUGOUT("candidate chunk %p has not enough blocks: %zu\n", cand, cand->blocks);
	t_block_header*	new_free = head + blocks_needed + 1;
	new_free->blocks = whole_blocks - 2 - blocks_needed;
	new_free->next = NULL;
	head->blocks = blocks_needed;
	remove_item(&g_root.frees, cand);
	insert_item(&g_root.frees, new_free);
	DEBUGSTR("** EXTENDED **\n");
	return head;
}

void*	_yo_shrink_chunk(t_block_header* head, size_t n) {
	if (GET_IS_LARGE(head->next)) {
		// LARGEの時はなにもしない
		DEBUGSTR("do nothing for large chunk\n");
		return head + 1;
	}

	size_t	blocks_needed = BLOCKS_FOR_SIZE(n);
	assert(head->blocks >= blocks_needed);
	if (head->blocks - blocks_needed >= 2) {
		DEBUGOUT("shrinking chunk %zu blocks -> %zu blocks\n", head->blocks, blocks_needed);
		// shrink することで有効な空きチャンクができるなら shrink する
		// -> head から blocks_needed + 1 ブロック後ろに新しいブロックヘッダを作る 
		head->blocks = blocks_needed;
		t_block_header	*free_head = head + head->blocks + 1;
		free_head->blocks = head->blocks - blocks_needed - 1;
		free_head->next = NULL;
		yo_free(free_head + 1);
	} else {
		DEBUGOUT("** MAINTAIN(%zu, %p) **\n", head->blocks, head);
	}
	return head + 1;
}
