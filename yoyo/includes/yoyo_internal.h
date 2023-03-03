#ifndef YOYO_INTERNAL_MALLOC_H
# define YOYO_INTERNAL_MALLOC_H

# include "yoyo_common.h"
# include "yoyo_time.h"
# include "yoyo_structure.h"
# include "yoyo_flag.h"
# include <stdio.h>
# include <unistd.h>
# include <string.h>
# include <sys/mman.h>
# include <errno.h>
# include <assert.h>

t_yoyo_realm	g_yoyo_realm;

// yoyo_actual_malloc.c
void*	actual_malloc(size_t n);

// yoyo_actual_free.c
void	actual_free(void* addr);

// yoyo_lock.c
bool	lock_arena(t_yoyo_arena* arena, t_yoyo_zone_type zone_type);
bool	try_lock_arena(t_yoyo_arena* arena, t_yoyo_zone_type zone_type);
bool	lock_subarena(t_yoyo_subarena* subarena);
bool	try_lock_subarena(t_yoyo_subarena* subarena);
bool	lock_zone(t_yoyo_zone* zone);
bool	try_lock_zone(t_yoyo_zone* zone);
bool	unlock_arena(t_yoyo_arena* arena, t_yoyo_zone_type zone_type);
bool	unlock_subarena(t_yoyo_subarena* subarena);
bool	unlock_zone(t_yoyo_zone* zone);

// yoyo_memory_alloc.c
void*	map_memory(size_t bytes);
void	unmap_memory(void* start, size_t size);

// yoyo_init_realm.c
bool	init_realm(bool multi_thread);

// yoyo_arena_initialize.c
bool				init_arena(unsigned int index, bool multi_thread);
void				destroy_arena(t_yoyo_arena* arena);
t_yoyo_subarena*	get_subarena(const t_yoyo_arena* arena, t_yoyo_zone_type zone_type);


// yoyo_zone_initialize.c
t_yoyo_zone*	allocate_zone(const t_yoyo_arena* arena, t_yoyo_zone_type zone_type);

// yoyo_zone_bitmap.c
bool	is_head(const t_yoyo_zone* zone, unsigned int block_index);
bool	is_used(const t_yoyo_zone* zone, unsigned int block_index);

// yoyo_zone_utils.c
size_t	zone_bytes_for_zone_type(t_yoyo_zone_type zone_type);
size_t	max_chunk_blocks_for_zone_type(t_yoyo_zone_type zone_type);
t_yoyo_zone_type	zone_type_for_bytes(size_t n);
unsigned int	get_block_index(const t_yoyo_zone* zone, const t_yoyo_chunk* head);

// yoyo_zone_operation.c
void	unmark_chunk(t_yoyo_zone* zone, const t_yoyo_chunk* chunk);
void	mark_chunk_as_free(t_yoyo_zone* zone, const t_yoyo_chunk* chunk);
void	mark_chunk_as_used(t_yoyo_zone* zone, const t_yoyo_chunk* chunk);

// yoyo_debug.c
void	print_zone_state(const t_yoyo_zone* zone);
void	print_zone_bitmap_state(const t_yoyo_zone* zone);
void	print_memory_state(const void* addr);


#endif
