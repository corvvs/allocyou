#include "internal.h"

// フリーリスト上のチャンク front_chunk とその次の chunk を可能なら統合する
// 統合できたかどうかを返す
static bool	try_unite_free_chunks(t_yoyo_zone* zone, t_yoyo_chunk* front_chunk) {
	// [連結可能性を判定]
	if (front_chunk == NULL) { return false; }
	t_yoyo_chunk*	back_chunk = NEXT_OF(front_chunk);
	DEBUGOUT("TRY to unite %p (%zu) + %p", front_chunk, front_chunk->blocks, back_chunk);
	if (back_chunk == NULL)	{ return false; }
	unsigned int	front_index = get_block_index(zone, front_chunk);
	unsigned int	back_index = get_block_index(zone, back_chunk);
	if (front_index + front_chunk->blocks != back_index) { return false; }
	// [連結可能なので連結する]
	// DEBUGOUT("UNITE %p (%zu) + %p", front_chunk, front_chunk->blocks, back_chunk);
	front_chunk->next = back_chunk->next;
	front_chunk->blocks += back_chunk->blocks;
	unmark_chunk(zone, back_chunk);
	return true;
}

static void insert_chunk_to_tiny_small_zone(t_yoyo_zone* zone, t_yoyo_chunk* chunk) {
	DEBUGOUT("zone->frees: %p", zone->frees);
	DEBUGOUT("chunk      : %p", chunk);
	const size_t	chunk_blocks = chunk->blocks; // 後で使う
	t_yoyo_chunk**	current_lot = &(zone->frees);
	t_yoyo_chunk*	left = NULL;
	t_yoyo_chunk*	right = ADDRESS_OF(*current_lot);

	// [挿入場所を見つける]
	while (!(right == NULL) && !((uintptr_t)chunk < (uintptr_t)right)) {
		DEBUGOUT("front: %p, back: %p", left, right);
		left = right;
		current_lot = &(right->next);
		right = ADDRESS_OF(*current_lot);
	}
	if (right == NULL) {
		DEBUGOUT("PUSH BACK a chunk %p into %p (back of %p)", chunk, current_lot, left);
	} else {
		DEBUGOUT("INSERT a chunk %p between %p and %p", chunk, left, right);
	}
	// [後挿入操作]
	chunk->next = COPY_FLAGS(ADDRESS_OF(*current_lot), chunk->next);
	bool	is_unified;
	// [合体できるなら合体]
	is_unified = try_unite_free_chunks(zone, chunk);
	// [前挿入操作]
	*current_lot = chunk;
	is_unified = try_unite_free_chunks(zone, left);
	zone->free_prev = is_unified ? left : chunk;
	assert(zone->free_prev != NULL);
	// [zone のblocks を変更する]
	zone->blocks_free += chunk_blocks;
	zone->blocks_used -= chunk_blocks;
}

void	yoyo_free_from_locked_tiny_small_zone(t_yoyo_zone* zone, t_yoyo_chunk* chunk) {
	// print_zone_state(zone);
	// print_zone_bitmap_state(zone);
	// [zone のfreeマップの状態を変更する]
	mark_chunk_as_free(zone, chunk);
	// [zone のフリーリストに chunk を挿入する]
	insert_chunk_to_tiny_small_zone(zone, chunk);
	// print_zone_state(zone);
	// print_zone_bitmap_state(zone);
	// [DEBUG: 埋め]
	fill_chunk_by_scribbler((void*)chunk + CEILED_CHUNK_SIZE, true);
}


static void	free_from_tiny_small_zone(t_yoyo_chunk* chunk) {
	// chunk の所属 zone に飛ぶ
	t_yoyo_zone*	zone = get_zone_of_chunk(chunk);
	assert(zone != NULL);
	DEBUGOUT("zone: %p", zone);
	// [ロックを取る]
	if (!lock_zone(zone)) {
		return;
	}
	if (is_header_and_used(zone, chunk)) {
		yoyo_free_from_locked_tiny_small_zone(zone, chunk);
	} else {
		DEBUGFATAL("chunk header %p is not an actual header addr or not a using chunk", chunk);
		assert(false);
	}
	// [zone のロックを離す]
	unlock_zone(zone);
}

