#ifndef YOYO_INTERNAL_MALLOC_H
# define YOYO_INTERNAL_MALLOC_H

# include "yoyo_common.h"
# include "yoyo_time.h"
# include "yoyo_structure.h"
# include <stdio.h>
# include <unistd.h>
# include <string.h>
# include <sys/mman.h>
# include <errno.h>
# include <assert.h>

t_yoyo_realm	g_yoyo_realm;

// yoyo_init_realm.c
bool	init_realm(bool multi_thread);

// yoyo_arena_initialize.c
bool	init_arena(t_yoyo_arena* arena, bool multi_thread);
void	destroy_arena(t_yoyo_arena* arena);

// yoyo_zone_initialize.c
size_t	heap_bytes_for_zone_bytes(size_t zone_bytes);
bool	init_zone(t_yoyo_arena* arena, t_yoyo_zone* zone, t_yoyo_zone_class zone_class);

// yoyo_zone_utils.c
size_t	zone_bytes_for_zone_class(t_yoyo_zone_class zone_class);



#endif
