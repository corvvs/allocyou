#ifndef YO_UTILS_H
# define YO_UTILS_H

# include "yo_common.h"

// quantize a by b
// least-greater-than multiple
// a 以上で最小の b の倍数を返す
// b > 0 を仮定している
# define QUANTIZE(a, b) (a ? ((a - 1) / b + 1) * b : b)


// これが立っているとLARGEゾーンのチャンク
# define IS_LARGE (1u)
// これが立っているとTINYゾーンのチャンク
# define IS_TINY (2u)
// IS_LARGE でも IS_TINY でもない場合はSMALLゾーンのチャンク
// IS_LARGE でも IS_TINY でもある場合お前は間違っている

// t_block_header の next から実際のアドレスを取り出す
// (下位3ビットを0にするだけ)
# define ADDRESS(ptr) ((void *)((uintptr_t)ptr / 8 * 8))
# define FLAGS(ptr) ((void *)((uintptr_t)ptr % 8))
# define COPYFLAGS(dst, src) ((void *)((uintptr_t)ADDRESS(dst) | (uintptr_t)FLAGS(src)))
# define SETFLAGS(ptr, flag) ((void *)((uintptr_t)ADDRESS(ptr) | (uintptr_t)flag))
# define GET_IS_LARGE(ptr) (!!((uintptr_t)ptr & IS_LARGE))
# define SET_IS_LARGE(ptr) ((void*)(((uintptr_t)ptr | IS_LARGE)))
# define UNSET_IS_LARGE(ptr) ((void*)(((uintptr_t)ptr | IS_LARGE) ^ IS_LARGE))
# define GET_IS_TINY(ptr) (!!((uintptr_t)ptr & IS_TINY))
# define SET_IS_TINY(ptr) ((void*)(((uintptr_t)ptr | IS_TINY)))
# define UNSET_IS_TINY(ptr) ((void*)(((uintptr_t)ptr | IS_TINY) ^ IS_TINY))
# define GET_IS_SMALL(ptr) (!GET_IS_LARGE(ptr) && !GET_IS_TINY(ptr))
# define SET_IS_SMALL(ptr) (UNSET_IS_TINY(UNSET_IS_LARGE(ptr)))

void	*yo_memcpy(void* dst, const void* src, size_t n);

#endif
