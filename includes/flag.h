#ifndef YOYO_FLAG_H
# define YOYO_FLAG_H

# include <stdlib.h>
# include "structure.h"

// このチャンクがLARGEであることを示すフラグ
# define YOYO_FLAG_LARGE 1u
// このチャンクヘッダが「擬似ヘッダ」であることを示すフラグ
# define YOYO_FLAG_PSEUDO_HEADER 2u
// フラグ部分をカバーするマスク
# define YOYO_FLAG_MASK 7u
// フラグ部分以外をカバーするマスク
# define YOYO_FLAG_NOT_MASK (~(uintptr_t)7)

// アドレス addr からフラグを切り落として正常なアドレスを返す
# define ADDRESS_OF(addr) ((void*)((uintptr_t)addr & YOYO_FLAG_NOT_MASK))

// chunk->next からフラグを切り落として正常なアドレスを返す
# define NEXT_OF(chunk) ADDRESS_OF((chunk)->next)

// アドレス addr にフラグ flags をセットしたものを返す
# define SET_FLAGS(addr, flags) ((void*)((uintptr_t)ADDRESS_OF(addr) | (YOYO_FLAG_MASK & flags)))

// アドレス dst のフラグをアドレス src のフラグにしたものを返す
# define COPY_FLAGS(dst, src) (SET_FLAGS(dst, (uintptr_t)src))

// この chunk が LARGE かどうかを判定する
# define IS_LARGE_CHUNK(chunk) (!!((uintptr_t)((chunk)->next) & YOYO_FLAG_LARGE))
// このチャンクヘッダが擬似かどうかを判定する
# define IS_PSEUDO_HEADER(chunk) (!!((uintptr_t)((chunk)->next) & YOYO_FLAG_PSEUDO_HEADER))

#endif
