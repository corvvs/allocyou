#include "internal.h"

static void	*yoyo_memcpy(void* dst, const void* src, size_t n) {
	unsigned char*			ud;
	const unsigned char*	us;

	ud = dst;
	us = src;
	while (n--)
		*ud++ = *us++;
	return dst;
}

// 最低 blocks_required ブロックあるチャンクを新しく確保し,
// 使用中チャンク chunk の内容をそこにコピーし,
// chunk を解放する.
static void*	relocate_chunk(t_yoyo_chunk* chunk, size_t blocks_required) {
	// [引っ越し先チャンクを確保]
	DEBUGINFO("ALLOCATE: (%p, %zu) -> %zu", chunk, chunk->blocks, blocks_required);
	void*	relocated = yoyo_actual_malloc((blocks_required - 1) * BLOCK_UNIT_SIZE);
	DEBUGINFO("RELOCATED: (%p, %zu) -> %p", chunk, chunk->blocks, relocated);
	if (relocated == NULL) {
		return NULL;
	}
	// [データをコピー]
	t_yoyo_chunk*	chunk_relocated = relocated - CEILED_CHUNK_SIZE;
	DEBUGINFO("COPY: (%p, %zu) -> (%p, %zu)", chunk, chunk->blocks, chunk_relocated, chunk_relocated->blocks);
	DEBUGOUT("chunk: %p", chunk);
	DEBUGOUT("chunk_relocated: %p", chunk_relocated);
	DEBUGOUT("chunk_relocated->blocks: %zu, chunk->blocks: %zu", chunk_relocated->blocks, chunk->blocks);
	const size_t	blocks_copy = ((chunk_relocated->blocks < chunk->blocks) ? chunk_relocated->blocks : chunk->blocks) - 1;
	void*			addr_current = (void*)chunk + CEILED_CHUNK_SIZE;
	DEBUGINFO("yoyo_memcpy(%p, %p, %zu)", relocated, addr_current, blocks_copy * BLOCK_UNIT_SIZE);
	yoyo_memcpy(relocated, addr_current, blocks_copy * BLOCK_UNIT_SIZE);
	// [現在のチャンクを解放]
	DEBUGINFO("DEALLOCATE: (%p, %zu)", chunk, chunk->blocks);
	yoyo_actual_free(addr_current);
	return relocated;
}

// 使用中チャンク chunk を, 先頭アドレスを変えないで blocks_required ブロックに縮小し,
// 残ったブロックからフリーチャンクを生成する.
static void	shrink_chunk(t_yoyo_chunk* chunk, size_t blocks_required) {
	DEBUGOUT("chunk: %p, chunk->blocks: %zu", chunk, chunk->blocks);
	YOYO_ASSERT(2 <= blocks_required);
	YOYO_ASSERT(blocks_required < chunk->blocks);
	t_yoyo_zone*	zone_current = get_zone_of_chunk(chunk);
	if (zone_current == NULL) {
		return;
	}
	YOYO_ASSERT(zone_current->zone_type != YOYO_ZONE_LARGE);
	t_yoyo_chunk*	chunk_new_free = (void *)chunk + blocks_required * BLOCK_UNIT_SIZE;
	chunk_new_free->blocks = chunk->blocks - blocks_required;
	YOYO_ASSERT(2 <= chunk_new_free->blocks);
	chunk_new_free->next = COPY_FLAGS(NULL, chunk->next);
	// ここからロックが必要
	if (!lock_zone(zone_current)) {
		return;
	}
	chunk->blocks = blocks_required;
	yoyo_free_from_locked_tiny_small_zone(zone_current, chunk_new_free);
	if (!unlock_zone(zone_current)) {
		return;
	}
	return;
}

// 実際の realloc の動作をする
void*	yoyo_actual_realloc(void* addr, size_t n) {
	if (addr == NULL) {
		DEBUGWARN("addr == NULL -> delegate to malloc(%zu)", n);
		return yoyo_actual_malloc(n);
	}
	YOYO_ASSERT(CEILED_CHUNK_SIZE <= (size_t)addr);
	if (CEILED_CHUNK_SIZE > (size_t)addr) {
		DEBUGFATAL("addr is TOO LOW: %p", addr);
		return NULL;
	}
	t_yoyo_chunk*			chunk = addr - CEILED_CHUNK_SIZE;
	const size_t			blocks_needed = BLOCKS_FOR_SIZE(n);
	const size_t			blocks_required = blocks_needed + 1;
	if (blocks_needed == 0 || blocks_required == 0) {
		errno = ENOMEM;
		return NULL;
	}
	const size_t			blocks_current = chunk->blocks;
	const t_yoyo_zone_type	type_required = zone_type_for_bytes(n);
	const t_yoyo_zone_type	type_current = zone_type_for_bytes(blocks_current * BLOCK_UNIT_SIZE);

	DEBUGOUT("for (%p, %zu): blocks_required: %zu (%d) <? blocks_current: %zu (%d)", addr, n, blocks_required, type_required, blocks_current, type_current);
	// ゾーン種別が変わる場合は RELOCATE
	if (type_required != type_current) {
		DEBUGWARN("for (%p, %zu B): EXEC RELOCATE", addr, n);
		return relocate_chunk(chunk, blocks_required);
	}
	// SHRINK できるか？
	// - 要求ブロックサイズ < 現在ブロックサイズ
	// - 要求ゾーンタイプ = 現在ゾーンタイプ
	// - 現在ゾーンタイプ != LARGE
	// - 新しく生成されるフリーチャンクのブロックサイズが2以上
	if (type_current != YOYO_ZONE_LARGE && blocks_required < blocks_current) {
		const size_t	blocks_new_free = blocks_current - blocks_required;
		DEBUGOUT("blocks_new_free: %zu", blocks_new_free);
		const t_yoyo_zone_type	type_new_free = zone_type_for_bytes(blocks_new_free * BLOCK_UNIT_SIZE);
		if (blocks_new_free >= 2 && type_new_free == type_current) {
			// SHRINK できる.
			DEBUGWARN("for (%p, %zu): EXEC SHRINK", addr, n);
			shrink_chunk(chunk, blocks_required);
			return addr;
		}
	}
	// MAINTAIN できるか？
	// - 要求ブロックサイズ ≤ 現在ブロックサイズ
	// - 要求ゾーンタイプ = 現在ゾーンタイプ
	if (blocks_required <= blocks_current) {
		DEBUGWARN("for (%p, %zu): EXEC MAINTAIN", addr, n);
		return addr;
	}
	// RELOCATE
	return relocate_chunk(chunk, blocks_required);
}
