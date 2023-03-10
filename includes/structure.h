#ifndef YOYO_STRUCTURE_H
# define YOYO_STRUCTURE_H

# include <stdlib.h>
# include <stdbool.h>
# include <pthread.h>

# define ARENA_MAX	5
// a を bの倍数に切り上げる
# define CEIL_BY(a, b) (a ? ((a - 1) / b + 1) * b : b)
// a を bの倍数に切り下げる
# define FLOOR_BY(a, b) (a / b * b)

typedef enum e_yoyo_zone_type {
	YOYO_ZONE_TINY,
	YOYO_ZONE_SMALL,
	YOYO_ZONE_LARGE,
}	t_yoyo_zone_type;

typedef unsigned char	t_bitfield;

// [chunk ヘッダ構造体]
// chunk ヘッダ t_yoyo_chunk の後にユーザが使用可能な領域が続く.
// 
//         t_yoyo_chunk        |    (chunk body)     
// +-------------+-------------+---------//---------+
// |    blocks   |     next    |                    |
// +-------------+-------------+---------//---------+
typedef struct	s_yoyo_chunk {
	// chunk のブロック数(ヘッダ含む)
	size_t					blocks;
	// フリーリストにおける次の chunk への参照.
	// (末尾3ビットにはフラグがエンコードされている)
	struct s_yoyo_chunk*	next;
}	t_yoyo_chunk;

// [LARGE chunk ヘッダ構造体]
// LARGE chunk ヘッダ t_yoyo_large_chunk の後ろに通常の chunk が続く形になっている.
// 
//             t_yoyo_large_chunk          |       t_yoyo_chunk        |    (chunk body)     
// +------------+-------------+------------+-------------+-------------+---------//---------+
// |  subarena  | memory_byte | large_next |    blocks   |     next    |                    |
// +------------+-------------+------------+-------------+-------------+---------//---------+
typedef struct s_yoyo_large_chunk {
	// この chunk が所属する LARGE subarena への参照
	void*						subarena;
	// mmap された正味のバイト数.
	// この値は munmap だけに使うこと.
	size_t						memory_byte;
	// LARGE chunk の使用中リストにおける次の chunk への参照
	struct s_yoyo_large_chunk*	large_next;
}	t_yoyo_large_chunk;

# define BLOCK_UNIT_SIZE (CEIL_BY(sizeof(t_yoyo_chunk), sizeof(size_t)))
# define BLOCKS_FOR_SIZE(n) (CEIL_BY(n, BLOCK_UNIT_SIZE) / BLOCK_UNIT_SIZE)

# define CEILED_LARGE_CHUNK_SIZE CEIL_BY(sizeof(t_yoyo_large_chunk), BLOCK_UNIT_SIZE)
# define CEILED_CHUNK_SIZE CEIL_BY(sizeof(t_yoyo_chunk), BLOCK_UNIT_SIZE)
# define LARGE_OFFSET_USABLE (CEILED_LARGE_CHUNK_SIZE + CEILED_CHUNK_SIZE)

// n in PDF
# define TINY_MAX_CHUNK_BYTE ((size_t)992)
// n as blocks
# define TINY_MAX_CHUNK_BLOCK (BLOCKS_FOR_SIZE(TINY_MAX_CHUNK_BYTE))
// m in PDF
# define SMALL_MAX_CHUNK_BYTE ((size_t)15360)
// m as blocks
# define SMALL_MAX_CHUNK_BLOCK (BLOCKS_FOR_SIZE(SMALL_MAX_CHUNK_BYTE))

// N in PDF
# define ZONE_TINY_BYTE ((size_t)(1024 * 1024))
// M in PDF
# define ZONE_SMALL_BYTE ((size_t)(8 * 1024 * 1024))

typedef unsigned int block_index_t;

// [TINY/SMALL zone 構造体]
// ただし固定長の部分のみ. ビット配列部分はここに入っていない.
typedef struct	s_yoyo_zone {
	// 次の zone
	struct s_yoyo_zone*	next;

	// zone ロック
	pthread_mutex_t		lock;

	// マルチスレッドモードかどうか
	bool				multi_thread;

	// ゾーン種別
	t_yoyo_zone_type	zone_type;

	// free リスト
	t_yoyo_chunk*		frees;

	// previous free
	t_yoyo_chunk*		free_prev;

	// zone 全体のブロック数
	unsigned int		blocks_zone;
	// zone のうち heap 部分のブロック数
	unsigned int		blocks_heap;

	// heap のうち使用されていないブロック数
	unsigned int		blocks_free;

	// heap のうち使用されているブロック数
	unsigned int		blocks_used;

	// blocks_heap = blocks_free + blocks_used

	// zone 先頭から heads ビット配列へのバイトオフセット
	// heads ビット配列のバイト数は ceil(blocks_heap / 8)
	unsigned int		offset_bitmap_heads;

	// zone 先頭から used ビット配列へのバイトオフセット
	// used ビット配列のバイト数は ceil(blocks_heap / 8)
	unsigned int		offset_bitmap_used;

	// zone 先頭から ヒープ先頭へのバイトオフセット
	unsigned int		offset_heap;

}	t_yoyo_zone;

typedef struct	s_yoyo_normal_arena {
	// arena ロック
	pthread_mutex_t	lock;
	// マルチスレッドモードかどうか
	bool			multi_thread;
	// zone リスト
	t_yoyo_zone*	head;
}	t_yoyo_normal_arena;

typedef struct	s_yoyo_large_arena {
	// arena ロック
	pthread_mutex_t		lock;
	// マルチスレッドモードかどうか
	bool				multi_thread;
	// 使用済み LARGE chunk リスト
	t_yoyo_large_chunk*	allocated;
}	t_yoyo_large_arena;

typedef struct	s_yoyo_subarena {
	// arena ロック
	pthread_mutex_t	lock;
	// マルチスレッドモードかどうか
	bool			multi_thread;
	// なんかしらのポインタ
	void*			some;
}	t_yoyo_subarena;

// [arena 構造体]
typedef struct	s_yoyo_arena {
	bool			initialized;
	// マルチスレッドモードかどうか
	bool			multi_thread;

	unsigned int	index;

	// TINY zone 管理ヘッダ
	t_yoyo_normal_arena	tiny;
	// SMALL zone 管理ヘッダ
	t_yoyo_normal_arena	small;
	// LARGE zone 管理ヘッダ
	t_yoyo_large_arena	large;
}	t_yoyo_arena;


// [realm 構造体]
typedef struct	s_yoyo_realm {
	// 初期化済みフラグ
	bool			initialized;
	// 使用可能な arena の総数.
	// arena_count == 1 は シングルスレッドモードであることと同値.
	// !initialized の時は 1 とみなすこと.
	unsigned int	arena_count;

	// arena 配列
	// インデックス 0...arena_count の範囲が実際に利用可能.
	t_yoyo_arena	arenas[ARENA_MAX];

}	t_yoyo_realm;

// シングルスレッドモード ではロックを取らない.
// (マルチスレッドモード ではロックを取る.)

#endif
