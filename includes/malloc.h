#ifndef YOYO_MALLOC_H
# define YOYO_MALLOC_H

# include <stdlib.h>

extern void*	malloc(size_t n);
extern void*	calloc(size_t count, size_t size);
extern void	free(void* addr);
extern void*	realloc(void* addr, size_t n);
extern void	show_alloc_mem(void);

#endif
