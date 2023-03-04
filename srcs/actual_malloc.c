#include "internal.h"

extern t_yoyo_realm	g_yoyo_realm;

// arena を1つロックして返す.
// どの arena もロックできなかった場合は NULL を返す.
static t_yoyo_arena*	occupy_arena(t_yoyo_zone_type zone_type) {
	// まず trylock でロックできるか試してみる
	for (unsigned int i = 0; i < g_yoyo_realm.arena_count; ++i) {
		t_yoyo_arena*	arena = &g_yoyo_realm.arenas[i];
		if (try_lock_arena(arena, zone_type)) {
			DEBUGOUT("locked arenas[%u]", i);
			return arena;
		}
	}
	// trylock できなかった場合はデフォルトの arena がロックできるまで待つ.
	t_yoyo_arena*	default_arena = &g_yoyo_realm.arenas[0];
	if (lock_arena(default_arena, zone_type)) {
		DEBUGOUT("locked arenas[%u]", 0);
		return default_arena;
	}
	// このエラーは致命傷
	DEBUGERR("%s", "COULDN'T LOCK any arena");
	return NULL;
}

// LARGE chunk を mmap でアロケートして返す.
// mmap が失敗した場合は NULL を返す.
static t_yoyo_large_chunk*	allocate_large_chunk(t_yoyo_large_arena* subarena, size_t bytes) {
	// まず mmap する.
	const size_t		blocks_needed = BLOCKS_FOR_SIZE(bytes);
	const size_t		bytes_usable = blocks_needed * BLOCK_UNIT_SIZE;
	const size_t		bytes_large_chunk = LARGE_OFFSET_USABLE + bytes_usable;
	t_yoyo_large_chunk*	large_chunk = map_memory(bytes_large_chunk);
	if (large_chunk == NULL) {
		DEBUGERR("FAILED for %zu B", bytes);
		return NULL;
	}
	// 初期化する.
	large_chunk->subarena = subarena;
	large_chunk->memory_byte = bytes_large_chunk;
	large_chunk->large_next = NULL;
	t_yoyo_chunk*		chunk = (void*)large_chunk + CEILED_LARGE_CHUNK_SIZE;
	chunk->next = SET_FLAGS(NULL, YOYO_FLAG_LARGE);
	chunk->blocks = blocks_needed + 1;
	return large_chunk;
}

// LARGE subarena の使用中リストに LARGE chunk を接続する.
static void	insert_large_chunk_to_subarena(
	t_yoyo_large_arena* subarena,
	t_yoyo_large_chunk* large_chunk
) {
	t_yoyo_large_chunk**	current_lot = &(subarena->allocated);
	t_yoyo_large_chunk*		front = NULL;
	(void)front;

	// [挿入場所を見つける]
	while (true) {
		t_yoyo_large_chunk*	back = *current_lot;
		DEBUGOUT("front: %p, back: %p", front, back);
		if (back == NULL) {
			DEBUGOUT("PUSH BACK a chunk %p into %p (back of %p)", large_chunk, current_lot, front);
			break;
		}
		if ((uintptr_t)large_chunk < (uintptr_t)back) {
			DEBUGOUT("INSERT a chunk %p between %p and %p", large_chunk, front, back);
			break;
		}
		front = back;
		current_lot = &(back->large_next);
	}
	// [後挿入操作]
	large_chunk->large_next = *current_lot;
	// [前挿入操作]
	*current_lot = large_chunk;
}

// LARGE chunk をアロケートし, それを subarena に接続してから, 使用可能領域のアドレスを返す.
// どこかの過程で失敗した場合は NULL を返す.
static void*	allocate_memory_from_large(t_yoyo_large_arena* subarena, size_t n) {
	// LARGE chunk をアロケートする.
	t_yoyo_large_chunk*	large_chunk = allocate_large_chunk(subarena, n);
	if (large_chunk == NULL) { return NULL; }
	// アロケートした LARGE chunk を subarena のリストに接続する.
	insert_large_chunk_to_subarena(subarena, large_chunk);
	// 使用可能領域を返す
	return (void*)large_chunk + LARGE_OFFSET_USABLE;
}

