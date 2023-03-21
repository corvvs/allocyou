#include "internal.h"

bool	check_is_free(t_yoyo_zone* zone, t_yoyo_chunk* chunk) {
	unsigned int	bi = get_block_index(zone, chunk);
	(void)bi;
	assert(is_head(zone, bi));
	assert(!is_used(zone, bi));
	return is_head(zone, bi) && !is_used(zone, bi);
}

bool	check_is_used(t_yoyo_zone* zone, t_yoyo_chunk* chunk) {
	unsigned int	bi = get_block_index(zone, chunk);
	(void)bi;
	assert(is_head(zone, bi));
	assert(is_used(zone, bi));
	return is_head(zone, bi) && is_used(zone, bi);
}

// このブロックが chunk のヘッダかどうか
bool	is_head(const t_yoyo_zone* zone, unsigned int block_index) {
	unsigned int	byte_index = block_index / CHAR_BIT;
	unsigned int	bit_index = block_index % CHAR_BIT;
	unsigned char*	heads = (void*)zone + zone->offset_bitmap_heads;
	return !!(heads[byte_index] & (1 << bit_index));
}

// このブロックがヘッダになっている chunk が使用中かどうか
bool	is_used(const t_yoyo_zone* zone, unsigned int block_index) {
	unsigned int	byte_index = block_index / CHAR_BIT;
	unsigned int	bit_index = block_index % CHAR_BIT;
	unsigned char*	used = (void*)zone + zone->offset_bitmap_used;
	return !!(used[byte_index] & (1 << bit_index));
}

// block_index に対応する位置に chunk ヘッダなら, それを返す.
t_yoyo_chunk*	get_chunk_by_index(t_yoyo_zone* zone, unsigned int block_index) {
	assert(block_index < zone->blocks_heap);
	if (!is_head(zone, block_index)) {
		return NULL;
	}
	return (void*)zone + zone->offset_heap + block_index * BLOCK_UNIT_SIZE;
}

// 対象アドレスがchunkヘッダであること, 使用中であることを確認する
bool	is_header_and_used(const t_yoyo_zone* zone, const t_yoyo_chunk* chunk) {
	const unsigned int	bi = get_block_index(zone, chunk);
	return is_head(zone, bi) && is_used(zone, bi);
}
