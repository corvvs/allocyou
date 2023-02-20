#ifndef YO_MALLOC_H
# define YO_MALLOC_H

# include <stdlib.h>

void*	yo_malloc(size_t n);
void	yo_free(void *addr);
void*	yo_realloc(void *addr, size_t size);
void	show_alloc_mem(void);

#endif
