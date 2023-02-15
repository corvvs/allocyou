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


typedef struct s_yo_malloc_root {
	t_block_header*	frees;
	t_block_header*	allocated;
	t_block_header* large;
}	t_yo_malloc_root;

// 1ブロックのサイズ
// 16Bであると思っておく
#define BLOCK_UNIT_SIZE (QUANTIZE(sizeof(t_block_header), sizeof(size_t)))
// 1度にmmapする単位.
// BLOCK_UNIT_SIZE をかけるとバイトサイズになる.
// ※実際にmmapするのはこれより1つ多くなる

#endif
