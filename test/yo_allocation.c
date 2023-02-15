#include "yo_internal.h"

extern t_yo_malloc_root	g_root;

// n + 1 個分のブロックを mmap で確保して返す
// 失敗した場合は NULL が返る
void	*_yo_allocate_heap(size_t n) {
	void	*mapped = mmap(
		NULL,
		(n + 1) * BLOCK_UNIT_SIZE,
		PROT_READ | PROT_WRITE,
		MAP_ANON | MAP_PRIVATE,
		-1, 0);
	if (mapped == MAP_FAILED) {
		return NULL;
	}
	// 確保できたら先頭にブロックヘッダを詰める
	t_block_header	*header = mapped;
	header->blocks = n;
	header->next = NULL;
	return mapped;
}

void	_yo_free_large_chunk(t_block_header *head) {
	DEBUGOUT("** addr: %p, block: %zu **\n", head, head->blocks);
	remove_item(&g_root.large, head);
	size_t bytes = (head->blocks + 1) * BLOCK_UNIT_SIZE;
	DEBUGOUT("bytes = %zu\n", bytes);
	if (munmap(head, bytes) < 0) {
		// failed
		DEBUGOUT("FAILED: errno = %d, %s\n", errno, strerror(errno));
		return;
	}
	return;
}

void	*_yo_large_malloc(size_t n) {
	size_t	blocks_needed = QUANTIZE(n, BLOCK_UNIT_SIZE) / BLOCK_UNIT_SIZE;
	DEBUGOUT("** bytes: B:%zu, blocks: %zu **\n", n, blocks_needed);
	t_block_header	*head = _yo_allocate_heap(blocks_needed);
	if (head == NULL) {
		return NULL;
	}
	head->blocks = blocks_needed;
	head->next = SET_IS_LARGE(NULL, 1);
	insert_item(&g_root.large, head);
	return head + 1;
}
