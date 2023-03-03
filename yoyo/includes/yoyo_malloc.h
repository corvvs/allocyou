#ifndef YOYO_MALLOC_H
# define YOYO_MALLOC_H

# include <stdlib.h>

void*	yoyo_malloc(size_t n);
void	yoyo_free(void* addr);
void*	yoyo_realloc(void* addr, size_t n);
void	show_alloc_mem(void);

#endif
