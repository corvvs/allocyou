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
	DEBUGINFO("%s", "ALLOCATE");
	void*	relocated = yoyo_actual_malloc((blocks_required - 1) * BLOCK_UNIT_SIZE);
	if (relocated == NULL) {
		return NULL;
	}
	// [データをコピー]
	DEBUGINFO("%s", "COPY");
	t_yoyo_chunk*	chunk_relocated = relocated - CEILED_CHUNK_SIZE;
	DEBUGOUT("chunk: %p", chunk);
	DEBUGOUT("chunk_relocated: %p", chunk_relocated);
	DEBUGOUT("chunk_relocated->blocks: %zu, chunk->blocks: %zu", chunk_relocated->blocks, chunk->blocks);
	const size_t	blocks_copy = ((chunk_relocated->blocks < chunk->blocks) ? chunk_relocated->blocks : chunk->blocks) - 1;
	void*			addr_current = (void*)chunk + CEILED_CHUNK_SIZE;
	DEBUGINFO("yoyo_memcpy(%p, %p, %zu)", relocated, addr_current, blocks_copy * BLOCK_UNIT_SIZE);
	yoyo_memcpy(relocated, addr_current, blocks_copy * BLOCK_UNIT_SIZE);
	// [現在のチャンクを解放]
	DEBUGINFO("%s", "DEALLOCATE");
	yoyo_actual_free(addr_current);
	return relocated;
}

// 使用中チャンク chunk を, 先頭アドレスを変えないで blocks_required ブロックに縮小し,
// 残ったブロックからフリーチャンクを生成する.
static void	shrink_chunk(t_yoyo_chunk* chunk, size_t blocks_required) {
	DEBUGOUT("chunk: %p, chunk->blocks: %zu", chunk, chunk->blocks);
	assert(2 <= blocks_required);
	assert(blocks_required < chunk->blocks);
	t_yoyo_zone*	zone_current = get_zone_of_chunk(chunk);
	if (zone_current == NULL) {
		return;
	}
	assert(zone_current->zone_type != YOYO_ZONE_LARGE);
	t_yoyo_chunk*	chunk_new_free = (void *)chunk + blocks_required * BLOCK_UNIT_SIZE;
	chunk_new_free->blocks = chunk->blocks - blocks_required;
	assert(2 <= chunk_new_free->blocks);
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
	assert(CEILED_CHUNK_SIZE <= (size_t)addr);
	t_yoyo_chunk*	chunk = addr - CEILED_CHUNK_SIZE;

	const size_t			blocks_required = BLOCKS_FOR_SIZE(n) + 1;
	const size_t			blocks_current = chunk->blocks;
	const t_yoyo_zone_type	type_required = zone_type_for_bytes(n);
	const t_yoyo_zone_type	type_current = zone_type_for_bytes(blocks_current * BLOCK_UNIT_SIZE);

	DEBUGOUT("blocks_required: %zu <? blocks_current: %zu", blocks_required, blocks_current);
	// ゾーン種別が変わる場合は RELOCATE
	if (type_required != type_current) {
		DEBUGWARN("%s", "EXEC RELOCATE");
		return relocate_chunk(chunk, blocks_required);
	}
	// SHRINK できるか？
	// - 要求ブロックサイズ < 現在ブロックサイズ
	// - 要求ゾーンタイプ = 現在ゾーンタイプ
	// - 新しく生成されるフリーチャンクのブロックサイズが2以上
	if (blocks_required < blocks_current) {
		const size_t	blocks_new_free = blocks_current - blocks_required;
		DEBUGOUT("blocks_new_free: %zu", blocks_new_free);
		const t_yoyo_zone_type	type_new_free = zone_type_for_bytes(blocks_new_free * BLOCK_UNIT_SIZE);
		if (blocks_new_free >= 2 && type_new_free == type_current) {
			// SHRINK できる.
			DEBUGWARN("%s", "EXEC SHRINK");
			shrink_chunk(chunk, blocks_required);
			return addr;
		}
	}
	// MAINTAIN できるか？
	// - 要求ブロックサイズ ≤ 現在ブロックサイズ
	// - 要求ゾーンタイプ = 現在ゾーンタイプ
	if (blocks_required <= blocks_current) {
		DEBUGWARN("%s", "EXEC MAINTAIN");
		return addr;
	}
	// RELOCATE
	return relocate_chunk(chunk, blocks_required);
}
