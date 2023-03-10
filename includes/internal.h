#ifndef YOYO_INTERNAL_MALLOC_H
# define YOYO_INTERNAL_MALLOC_H

# include "common.h"
# include "time.h"
# include "structure.h"
# include "flag.h"
# include <stdio.h>
# include <unistd.h>
# include <string.h>
# include <sys/mman.h>
# include <errno.h>
# include <assert.h>

extern t_yoyo_realm	g_yoyo_realm;

// actual_malloc.c
void*	actual_malloc(size_t n);

// actual_free.c
void	actual_free(void* addr);
void	free_from_locked_tiny_small_zone(t_yoyo_zone* zone, t_yoyo_chunk* chunk);

// actual_realloc.c
void*	actual_realloc(void* addr, size_t n);

// visualize.c
void	actual_show_alloc_mem(void);

// lock.c
bool	lock_arena(t_yoyo_arena* arena, t_yoyo_zone_type zone_type);
bool	try_lock_arena(t_yoyo_arena* arena, t_yoyo_zone_type zone_type);
bool	lock_subarena(t_yoyo_subarena* subarena);
bool	try_lock_subarena(t_yoyo_subarena* subarena);
bool	lock_zone(t_yoyo_zone* zone);
bool	try_lock_zone(t_yoyo_zone* zone);
bool	unlock_arena(t_yoyo_arena* arena, t_yoyo_zone_type zone_type);
bool	unlock_subarena(t_yoyo_subarena* subarena);
bool	unlock_zone(t_yoyo_zone* zone);

// memory_alloc.c
void*	map_memory(size_t bytes, bool align);
void	unmap_memory(void* start, size_t size);

// init_realm.c
bool	init_realm(bool multi_thread);

// arena_initialize.c
bool				init_arena(unsigned int index, bool multi_thread);
void				destroy_arena(t_yoyo_arena* arena);
t_yoyo_subarena*	get_subarena(const t_yoyo_arena* arena, t_yoyo_zone_type zone_type);


// zone_initialize.c
t_yoyo_zone*	allocate_zone(const t_yoyo_arena* arena, t_yoyo_zone_type zone_type);

// zone_bitmap.c
bool			is_head(const t_yoyo_zone* zone, unsigned int block_index);
bool			is_used(const t_yoyo_zone* zone, unsigned int block_index);
t_yoyo_chunk*	get_chunk_by_index(t_yoyo_zone* zone, unsigned int block_index);

// zone_utils.c
size_t	zone_bytes_for_zone_type(t_yoyo_zone_type zone_type);
size_t	max_chunk_blocks_for_zone_type(t_yoyo_zone_type zone_type);
t_yoyo_zone_type	zone_type_for_bytes(size_t n);
unsigned int	get_block_index(const t_yoyo_zone* zone, const t_yoyo_chunk* head);
t_yoyo_zone*	get_zone_of_chunk(const t_yoyo_chunk* chunk);
void	sort_zone_list(t_yoyo_zone** list);

// zone_operation.c
void	unmark_chunk(t_yoyo_zone* zone, const t_yoyo_chunk* chunk);
void	mark_chunk_as_free(t_yoyo_zone* zone, const t_yoyo_chunk* chunk);
void	mark_chunk_as_used(t_yoyo_zone* zone, const t_yoyo_chunk* chunk);

// debug.c
void	print_zone_state(const t_yoyo_zone* zone);
void	print_zone_bitmap_state(const t_yoyo_zone* zone);
void	print_memory_state(const void* addr);


#endif
