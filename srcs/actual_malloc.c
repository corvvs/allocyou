#include "internal.h"

// arena を1つロックして返す.
// どの arena もロックできなかった場合は NULL を返す.
static t_yoyo_arena*	occupy_arena(t_yoyo_zone_type zone_type, size_t n) {
	(void)n;
	// arena が複数ある場合は, まず trylock でロックできるか試してみる
	if (g_yoyo_realm.arena_count > 1) {
		for (unsigned int i = 0; i < g_yoyo_realm.arena_count; ++i) {
			t_yoyo_arena*	arena = &g_yoyo_realm.arenas[i];
			DEBUGOUT("locking arenas[%u] (%p) %u for %zu B", arena->index, arena, i, n);
			if (try_lock_arena(arena, zone_type)) {
				DEBUGOUT("locked arenas[%u] (%p) %u for %zu B", arena->index, arena, i, n);
				return arena;
			}
			DEBUGOUT("FAILED to lock arenas[%u] (%p) %u for %zu B", arena->index, arena, i, n);
		}
	}
	// trylock できなかった場合はデフォルトの arena がロックできるまで待つ.
	t_yoyo_arena*	default_arena = &g_yoyo_realm.arenas[0];
	DEBUGOUT("locking default arena [%u] (%p) for %zu B", default_arena->index, default_arena, n);
	if (lock_arena(default_arena, zone_type)) {
		DEBUGOUT("locked arenas[%u] (%p) for %zu B", default_arena->index, default_arena, n);
		return default_arena;
	}
	// このエラーは致命傷
	DEBUGFATAL("COULDN'T LOCK any arena for %zu B", n);
	return NULL;
}

// LARGE chunk を mmap でアロケートして返す.
// mmap が失敗した場合は NULL を返す.
static t_yoyo_large_chunk*	allocate_large_chunk(t_yoyo_large_arena* subarena, size_t bytes) {
	// まず mmap する.
	const size_t		blocks_needed = BLOCKS_FOR_SIZE(bytes);
	const size_t		bytes_usable = blocks_needed * BLOCK_UNIT_SIZE;
	if (LARGE_OFFSET_USABLE >= SIZE_MAX - bytes_usable) {
		DEBUGERR("required bytes %zu B is too large", bytes);
		errno = ENOMEM;
		return NULL;
	}
	const size_t		bytes_large_chunk = LARGE_OFFSET_USABLE + bytes_usable;
	const size_t		pagesize = getpagesize();
	const size_t		memory_byte = CEIL_BY(bytes_large_chunk, pagesize);
	if (memory_byte < pagesize) {
		DEBUGERR("required bytes %zu B is too large", bytes);
		errno = ENOMEM;
		return NULL;
	}
	t_yoyo_large_chunk*	large_chunk = yoyo_map_memory(memory_byte, false);
	if (large_chunk == NULL) {
		// サイズがデカすぎて失敗しているならいいが, そうでないなら致命傷
		DEBUGERR("FAILED for %zu B", bytes);
		return NULL;
	}
	// 初期化する.
	large_chunk->subarena = subarena;
	large_chunk->memory_byte = memory_byte;
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
	if (large_chunk == NULL) {
		DEBUGERR("failed to allocate LARGE chunk at subarena %p for %zu B", subarena, n);
		return NULL;
	}
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
	// const bool semi_just_fit = chunk->blocks == blocks_needed + 2;
	return just_fit;
}

// current_free_chunk をすべて使用中にする.
static void	exhaust_chunk(t_yoyo_zone* zone, t_yoyo_chunk** current_lot, t_yoyo_chunk* current_free_chunk) {
	// DEBUGSTR("EXHAUSTIBLE");
	*current_lot = current_free_chunk->next;
	mark_chunk_as_used(zone, current_free_chunk);
	zone->blocks_free -= current_free_chunk->blocks;
	zone->blocks_used += current_free_chunk->blocks;
}

// current_free_chunk の先頭を分離して blocks_needed + 1 の使用中ブロックを生成する.
static void	separate_chunk(t_yoyo_zone* zone, t_yoyo_chunk** current_lot, t_yoyo_chunk* current_free_chunk, size_t whole_needed) {
	// DEBUGSTR("SEPARATABLE");
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
}

