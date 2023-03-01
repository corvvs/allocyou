#include "yoyo_internal.h"

// まだ固まってない
void	mark_chunk_as_free(t_yoyo_zone* zone, t_yoyo_chunk* chunk) {
	const unsigned int	bi = get_block_index(zone, chunk);
	const unsigned int	byte_index = bi / 8;
	const unsigned int	bit_index = bi % 8;
	unsigned char*		heads = (void*)zone + zone->offset_bitmap_heads;
	unsigned char*		used = (void*)zone + zone->offset_bitmap_used;
	const unsigned char	mask = (1u << bit_index);
	heads[byte_index] |= mask;
	used[byte_index] &= ~mask;
}

void	mark_chunk_as_used(t_yoyo_zone* zone, t_yoyo_chunk* chunk) {
	const unsigned int	bi = get_block_index(zone, chunk);
	const unsigned int	byte_index = bi / 8;
	const unsigned int	bit_index = bi % 8;
	unsigned char*		heads = (void*)zone + zone->offset_bitmap_heads;
	unsigned char*		used = (void*)zone + zone->offset_bitmap_used;
	const unsigned char	mask = (1u << bit_index);
	heads[byte_index] |= mask;
	used[byte_index] |= mask;
}
