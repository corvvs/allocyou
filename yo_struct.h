#ifndef YO_STRUCT_H
# define YO_STRUCT_H

# include <stdlib.h>
# include <stdbool.h>
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

	// このチャンクのブロック数
	size_t					blocks;
} t_block_header;

typedef struct s_listcursor {
	t_block_header*	curr;
	t_block_header*	prev;

	t_block_header*	head;
	t_block_header*	start;
	bool			visited_once;
}	t_listcursor;

// n in PDF
# define TINY_MAX_CHUNK_BYTE ((size_t)992)
// n as blocks
# define TINY_MAX_CHUNK_BLOCK (BLOCKS_FOR_SIZE(TINY_MAX_CHUNK_BYTE))
// m in PDF
# define SMALL_MAX_CHUNK_BYTE ((size_t)15360)
// m as blocks
# define SMALL_MAX_CHUNK_BLOCK (BLOCKS_FOR_SIZE(SMALL_MAX_CHUNK_BYTE))

// N in PDF
# define TINY_MAX_HEAP_BYTE ((size_t)1048576)
// M in PDF
# define SMALL_MAX_HEAP_BYTE ((size_t)8388608)

typedef enum e_yo_zone_class {
	YO_ZONE_TINY,
	YO_ZONE_SMALL,
	YO_ZONE_LARGE,
}	t_yo_zone_class;

typedef struct s_yo_zone_consistency {
	// ヘッダも含めた当該ゾーンの全ブロック数
	// ヒープアロケートにより追加される.
	size_t			total_blocks;
	size_t			free_blocks;
	size_t			used_blocks;
}	t_yo_zone_consistency;

typedef struct s_yo_zone {
	// ゾーン種別
	t_yo_zone_class	zone_class;
	// このゾーンに割り当てられる最大の要求バイトサイズ
	size_t			max_chunk_bytes;
	// このゾーンのヒープのバイトサイズ
	size_t			heap_bytes;
	// このゾーンのヒープのブロックサイズ
	size_t			heap_blocks;
	// このゾーンのフリーリスト
	t_block_header*	frees;
	t_block_header*	free_p;
	// このゾーンのアロケーションリスト
	t_block_header*	allocated;
	// 整合性情報
	t_yo_zone_consistency	cons;
}	t_yo_zone;

typedef struct s_yo_malloc_root {
	t_yo_zone		tiny;
	t_yo_zone		small;
	t_yo_zone		large;
}	t_yo_malloc_root;

// 1ブロックのサイズ
// 16Bであると思っておく
#define BLOCK_UNIT_SIZE (QUANTIZE(sizeof(t_block_header), sizeof(size_t)))
// 1度にmmapする単位.
// BLOCK_UNIT_SIZE をかけるとバイトサイズになる.
// ※実際にmmapするのはこれより1つ多くなる

#endif
