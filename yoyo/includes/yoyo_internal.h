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
bool	lock_arena(t_yoyo_arena* arena, t_yoyo_zone_class zone_class);
bool	try_lock_arena(t_yoyo_arena* arena, t_yoyo_zone_class zone_class);
bool	lock_subarena(t_yoyo_subarena* subarena);
bool	try_lock_subarena(t_yoyo_subarena* subarena);
bool	lock_zone(t_yoyo_zone* zone);
bool	try_lock_zone(t_yoyo_zone* zone);
bool	unlock_arena(t_yoyo_arena* arena, t_yoyo_zone_class zone_class);
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
t_yoyo_subarena*	get_subarena(t_yoyo_arena* arena, t_yoyo_zone_class zone_class);


// yoyo_zone_initialize.c
size_t			heap_bytes_for_zone_bytes(size_t zone_bytes);
bool			init_zone(const t_yoyo_arena* arena, t_yoyo_zone* zone, t_yoyo_zone_class zone_class);
t_yoyo_zone*	allocate_zone(const t_yoyo_arena* arena, t_yoyo_zone_class zone_class);

// yoyo_zone_utils.c
size_t	zone_bytes_for_zone_class(t_yoyo_zone_class zone_class);
size_t	max_chunk_blocks_for_zone_class(t_yoyo_zone_class zone_class);
t_yoyo_zone_class	zone_class_for_bytes(size_t n);
unsigned int	get_block_index(t_yoyo_zone* zone, t_yoyo_chunk* head);

// yoyo_zone_operation.c
void	mark_chunk_as_free(t_yoyo_zone* zone, t_yoyo_chunk* chunk);
void	mark_chunk_as_used(t_yoyo_zone* zone, t_yoyo_chunk* chunk);


#endif
