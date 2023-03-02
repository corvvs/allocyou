#include "yoyo_internal.h"

// ロックされた arena を1つ返す.
static t_yoyo_arena*	occupy_arena(t_yoyo_zone_class zone_class) {
	t_yoyo_arena* arena = NULL;
	for (unsigned int i = 0; i < g_yoyo_realm.arena_count; ++i) {
		arena = &g_yoyo_realm.arenas[i];
		if (try_lock_arena(arena, zone_class)) {
			DEBUGOUT("locked arenas[%u]", i);
			break;
		}
		arena = NULL;
	}
	if (arena == NULL) {
		arena = &g_yoyo_realm.arenas[0];
		if (!lock_arena(arena, zone_class)) {
			return NULL;
		}
		DEBUGOUT("locked arenas[%u]", 0);
	}
	return arena;
}

// LARGE chunk を mmap でアロケートする.
static t_yoyo_large_chunk*	map_large_chunk(t_yoyo_large_arena* subarena, size_t n) {
	const size_t		large_chunk_size = CEIL_BY(sizeof(t_yoyo_large_chunk), BLOCK_UNIT_SIZE);
	const size_t		chunk_size = CEIL_BY(sizeof(t_yoyo_chunk), BLOCK_UNIT_SIZE);
	const size_t		blocks_needed = BLOCKS_FOR_SIZE(n);
	const size_t		usable_size = blocks_needed * BLOCK_UNIT_SIZE;
	const size_t		mem_size = large_chunk_size + chunk_size + usable_size;
	t_yoyo_large_chunk*	large_chunk = map_memory(mem_size);
	if (large_chunk == NULL) {
		DEBUGERR("failed for %zuB", n);
		return NULL;
	}
	large_chunk->subarena = subarena;
	large_chunk->memory_byte = mem_size;
	large_chunk->large_next = NULL;
	t_yoyo_chunk*		chunk = (void*)large_chunk + large_chunk_size;
	chunk->next = SET_FLAGS(NULL, YOYO_FLAG_LARGE);
	chunk->blocks = blocks_needed + 1;
	return large_chunk;
}

// LARGE chunk をアロケートし, subarena に接続し, 使用可能領域のアドレスを返す.
static void*	allocate_from_large(t_yoyo_large_arena* subarena, size_t n) {
	t_yoyo_large_chunk*	large_chunk = map_large_chunk(subarena, n);
	if (large_chunk == NULL) { return NULL; }
	t_yoyo_large_chunk**	list = &(subarena->allocated);

	while (true) {
		t_yoyo_large_chunk*	head = ADDRESS_OF(*list);
		DEBUGOUT("head: %p", head);
		if (head == NULL) {
			DEBUGOUT("PUSH BACK a chunk %p to %p", large_chunk, list);
			break;
		}
		if ((uintptr_t)large_chunk < (uintptr_t)head) {
			DEBUGOUT("PUSH FRONT a chunk %p of %p", large_chunk, list);
			list = NULL;
			break;
		}
		t_yoyo_large_chunk*	next = head->large_next;
		if ((uintptr_t)large_chunk < (uintptr_t)next) {
			DEBUGOUT("INSERT a chunk %p next of %p", large_chunk, list);
			break;
		}
		list = &(head->large_next);
	}
	if (list == NULL) {
		// PUSH FRONT
		large_chunk->large_next = subarena->allocated;
		list = &(subarena->allocated);
	} else if (*list != NULL) {
		// INSERT
		large_chunk->large_next = (*list)->large_next;
	} else {
		// PUSH BACK
		large_chunk->large_next = NULL;
	}
	*list = large_chunk;
	// 使用可能領域を返す
	return (void*)large_chunk + LARGE_OFFSET_USABLE;
}

static void	zone_push_front(t_yoyo_zone** list, t_yoyo_zone* zone) {
	assert(list != NULL);
	zone->next = *list;
	*list = zone;
}

// このブロックが chunk のヘッダかどうか
static bool	is_head(const t_yoyo_zone* zone, unsigned int block_index) {
	unsigned int	byte_index = block_index / 8;
	unsigned int	bit_index = block_index % 8;
	unsigned char*	heads = (void*)zone + zone->offset_bitmap_heads;
	return !!(heads[byte_index] & (1 << bit_index));
}

// このブロックがヘッダになっている chunk が使用中かどうか
static bool	is_used(const t_yoyo_zone* zone, unsigned int block_index) {
	unsigned int	byte_index = block_index / 8;
	unsigned int	bit_index = block_index % 8;
	unsigned char*	used = (void*)zone + zone->offset_bitmap_used;
	return !!(used[byte_index] & (1 << bit_index));
}


// blocks_needed 要求されているとき, chunk を分割することで要求に応えられるか?
static bool	is_separatable(const t_yoyo_chunk* chunk, size_t blocks_needed) {
	assert(chunk->blocks > 1);
	const size_t	whole_needed = blocks_needed + 1;
	if (chunk->blocks < whole_needed) {
		DEBUGOUT("no: not enough blocks: %zu < %zu", chunk->blocks, whole_needed);
		return false;
	}
	const size_t	whole_rest = chunk->blocks - whole_needed;
	if (whole_rest >= whole_needed) {
		DEBUGOUT("yes : %zu >= %zu", whole_rest, whole_needed);
		return true;
	}
	t_yoyo_zone_class needed_class = zone_class_for_bytes(blocks_needed * BLOCK_UNIT_SIZE);
	t_yoyo_zone_class needed_rest = zone_class_for_bytes((whole_rest - 1) * BLOCK_UNIT_SIZE);
	if (needed_class != needed_rest) {
		DEBUGOUT("no: not enough rest blocks: %zu < %zu", chunk->blocks, whole_needed);
		return false;
	}
	DEBUGOUT("yes: has enough rest blocks: %zu < %zu", whole_rest, whole_needed);
	return true;
}

