#include "yo_internal.h"

extern t_yo_malloc_root	g_root;

// 要求サイズに対応するメモリゾーンクラスを返す
t_yo_zone_class	_yo_zone_for_bytes(size_t n) {
	size_t	block_needed = BLOCKS_FOR_SIZE(n);
	if (block_needed > SMALL_MAX_CHUNK_BLOCK) {
		DEBUGWARN("zone LARGE for %zu B", n);
		return YO_ZONE_LARGE;
	} else if (block_needed > TINY_MAX_CHUNK_BLOCK) {
		DEBUGWARN("zone SMALL for %zu B", n);
		return YO_ZONE_SMALL;
	}
	DEBUGWARN("zone TINY for %zu B", n);
	return YO_ZONE_TINY;
}

// メモリゾーンクラスに対応するメモリゾーン(へのポインタ)を返す
t_yo_zone*		_yo_retrieve_zone_for_class(t_yo_zone_class zone) {
	switch (zone) {
		case YO_ZONE_TINY: {
			if (g_root.tiny.max_chunk_bytes == 0) {
				g_root.tiny.max_chunk_bytes = TINY_MAX_CHUNK_BYTE;
				g_root.tiny.heap_bytes = QUANTIZE(TINY_MAX_HEAP_BYTE, QUANTIZE(getpagesize(), BLOCK_UNIT_SIZE));
				g_root.tiny.heap_blocks = (g_root.tiny.heap_bytes - 1) / BLOCK_UNIT_SIZE;
			}
			return &g_root.tiny;
		}

		case YO_ZONE_SMALL: {
			if (g_root.small.max_chunk_bytes == 0) {
				g_root.small.max_chunk_bytes = SMALL_MAX_CHUNK_BYTE;
				g_root.small.heap_bytes = QUANTIZE(SMALL_MAX_HEAP_BYTE, QUANTIZE(getpagesize(), BLOCK_UNIT_SIZE));
				g_root.small.heap_blocks = (g_root.small.heap_bytes - 1) / BLOCK_UNIT_SIZE;
			}
			return &g_root.small;
		}
		default: {
			DEBUGERR("SOMETHING WRONG: %d", zone);
			return NULL;
		}
	}
}

// アドレス(mallocが返したポインタの値と仮定)に対応するメモリゾーン(へのポインタ)を返す
t_yo_zone_class		_yo_zone_for_addr(void *addr) {
	t_block_header	*head = addr;
	--head;
	void	*flags = head->next;
	if (GET_IS_LARGE(flags)) {
		DEBUGWARN("zone LARGE for addr %p, flags: %p", addr, flags);
		return YO_ZONE_LARGE;
	} else if (GET_IS_TINY(flags)) {
		DEBUGWARN("zone TINY for addr %p, flags: %p", addr, flags);
		return YO_ZONE_TINY;
	}
	DEBUGWARN("zone SMALL for addr %p, flags: %p", addr, flags);
	return YO_ZONE_SMALL;
}


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
	DEBUGOUT("** addr: %p, block: %zu **", head, head->blocks);
	remove_item(&g_root.large, head);
	size_t bytes = (head->blocks + 1) * BLOCK_UNIT_SIZE;
	DEBUGOUT("bytes = %zu", bytes);
	if (munmap(head, bytes) < 0) {
		// failed
		DEBUGOUT("FAILED: errno = %d, %s", errno, strerror(errno));
		return;
	}
	return;
}

void	*_yo_large_malloc(size_t n) {
	size_t	blocks_needed = BLOCKS_FOR_SIZE(n);
	DEBUGOUT("** bytes: B:%zu, blocks: %zu **", n, blocks_needed);
	t_block_header	*head = _yo_allocate_heap(blocks_needed);
	if (head == NULL) {
		return NULL;
	}
	head->blocks = blocks_needed;
	head->next = SET_IS_LARGE(NULL);
	DEBUGWARN("(1) head->next = %p", head->next);
	insert_item(&g_root.large, head);
	DEBUGWARN("(2) head->next = %p", head->next);
	return head + 1;
}
