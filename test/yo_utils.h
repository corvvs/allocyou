#ifndef YO_UTILS_H
# define YO_UTILS_H

# include "yo_common.h"

# define TX_GRN "\e[32m"
# define TX_BLU "\e[32m"
# define TX_RST "\e[0m"

// quantize a by b
// least-greater-than multiple
// a 以上で最小の b の倍数を返す
// b > 0 を仮定している
# define QUANTIZE(a, b) (a ? ((a - 1) / b + 1) * b : b)

// t_block_header の next から実際のアドレスを取り出す
// (下位3ビットを0にするだけ)
# define ADDRESS(ptr) ((void *)((uintptr_t)ptr / 8 * 8))
# define FLAGS(ptr) ((void *)((uintptr_t)ptr % 8))
# define COPYFLAGS(dst, src) ((void *)((uintptr_t)ADDRESS(dst) | (uintptr_t)FLAGS(src)))
# define GET_IS_LARGE(ptr) (!!((uintptr_t)ptr & 1))
# define SET_IS_LARGE(ptr, flag) ((void*)(((uintptr_t)ptr ^ 1) | !!flag))

#endif