// blocks_needed 要求されているとき, chunk を占有することで要求に応えられるか?
static bool	is_exhaustible(const t_yoyo_chunk* chunk, size_t blocks_needed) {
	const bool just_fit = chunk->blocks == blocks_needed + 1;
	const bool semi_just_fit = chunk->blocks == blocks_needed + 2;
	return just_fit || semi_just_fit;
}

// zone からサイズ n の chunk を取得しようとする.
static void*	try_allocate_from_zone(t_yoyo_zone* zone, size_t n) {
	DEBUGOUT("try from: %p - %zu", zone, n);

	const size_t	blocks_needed = BLOCKS_FOR_SIZE(n);
	DEBUGOUT("blocks needed: %zu, rest: %u", blocks_needed, zone->blocks_free);
	const size_t	whole_needed = blocks_needed + 1;
	if (zone->blocks_free < whole_needed) {
		DEBUGWARN("no enough blocks in this zone: %u", zone->blocks_free);
		return NULL;
	}
	t_yoyo_chunk**	list = &zone->frees;
	while (*list != NULL) {
		t_yoyo_chunk*	head = ADDRESS_OF(*list);
		unsigned int	bi = get_block_index(zone, head);
		(void)bi;
		assert(is_head(zone, bi));
		assert(!is_used(zone, bi));
		if (is_exhaustible(head, blocks_needed)) {
			// head をすべて使用中にする.
			DEBUGSTR("EXHAUSTIBLE");
			*list = head->next;
			mark_chunk_as_used(zone, head);
			zone->blocks_free -= head->blocks;
			zone->blocks_used += head->blocks;
			return (void*)head + BLOCK_UNIT_SIZE;
		}
		if (is_separatable(head, blocks_needed)) {
			// head の先頭を分離して blocks_needed + 1 の使用中ブロックを生成する.
			DEBUGSTR("SEPARATABLE");
			t_yoyo_chunk*	rest = (void*)head + whole_needed * BLOCK_UNIT_SIZE;
			rest->blocks = head->blocks - whole_needed;
			head->blocks = whole_needed;
			rest->next = head->next;
			*list = COPY_FLAGS(rest, head->next);
			mark_chunk_as_free(zone, rest);
			mark_chunk_as_used(zone, head);
			zone->blocks_free -= whole_needed;
			zone->blocks_used += whole_needed;
			return (void*)head + BLOCK_UNIT_SIZE;
		}
		list = &(head->next);
	}
	DEBUGWARN("FAILED from: %p - %zuB", zone, n);
	return NULL;
}

static void*	allocate_from_zone_list(t_yoyo_arena* arena, t_yoyo_zone_class zone_class, size_t n) {
	t_yoyo_normal_arena* subarena = (t_yoyo_normal_arena*)get_subarena(arena, zone_class);
	t_yoyo_zone*	head = subarena->head;
	while (head != NULL) {
		if (try_lock_zone(head)) {
			// zone ロックが取れた -> アロケートを試みる
			void*	mem = try_allocate_from_zone(head, n);
			unlock_zone(head);
			if (mem != NULL) {
				return mem;
			}
		}
		// zone ロックを取らないオペレーションは, 既存の zone の next をいじらないので,
		// zone ロックを取らなくても zone->next を触っていいはず. たぶん.
		head = head->next;
	}

	// どの zone からもアロケートできなかった -> zone を増やしてもう一度
	DEBUGWARN("ALLOCATE A ZONE NEWLY for %zuB", n);
	t_yoyo_zone*	new_zone = allocate_zone(arena, zone_class);
	if (new_zone == NULL) {
		return NULL;
	}
	zone_push_front(&subarena->head, new_zone);

	// new_zone はロック取らなくていい.
	return try_allocate_from_zone(new_zone, n);
}

static void*	allocate_from_arena(t_yoyo_arena* arena, t_yoyo_zone_class zone_class, size_t n) {
	if (zone_class == YOYO_ZONE_LARGE) {
		// LARGE からアロケートする
		return allocate_from_large(&arena->large, n);
	}
	// TINY / SMALL からアロケートする
	t_yoyo_subarena* subarena = get_subarena(arena, zone_class);
	assert(subarena != NULL);
	assert((void*)subarena != (void*)&arena->large);
	return allocate_from_zone_list(arena, zone_class, n);
}

void*	actual_malloc(size_t n) {
	// ゾーン種別決定
	t_yoyo_zone_class zone_class = zone_class_for_bytes(n);

	// 対応するアリーナを選択してロックする
	t_yoyo_arena* arena = occupy_arena(zone_class);
	if (arena == NULL) {
		DEBUGWARN("%s", "failed: couldn't lock any arena");
		return NULL;
	}

	void*	mem = allocate_from_arena(arena, zone_class, n);

	// アンロックする
	unlock_arena(arena, zone_class);
	return mem;
}

