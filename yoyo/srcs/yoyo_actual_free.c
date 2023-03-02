#include "yoyo_internal.h"


static void	free_from_normal_zone(t_yoyo_chunk* chunk) {
	// [まず chunk が TINY / SMALL のどちらかを判定する]
	// -> blocks から計算できるはず
	const size_t whole_blocks = chunk->blocks;
	assert(whole_blocks > 0);
	const size_t actual_blocks = whole_blocks - 1;
	assert(actual_blocks > 0);
	t_yoyo_zone_class zone_class = zone_class_for_bytes(actual_blocks * BLOCK_UNIT_SIZE);
	DEBUGOUT("zone_class is %d", zone_class);
	assert(zone_class != YOYO_ZONE_LARGE);

	// [対応する zone に飛ぶ]
	uintptr_t zone_mask = ~((zone_class == YOYO_ZONE_TINY ? ZONE_TINY_BYTE : ZONE_SMALL_BYTE) - 1);
	DEBUGOUT("zone_mask is %lx", zone_mask);
	t_yoyo_zone*	zone = (t_yoyo_zone*)((uintptr_t)chunk & zone_mask);
	DEBUGOUT("zone: %p", zone);

	// [ロックを取る]
	if (!lock_zone(zone)) {
		DEBUGERR("FAILED to lock zone: %p", zone);
		return;
	}
	
	// [zone のfreeマップの状態を変更する]
	mark_chunk_as_free(zone, chunk);
	// head ビットマップの状態は合体で変わるかもしれない

	// [zone のフリーリストに chunk を挿入する]
	DEBUGOUT("zone->frees: %p", zone->frees);
	DEBUGOUT("chunk      : %p", chunk);
	t_yoyo_chunk**	list = &(zone->frees);

	while (true) {
		t_yoyo_chunk*	head = ADDRESS_OF(*list);
		DEBUGOUT("head: %p", head);
		if (head == NULL) {
			DEBUGOUT("PUSH BACK a chunk %p to %p", chunk, list);
			break;
		}
		if ((uintptr_t)chunk < (uintptr_t)head) {
			DEBUGOUT("PUSH FRONT a chunk %p of %p", chunk, list);
			list = NULL;
			break;
		}
		t_yoyo_chunk*	next = NEXT_OF(head);
		if ((uintptr_t)chunk < (uintptr_t)next) {
			DEBUGOUT("INSERT a chunk %p next of %p", chunk, list);
			break;
		}
		list = &(head->next);
	}
	if (list == NULL) {
		// PUSH FRONT
		chunk->next = COPY_FLAGS(zone->frees, chunk->next);
		list = &(zone->frees);
	} else if (*list != NULL) {
		// INSERT
		chunk->next = COPY_FLAGS((*list)->next, chunk->next);
	} else {
		// PUSH BACK
		chunk->next = COPY_FLAGS(NULL, chunk->next);
	}
	*list = chunk;

	// [zone のblocks を変更する]
	zone->blocks_free += chunk->blocks;
	zone->blocks_used -= chunk->blocks;

	// [zone のロックを離す]
	if (!unlock_zone(zone)) {
		DEBUGERR("FAILED to unlock zone: %p", zone);
	}
}


void	actual_free(void* addr) {
	t_yoyo_chunk*	chunk = addr;
	--chunk;

	if (IS_LARGE_CHUNK(chunk)) {
		// TODO: LARGE chunk を解放する.
		return;
	}
	// TINY / SMALL の解放処理
	free_from_normal_zone(chunk);
}