static void	free_from_large_zone(t_yoyo_chunk* chunk) {
	// [LARGE ヘッダを見る]
	t_yoyo_large_chunk*	large_chunk = (void*)chunk - CEILED_LARGE_CHUNK_SIZE;
	DEBUGOUT("LARGE chunk: %p", large_chunk);

	// [LARGEヘッダのto_zoneからLARGE zoneに飛ぶ]
	t_yoyo_large_arena*	subarena = large_chunk->subarena;
	const size_t chunk_blocks = chunk->blocks;
	(void)chunk_blocks;
	DEBUGOUT("locking LARGE subarena %p for free (%p, %zu)", subarena, chunk, chunk_blocks);
	// [LARGE zone のロックを取る]
	if (!lock_subarena((t_yoyo_subarena*)subarena)) {
		DEBUGFATAL("FAILED to lock LARGE subarena: %p", subarena);
		return;
	}
	DEBUGOUT("locked LARGE subarena %p for free (%p, %zu)", subarena, chunk, chunk_blocks);

	// [LARGE zoneのchunk_listを見る]
	t_yoyo_large_chunk**	list = &(subarena->allocated);

	// [LARGEヘッダのactual_nextを見ながら元々freeしたいchunkを見つける]
	while (true) {
		t_yoyo_large_chunk*	head = *list;
		// DEBUGOUT("head: %p", head);
		if (head == NULL) {
			DEBUGFATAL("not found in the list: %p", &subarena->allocated);
			unlock_subarena((t_yoyo_subarena*)subarena);
			return;
		}
		if (head == large_chunk) {
			// DEBUGOUT("POP FRONT chunk %p from the list: %p", large_chunk, &subarena->allocated);
			list = NULL;
			break;
		}
		t_yoyo_large_chunk*	next = head->large_next;
		if (next == large_chunk) {
			DEBUGOUT("REMOVE chunk %p next of %p", large_chunk, head);
			break;
		}
		list = &(head->large_next);
	}

	// [freeしたいchunkをchunk listから切り離す]
	if (list == NULL) {
		subarena->allocated = large_chunk->large_next;
	} else {
		(*list)->large_next = large_chunk->large_next;
	}
	large_chunk->large_next = NULL;

	// [DEBUG: munmap前に埋め]
	fill_chunk_by_scribbler((void*)large_chunk + LARGE_OFFSET_USABLE, true);
	// [chunkをmunmapする]
	yoyo_unmap_memory(large_chunk, large_chunk->memory_byte);
	// [LARGE zoneのロックを離す]
	DEBUGOUT("unlocking LARGE subarena %p for free (%p, %zu)", subarena, chunk, chunk_blocks);
	unlock_subarena((t_yoyo_subarena*)subarena);
	DEBUGOUT("unlocked LARGE subarena %p for free (%p, %zu)", subarena, chunk, chunk_blocks);
}

void	yoyo_actual_free(void* addr) {
	if (addr == NULL) {
		DEBUGSTR("addr is NULL; DO NOTHING");
		return;
	}
	t_yoyo_chunk*	chunk = addr_to_actual_header(addr);

	if (IS_LARGE_CHUNK(chunk)) {
		// LARGE chunk を解放する.
		free_from_large_zone(chunk);
		return;
	}
	// TINY / SMALL の解放処理
	free_from_tiny_small_zone(chunk);
}

size_t	yoyo_actual_malloc_usable_size(void *ptr) {
	if (ptr == NULL) {
		return 0;
	}
	assert((uintptr_t)ptr >= CEILED_CHUNK_SIZE);
	t_yoyo_chunk*	chunk = addr_to_nominal_header(ptr);
	assert(chunk->blocks >= 2);
	return (chunk->blocks - 1) * BLOCK_UNIT_SIZE;
}