static void	zone_push_front(t_yoyo_zone** list, t_yoyo_zone* zone) {
	assert(list != NULL);
	zone->next = *list;
	*list = zone;
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
	t_yoyo_zone_type needed_class = zone_type_for_bytes(blocks_needed * BLOCK_UNIT_SIZE);
	t_yoyo_zone_type needed_rest = zone_type_for_bytes((whole_rest - 1) * BLOCK_UNIT_SIZE);
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
static void*	try_allocate_chunk_from_zone(t_yoyo_zone* zone, size_t n) {
	DEBUGOUT("try from: %p - %zu", zone, n);
	const size_t	blocks_needed = BLOCKS_FOR_SIZE(n);
	DEBUGOUT("blocks needed: %zu, rest: %u", blocks_needed, zone->blocks_free);
	const size_t	whole_needed = blocks_needed + 1;
	if (zone->blocks_free < whole_needed) {
		DEBUGWARN("no enough blocks in this zone: %u", zone->blocks_free);
		return NULL;
	}
	t_yoyo_chunk**	current_lot = &zone->frees;
	while (*current_lot != NULL) {
		t_yoyo_chunk*	current_free_chunk = ADDRESS_OF(*current_lot);
		unsigned int	bi = get_block_index(zone, current_free_chunk);
		(void)bi;
		assert(is_head(zone, bi));
		assert(!is_used(zone, bi));
		if (is_exhaustible(current_free_chunk, blocks_needed)) {
			// current_free_chunk をすべて使用中にする.
			DEBUGSTR("EXHAUSTIBLE");
			*current_lot = current_free_chunk->next;
			mark_chunk_as_used(zone, current_free_chunk);
			zone->blocks_free -= current_free_chunk->blocks;
			zone->blocks_used += current_free_chunk->blocks;
			return (void*)current_free_chunk + CEILED_CHUNK_SIZE;
		}
		if (is_separatable(current_free_chunk, blocks_needed)) {
			// current_free_chunk の先頭を分離して blocks_needed + 1 の使用中ブロックを生成する.
			DEBUGSTR("SEPARATABLE");
			t_yoyo_chunk*	rest = (void*)current_free_chunk + whole_needed * BLOCK_UNIT_SIZE;
			rest->blocks = current_free_chunk->blocks - whole_needed;
			assert(2 <= rest->blocks);
			current_free_chunk->blocks = whole_needed;
			rest->next = current_free_chunk->next;
			*current_lot = COPY_FLAGS(rest, current_free_chunk->next);
			mark_chunk_as_free(zone, rest);
			mark_chunk_as_used(zone, current_free_chunk);
			zone->blocks_free -= whole_needed;
			zone->blocks_used += whole_needed;
			print_zone_state(zone);
			print_zone_bitmap_state(zone);
			DEBUGINFO("rest: %p - %zu", rest, rest->blocks);
			return (void*)current_free_chunk + CEILED_CHUNK_SIZE;
		}
		current_lot = &(current_free_chunk->next);
	}
	DEBUGWARN("FAILED from: %p - %zu B", zone, n);
	return NULL;
}

static void*	allocate_memory_from_zone_list(t_yoyo_arena* arena, t_yoyo_zone_type zone_type, size_t n) {
	t_yoyo_normal_arena* subarena = (t_yoyo_normal_arena*)get_subarena(arena, zone_type);
	t_yoyo_zone*	current_zone = subarena->head;
	while (current_zone != NULL) {
		if (try_lock_zone(current_zone)) {
			// zone ロックが取れた -> アロケートを試みる
			print_zone_state(current_zone);
			print_zone_bitmap_state(current_zone);
			void*	mem = try_allocate_chunk_from_zone(current_zone, n);
			print_zone_state(current_zone);
			print_zone_bitmap_state(current_zone);
			unlock_zone(current_zone);
			if (mem != NULL) {
				return mem;
			}
		}
		// zone ロックを取らないオペレーションは, 既存の zone の next をいじらないので,
		// zone ロックを取らなくても zone->next を触っていいはず. たぶん.
		current_zone = current_zone->next;
	}

	// どの zone からもアロケートできなかった -> zone を増やしてもう一度
	DEBUGWARN("ALLOCATE A ZONE NEWLY for %zu B", n);
	t_yoyo_zone*	new_zone = allocate_zone(arena, zone_type);
	if (new_zone == NULL) {
		return NULL;
	}
	print_zone_state(new_zone);
	print_zone_bitmap_state(new_zone);
	zone_push_front(&subarena->head, new_zone);

	// new_zone はロック取らなくていい.
	return try_allocate_chunk_from_zone(new_zone, n);
}

static void*	allocate_from_arena(t_yoyo_arena* arena, t_yoyo_zone_type zone_type, size_t n) {
	if (zone_type == YOYO_ZONE_LARGE) {
		// LARGE からアロケートする
		return allocate_memory_from_large(&arena->large, n);
	}
	// TINY / SMALL からアロケートする
	t_yoyo_subarena* subarena = get_subarena(arena, zone_type);
	assert(subarena != NULL);
	assert((void*)subarena != (void*)&arena->large);
	return allocate_memory_from_zone_list(arena, zone_type, n);
}

void*	actual_malloc(size_t n) {
	// [ゾーン種別決定]
	t_yoyo_zone_type zone_type = zone_type_for_bytes(n);

	// [アリーナを1つ選択してロックする]
	t_yoyo_arena* arena = occupy_arena(zone_type);
	if (arena == NULL) {
		DEBUGWARN("%s", "failed: couldn't lock any arena");
		return NULL;
	}

	// [アリーナからmalloc]
	void*	mem = allocate_from_arena(arena, zone_type, n);

	// [アリーナをアンロックする]
	unlock_arena(arena, zone_type);
	return mem;
}

