#include "yo_internal.h"

extern t_yo_malloc_root	g_root;

// 要求サイズに対応するメモリゾーンクラスを返す
t_yo_zone_class	yo_zone_for_bytes(size_t n) {
	size_t	block_needed = BLOCKS_FOR_SIZE(n);
	if (block_needed > SMALL_MAX_CHUNK_BLOCK) {
		DEBUGWARN("zone for %zu B: LARGE", n);
		return YO_ZONE_LARGE;
	} else if (block_needed > TINY_MAX_CHUNK_BLOCK) {
		DEBUGWARN("zone for %zu B: SMALL", n);
		return YO_ZONE_SMALL;
	}
	DEBUGWARN("zone for %zu B: TINY", n);
	return YO_ZONE_TINY;
}

static t_yo_zone	init_zone(t_yo_zone_class zone, size_t max_chunk_bytes, size_t max_heap_bytes) {
	t_yo_zone	z = {
		.zone_class = zone,
		.max_chunk_bytes = max_chunk_bytes,
		.heap_bytes = QUANTIZE(max_heap_bytes, QUANTIZE(getpagesize(), BLOCK_UNIT_SIZE)),
		.frees = NULL,
		.free_p = NULL,
		.cons = {},
	};
	z.heap_blocks = (z.heap_bytes - 1) / BLOCK_UNIT_SIZE;
	z.frees = yo_allocate_heap(z.heap_blocks, zone);
	z.free_p = z.frees;
	if (z.frees != NULL) {
		z.cons.total_blocks += z.frees->blocks + 1;
	}
	return z;
}

// メモリゾーンクラスに対応するメモリゾーン(へのポインタ)を返す
t_yo_zone*		yo_retrieve_zone_for_class(t_yo_zone_class zone) {
	switch (zone) {
		case YO_ZONE_TINY: {
			if (g_root.tiny.max_chunk_bytes == 0) {
				g_root.tiny = init_zone(zone, TINY_MAX_CHUNK_BYTE, TINY_MAX_HEAP_BYTE);
			}
			return &g_root.tiny;
		}

		case YO_ZONE_SMALL: {
			if (g_root.small.max_chunk_bytes == 0) {
				g_root.small = init_zone(zone, SMALL_MAX_CHUNK_BYTE, SMALL_MAX_HEAP_BYTE);
			}
			return &g_root.small;
		}
		default: {
			DEBUGERR("SOMETHING WRONG: %d", zone);
			assert(0);
			return NULL;
		}
	}
}

// アドレス(mallocが返したポインタの値と仮定)に対応するメモリゾーン(へのポインタ)を返す
t_yo_zone_class		yo_zone_for_addr(void *addr) {
	t_block_header	*head = addr;
	--head;
	void	*flags = head->next;
	if (GET_IS_LARGE(flags)) {
		DEBUGWARN("zone for addr %p: LARGE, flags: %p", addr, flags);
		return YO_ZONE_LARGE;
	} else if (GET_IS_TINY(flags)) {
		DEBUGWARN("zone for addr %p: TINY, flags: %p", addr, flags);
		return YO_ZONE_TINY;
	}
	DEBUGWARN("zone for addr %p: SMALL, flags: %p", addr, flags);
	return YO_ZONE_SMALL;
}


// n + 1 個分のブロックを mmap で確保して返す
// 失敗した場合は NULL が返る
void	*yo_allocate_heap(size_t n, t_yo_zone_class zone) {
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
	*header = (t_block_header) {
		.blocks = n,
		.next = set_for_zone(NULL, zone),
	};
	DEBUGOUT("allocated %zu blocks for addr %p", n, mapped);
	return mapped;
}

void	yo_free_large_chunk(t_block_header *head) {
	remove_item(&g_root.large, head);
	size_t bytes = (head->blocks + 1) * BLOCK_UNIT_SIZE;
	if (munmap(head, bytes) < 0) {
		// failed
		DEBUGOUT("FAILED: errno = %d, %s", errno, strerror(errno));
		return;
	}
	return;
}

void	*yo_large_malloc(size_t n) {
	size_t	blocks_needed = BLOCKS_FOR_SIZE(n);
	DEBUGOUT("** bytes: B:%zu, blocks: %zu **", n, blocks_needed);
	t_block_header	*head = yo_allocate_heap(blocks_needed, YO_ZONE_LARGE);
	if (head == NULL) {
		return NULL;
	}
	*head = (t_block_header) {
		.blocks = blocks_needed,
		.next = set_for_zone(NULL, YO_ZONE_LARGE),
	};
	DEBUGWARN("(1) head->next = %p", head->next);
	insert_item(&g_root.large, head);
	DEBUGWARN("(2) head->next = %p", head->next);
	return head + 1;
}
