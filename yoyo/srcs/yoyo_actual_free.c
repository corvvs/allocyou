#include "yoyo_internal.h"


static t_yoyo_zone*	get_zone_of_chunk(const t_yoyo_chunk* chunk) {
	const size_t whole_blocks = chunk->blocks;
	assert(whole_blocks > 0);
	const size_t usable_blocks = whole_blocks - 1;
	assert(usable_blocks > 0);

	const t_yoyo_zone_type zone_type = zone_type_for_bytes(usable_blocks * BLOCK_UNIT_SIZE);
	DEBUGOUT("zone_type is %d", zone_type);
	assert(zone_type != YOYO_ZONE_LARGE);

	const uintptr_t zone_addr_mask = ~((zone_type == YOYO_ZONE_TINY ? ZONE_TINY_BYTE : ZONE_SMALL_BYTE) - 1);
	DEBUGOUT("zone_addr_mask is %lx", zone_addr_mask);
	return (t_yoyo_zone*)((uintptr_t)chunk & zone_addr_mask);
}

// フリーリスト上のチャンク front_chunk とその次の chunk を可能なら統合する
static void	try_unite_free_chunks(t_yoyo_zone* zone, t_yoyo_chunk* front_chunk) {
	// [連結可能性を判定]
	if (front_chunk == NULL) { return; }
	t_yoyo_chunk*	back_chunk = NEXT_OF(front_chunk);
	DEBUGOUT("TRY to unite %p (%zu) + %p", front_chunk, front_chunk->blocks, back_chunk);
	if (back_chunk == NULL)	{ return; }
	unsigned int	front_index = get_block_index(zone, front_chunk);
	unsigned int	back_index = get_block_index(zone, back_chunk);
	if (front_index + front_chunk->blocks != back_index) { return; }
	// [連結可能なので連結する]
	DEBUGOUT("UNITE %p (%zu) + %p", front_chunk, front_chunk->blocks, back_chunk);
	front_chunk->next = back_chunk->next;
	front_chunk->blocks += back_chunk->blocks;
	unmark_chunk(zone, back_chunk);
}

static void insert_chunk_to_tiny_small_zone(t_yoyo_zone* zone, t_yoyo_chunk* chunk) {
	DEBUGOUT("zone->frees: %p", zone->frees);
	DEBUGOUT("chunk      : %p", chunk);
	t_yoyo_chunk**	current_lot = &(zone->frees);
	t_yoyo_chunk*	front = NULL;
	const size_t	chunk_blocks = chunk->blocks; // 後で使う

	// [挿入場所を見つける]
	while (true) {
		t_yoyo_chunk*	back = ADDRESS_OF(*current_lot);
		DEBUGOUT("front: %p, back: %p", front, back);
		if (back == NULL) {
			DEBUGOUT("PUSH BACK a chunk %p into %p (back of %p)", chunk, current_lot, front);
			break;
		}
		if ((uintptr_t)chunk < (uintptr_t)back) {
			DEBUGOUT("INSERT a chunk %p between %p and %p", chunk, front, back);
			break;
		}
		front = back;
		current_lot = &(back->next);
	}
	// [後挿入操作]
	chunk->next = COPY_FLAGS(ADDRESS_OF(*current_lot), chunk->next);
	// [合体できるなら合体]
	try_unite_free_chunks(zone, chunk);
	// [前挿入操作]
	*current_lot = chunk;
	try_unite_free_chunks(zone, front);
	// [zone のblocks を変更する]
	zone->blocks_free += chunk_blocks;
	zone->blocks_used -= chunk_blocks;
}

static void	free_from_tiny_small_zone(t_yoyo_chunk* chunk) {
	// chunk の所属 zone に飛ぶ
	t_yoyo_zone*	zone = get_zone_of_chunk(chunk);
	DEBUGOUT("zone: %p", zone);
	// [ロックを取る]
	if (!lock_zone(zone)) {
		DEBUGERR("FAILED to lock zone: %p", zone);
		return;
	}
	print_zone_state(zone);
	print_zone_bitmap_state(zone);
	// [zone のfreeマップの状態を変更する]
	mark_chunk_as_free(zone, chunk);
	// [zone のフリーリストに chunk を挿入する]
	insert_chunk_to_tiny_small_zone(zone, chunk);
	// [zone のロックを離す]
	print_zone_state(zone);
	print_zone_bitmap_state(zone);
	if (!unlock_zone(zone)) {
		DEBUGERR("FAILED to unlock zone: %p", zone);
	}
}

static void	free_from_large_zone(t_yoyo_chunk* chunk) {
	// [LARGE ヘッダを見る]
	t_yoyo_large_chunk*	large_chunk = (void*)chunk - CEILED_LARGE_CHUNK_SIZE;
	DEBUGOUT("LARGE chunk: %p", large_chunk);

	// [LARGEヘッダのto_zoneからLARGE zoneに飛ぶ]
	t_yoyo_large_arena*	subarena = large_chunk->subarena;
	DEBUGOUT("LARGE subarena: %p", subarena);

	// [LARGE zone のロックを取る]
	if (!lock_subarena((t_yoyo_subarena*)subarena)) {
		DEBUGERR("FAILED to lock LARGE subarena: %p", subarena);
		return;
	}

	// [LARGE zoneのchunk_listを見る]
	t_yoyo_large_chunk**	list = &(subarena->allocated);
	DEBUGOUT("subarena: %p", subarena);
	DEBUGOUT("subarena->allocated: %p", subarena->allocated);

	// [LARGEヘッダのactual_nextを見ながら元々freeしたいchunkを見つける]
	while (true) {
		t_yoyo_large_chunk*	head = *list;
		DEBUGOUT("head: %p", head);
		if (head == NULL) {
			DEBUGERR("FATAL: not found in the list: %p", &subarena->allocated);
			unlock_subarena((t_yoyo_subarena*)subarena);
			return;
		}
		if (head == large_chunk) {
			DEBUGOUT("POP FRONT chunk %p from the list: %p", large_chunk, &subarena->allocated);
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

	// [chunkをmunmapする]
	unmap_memory(large_chunk, large_chunk->memory_byte);

	// [LARGE zoneのロックを離す]
	unlock_subarena((t_yoyo_subarena*)subarena);
}

void	actual_free(void* addr) {
	if (addr == NULL) {
		DEBUGSTR("addr is NULL; DO NOTHING");
		return;
	}
	t_yoyo_chunk*	chunk = addr - CEILED_CHUNK_SIZE;

	if (IS_LARGE_CHUNK(chunk)) {
		// LARGE chunk を解放する.
		free_from_large_zone(chunk);
		return;
	}
	// TINY / SMALL の解放処理
	free_from_tiny_small_zone(chunk);
}
