#ifndef YO_INTERNAL_MALLOC_H
# define YO_INTERNAL_MALLOC_H

# include "yo_utils.h"
# include "yo_struct.h"
# include "yo_malloc.h"
# include <stdio.h>
# include <unistd.h>
# include <string.h>
# include <sys/mman.h>
# include <errno.h>
# include <assert.h>

t_yo_malloc_root	g_root;

void	*_yo_allocate_heap(size_t n);

void			insert_item(t_block_header **list, t_block_header *item);
void			remove_item(t_block_header **list, t_block_header *item);
t_block_header*	find_item(t_block_header* list, t_block_header *item);

void	show_list(t_block_header *list);
void	_yo_free_large_chunk(t_block_header *head);
void*	_yo_large_malloc(size_t n);
void*	_yo_relocate_chunk(t_block_header* head, size_t n);
void*	_yo_shrink_chunk(t_block_header* head, size_t n);
void*	_yo_try_extend_chunk(t_block_header* head, size_t n);


#define	BLOCKS_FOR_SIZE(n) (QUANTIZE(n, BLOCK_UNIT_SIZE) / BLOCK_UNIT_SIZE)

#endif
