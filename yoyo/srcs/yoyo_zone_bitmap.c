#include "yoyo_internal.h"

// このブロックが chunk のヘッダかどうか
bool	is_head(const t_yoyo_zone* zone, unsigned int block_index) {
	unsigned int	byte_index = block_index / 8;
	unsigned int	bit_index = block_index % 8;
	unsigned char*	heads = (void*)zone + zone->offset_bitmap_heads;
	return !!(heads[byte_index] & (1 << bit_index));
}

// このブロックがヘッダになっている chunk が使用中かどうか
bool	is_used(const t_yoyo_zone* zone, unsigned int block_index) {
	unsigned int	byte_index = block_index / 8;
	unsigned int	bit_index = block_index % 8;
	unsigned char*	used = (void*)zone + zone->offset_bitmap_used;
	return !!(used[byte_index] & (1 << bit_index));
}
