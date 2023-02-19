#ifndef YO_STRUCT_H
# define YO_STRUCT_H

# include <stdlib.h>
# include "yo_utils.h"

/**
 * ブロックヘッダ
 * ブロックヘッダの先頭から (blocks + 1) * BLOCK_UNIT_SIZE バイト分の領域を
 * 「一続きのブロックのあつまり」という意味で チャンク と呼ぶ.
 */
typedef struct s_block_header {
	// 次のブロックヘッダへのポインタ
	// next の下3ビットはアドレスでない情報に使うので,
	// next をアドレスとして用いる場合は下3ビットを落とすこと(ADDRESSマクロを使う)
	struct s_block_header*	next;

	// このチャンクの長さ
	size_t					blocks;
} t_block_header;

// n in PDF
# define TINY_MAX_CHUNK_BYTE ((size_t)992)
# define TINY_MAX_CHUNK_BLOCK (BLOCKS_FOR_SIZE(TINY_MAX_CHUNK_BYTE))
// m in PDF
# define SMALL_MAX_CHUNK_BYTE ((size_t)15360)
# define SMALL_MAX_CHUNK_BLOCK (BLOCKS_FOR_SIZE(SMALL_MAX_CHUNK_BYTE))

// N in PDF
# define TINY_MAX_HEAP_BYTE ((size_t)1048576)
// M in PDF
# define SMALL_MAX_HEAP_BYTE ((size_t)8388608)

typedef struct s_yo_zone {
	size_t			max_chunk_bytes;
	size_t			heap_bytes;
	size_t			heap_blocks;
	t_block_header*	frees;
	t_block_header*	allocated;
}	t_yo_zone;

typedef struct s_yo_malloc_root {
	t_yo_zone		tiny;
	t_yo_zone		small;
	t_block_header*	large;
}	t_yo_malloc_root;

// 1ブロックのサイズ
// 16Bであると思っておく
#define BLOCK_UNIT_SIZE (QUANTIZE(sizeof(t_block_header), sizeof(size_t)))
// 1度にmmapする単位.
// BLOCK_UNIT_SIZE をかけるとバイトサイズになる.
// ※実際にmmapするのはこれより1つ多くなる

#endif