// zone からサイズ n の chunk を取得しようとする.
static void*	try_allocate_chunk_from_zone(t_yoyo_zone* zone, size_t n) {
	DEBUGOUT("try from zone: %p, for %zu B", zone, n);
	const size_t	blocks_needed = BLOCKS_FOR_SIZE(n);
	DEBUGOUT("blocks needed: %zu, rest: %u", blocks_needed, zone->blocks_free);
	const size_t	whole_needed = blocks_needed + 1;
	if (zone->blocks_free < whole_needed) {
		DEBUGWARN("no enough blocks in this zone: %u", zone->blocks_free);
		return NULL;
	}
	t_yoyo_chunk*	prev = zone->free_prev;
	t_yoyo_chunk*	head = prev != NULL ? NEXT_OF(prev) : zone->frees;
	if (head == NULL) {
		head = zone->frees;
		prev = NULL;
	}
	// initial_head: head の初期値
	// head がふたたび initial_head に達したら, 使えるフリーチャンクが見つからなかったことにする
	const t_yoyo_chunk*	initial_head = head;
	while (true) {
		assert(check_is_free(zone, head));
		t_yoyo_chunk**	current_lot = prev != NULL ? &(prev->next) : &(zone->frees);
		if (is_exhaustible(head, blocks_needed)) {
			exhaust_chunk(zone, current_lot, head);
		} else if (is_separatable(head, blocks_needed)) {
			separate_chunk(zone, current_lot, head, whole_needed);
		} else {
			prev = head;
			head = NEXT_OF(head);
			if (head == NULL) {
				head = zone->frees;
				prev = NULL;
			}
			if (head == initial_head) {
				break;
			}
			continue;
		}
		// チャンクが取れたらここで返す
		zone->free_prev = zone->frees;
		// if (zone->zone_type == YOYO_ZONE_SMALL) {
		// 	head->next = SET_FLAGS(head->next, YOYO_FLAG_SMALL);
		// } else {
		// 	head->next = UNSET_FLAGS(head->next, YOYO_FLAG_SMALL);
		// }
		return (void*)head + CEILED_CHUNK_SIZE;
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
			void*	mem = try_allocate_chunk_from_zone(current_zone, n);
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
	// print_zone_state(new_zone);
	// print_zone_bitmap_state(new_zone);
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
	(void)subarena;
	return allocate_memory_from_zone_list(arena, zone_type, n);
}

void*	yoyo_actual_malloc(size_t n) {
	// 実際の要求バイト数は n バイトでも, これが BLOCK_UNIT_SIZE の倍数に切り上げあられ, さらにチャンクヘッダも必要になるので, 
	// 実際には (BLOCKS_FOR_SIZE(n) + 1) * BLOCK_UNIT_SIZE バイト必要になる.
	const size_t	blocks_needed = BLOCKS_FOR_SIZE(n);
	const size_t	blocks_required = blocks_needed + 1;
	DEBUGOUT("blocks_required * BLOCK_UNIT_SIZE: %zu", blocks_required * BLOCK_UNIT_SIZE);
	if (blocks_needed == 0 || blocks_required * BLOCK_UNIT_SIZE == 0) {
		errno = ENOMEM;
		return NULL;
	}

	// [ゾーン種別決定]
	t_yoyo_zone_type zone_type = zone_type_for_bytes(n);

	// [アリーナを1つ選択してロックする]
	t_yoyo_arena* arena = occupy_arena(zone_type, n);
	if (arena == NULL) {
		DEBUGWARN("%s", "failed: couldn't lock any arena");
		return NULL;
	}

	// [アリーナからmalloc]
	void*	mem = allocate_from_arena(arena, zone_type, n);

	// [アリーナをアンロックする]
	unsigned int ai = arena->index;
	DEBUGOUT("unlocking arenas[%u] (%p) for %zu B", ai, arena, n);
	unlock_arena(arena, zone_type);
	DEBUGOUT("unlocked arenas[%u] (%p) for %zu B", ai, arena, n);
	// [埋め]
	fill_chunk_by_scribbler(mem, false);
	return mem;
}

void*	yoyo_actual_calloc(size_t count, size_t size) {
	if (count == 0 || size == 0) {
		count = 1;
		size = 1;
	}
	if (count != 0 && SIZE_MAX / count < size) {
		errno = ENOMEM;
		return NULL;
	}
	void* mem = yoyo_actual_malloc(count * size);
	if (mem) {
		yo_memset(mem, 0, count * size);
	}
	return mem;
}

void*	yoyo_actual_memalign(size_t alignment, size_t size) {
	// [alignemt のチェック]
	if (!is_power_of_2(alignment)) {
		errno = EINVAL;
		return NULL;
	}
	// [オーバーフローチェック]
	if (overflow_by_addtion(size, alignment)) {
		errno = ENOMEM;
		return NULL;
	}
	const size_t ceiled_size = CEIL_BY(size + alignment, BLOCK_UNIT_SIZE);
	if (ceiled_size == 0 || overflow_by_addtion(ceiled_size, CEILED_CHUNK_SIZE)) {
		errno = ENOMEM;
		return NULL;
	}
	// [malloc]
	const size_t	alloc_size = ceiled_size + CEILED_CHUNK_SIZE;
	void*	mem = yoyo_actual_malloc(alloc_size);
	if (mem == NULL) {
		return NULL;
	}
	// [アライメント調整]
	uintptr_t	ptr = (uintptr_t)mem;
	if (ptr % alignment == 0) {
		// 何もしなくても既にアラインされている
		return mem;
	}
	// 頑張ってアラインする
	t_yoyo_chunk*	actual_header = addr_to_actual_header(mem);
	ptr += CEILED_CHUNK_SIZE;

	size_t offset = (alignment - (ptr % alignment)) % alignment;
	void* pseudo_mem = (void*)(ptr + offset);
	// [擬似ヘッダの設定]
	// pseudo_mem に対して addr_to_actual_header を使ってはならない
	t_yoyo_chunk*	pseudo_header = pseudo_mem - CEILED_CHUNK_SIZE;
	pseudo_header->blocks = BLOCKS_FOR_SIZE(size) + 1;
	pseudo_header->next = SET_FLAGS(actual_header, YOYO_FLAG_PSEUDO_HEADER);
	assert((uintptr_t)pseudo_mem % alignment == 0);
	return pseudo_mem;
}

void*	yoyo_actual_aligned_alloc(size_t alignment, size_t size) {
	if (alignment == 0 || size % alignment != 0) {
		errno = EINVAL;
		return NULL;
	}
	return yoyo_actual_memalign(alignment, size);
}

int		yoyo_actual_posix_memalign(void **memptr, size_t alignment, size_t size) {
	if (memptr == NULL) {
		return EINVAL;
	}
	int init_errno = errno;
	void*	mem = aligned_alloc(alignment, size);
	if (mem == NULL) {
		int e = errno;
		errno = init_errno;
		return e;
	}
	errno = init_errno;
	assert((uintptr_t)mem % alignment == 0);
	*memptr = mem;
	return 0;
}
