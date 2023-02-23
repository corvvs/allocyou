#ifndef YO_INTERNAL_MALLOC_H
# define YO_INTERNAL_MALLOC_H

# include "yo_utils.h"
# include "yo_struct.h"
# include "yo_malloc.h"
# include "yo_time.h"
# include <stdio.h>
# include <unistd.h>
# include <string.h>
# include <sys/mman.h>
# include <errno.h>
# include <assert.h>

t_yo_malloc_root	g_root;

t_yo_zone_class	yo_zone_for_bytes(size_t n);
t_yo_zone_class	yo_zone_for_addr(void* n);
t_yo_zone*		yo_retrieve_zone_for_class(t_yo_zone_class zone);
void*			yo_allocate_heap(size_t n, t_yo_zone_class zone);
void*			set_for_zone(void *addr, t_yo_zone_class zone);


t_listcursor	init_cursor(t_block_header *list);
t_listcursor	init_cursor_from_mid(t_block_header *list, t_block_header *mid);
t_block_header*	list_next_head(t_block_header *head);
void			concat_item(t_block_header *head, t_block_header *item);
void			increment_cursor(t_listcursor *cursor);
void			insert_item(t_block_header **list, t_block_header *item);
void			remove_item(t_block_header **list, t_block_header *item);
t_block_header*	find_inf_item(t_block_header* list, t_block_header *item);
t_listcursor	find_cross_cursor(t_block_header* list, t_block_header *item);
t_listcursor	find_fit_cursor(t_block_header* list, t_block_header *item);
void			nullify_chunk(t_block_header *chunk);
t_block_header*	unify_chunk(t_block_header *b1, t_block_header *b2);
t_block_header*	assimilate_chunk(t_block_header *b1, t_block_header *b2);

void	show_list(t_block_header *list);
void*	yo_malloc_actual(size_t n);
void	yo_free_actual(void *addr);
void*	yo_realloc_actual(void *addr, size_t n);
void	show_alloc_mem_actual(void);

void	yo_free_large_chunk(t_block_header *head);
void*	yo_large_malloc(size_t n);
void*	yo_relocate(void* addr, size_t n);
void	yo_shrink_chunk(t_block_header* head, size_t n);
void	yo_extend_chunk(t_yo_zone* zone, t_block_header* head, size_t n);

void	check_consistency(void);


#define	BLOCKS_FOR_SIZE(n) (QUANTIZE(n, BLOCK_UNIT_SIZE) / BLOCK_UNIT_SIZE)

#endif
