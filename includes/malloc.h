#ifndef YOYO_MALLOC_H
# define YOYO_MALLOC_H

# include <stdlib.h>

extern void*	malloc(size_t n);
extern void*	calloc(size_t count, size_t size);
extern void		free(void* addr);
extern void*	realloc(void* addr, size_t n);

extern size_t 	malloc_usable_size (void *ptr);
extern void*	memalign(size_t alignment, size_t size);
extern void*	aligned_alloc(size_t alignment, size_t size);
extern int		posix_memalign(void **memptr, size_t alignment, size_t size);

extern void		show_alloc_mem(void);

// show_alloc_mem にはない追加情報を表示する:
//  - プログラム起動時から呼び出し時点までのメモリ関連操作の履歴
//  - 現在使用中のチャンクの一部のダンプ
extern void		show_alloc_mem_ex(void);

// システムに返せるメモリ領域があるなら返す
extern void		release_memory(void);

#endif
