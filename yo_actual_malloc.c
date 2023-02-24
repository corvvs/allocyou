#include "yo_internal.h"

static bool	extend_zone(t_yo_zone *zone) {
	t_block_header	*new_heap = yo_allocate_heap(zone->heap_blocks, zone);
	if (new_heap == NULL) {
		errno = ENOMEM;
		return false;
	}
	yo_actual_free(new_heap + 1);
	return true;
}

// void* yo_actual_malloc(size_t n)
//
// [task]
// - n バイト**以上**のサイズを持つチャンクを確保し, その(ユーザが使用できる)領域の先頭アドレスを返す.
// - 上記が失敗した場合は NULL を返し, errno に ENOMEM をセットする.
// - n == 0 における動作は n == 1 と同じ.
//
// [mechanism]
// `n`(与えられた要求バイトサイズ)から, 必要なメモリブロック数`blocks_needed`を算出する.
// さらにゾーン種別`zone_class`を決定する.
// ゾーン種別は以下の3種類:
// - TINY  ... `blocks_needed`が「992バイトに対応するブロック数」以下の時
// - SMALL ... `blocks_needed`が「15360バイトに対応するブロック数」以下の時
// - LARGE ... 上記以外
//
// ゾーン種別が LARGE だった場合は, チャンクをリスト管理しない.
// 以下ゾーン種別が LARGE でないとする.
//
// ゾーン種別に対応するゾーン`zone`を取得する.
// この時, ゾーンがまだ存在していないなら mmap でゾーンを確保する.
// ゾーンのフリーリストから, ブロック数(ヘッダ除く)が`blocks_needed`以上であるようなチャンクを探す.
// 
// (チャンクが見つからなかった場合)
// 現在のゾーンに新しいヒープを追加し, 同じ引数でこの関数を呼び直す.
// 次はチャンクが見つかるはず.
//
// (チャンクが見つかった場合)
// 見つかったチャンクをフリーリストから除去し, アロケーションリストに挿入する.
// ただし, 見つかったチャンクのブロック数が`blocks_needed`より2ブロック以上小さい場合は,
// 見つかったチャンクを前後に分割し, 後の方をフリーリストに残す.
//
void*	yo_actual_malloc(size_t n) {
	size_t	blocks_needed = BLOCKS_FOR_SIZE(n);

	// [determine zone class]
	t_yo_zone_class	zone_class = yo_zone_for_bytes(n);

	if (zone_class == YO_ZONE_LARGE) {
		// [for LARGE zone]
		DEBUGWARN("required size %zu(B) is for LARGE", n);
		return yo_large_malloc(n);
	}

	// [retrieve zone]
	t_yo_zone	*zone = yo_retrieve_zone(zone_class);
	if (zone->max_chunk_bytes == 0) {
		errno = ENOMEM;
		DEBUGERR("FAIL: zone-allocation for class: %d", zone_class);
		return NULL;
	}
	if (zone->frees == NULL) {
		DEBUGOUT("NO FREE CHUNK -> extend current zone: %p", zone);
		if (!extend_zone(zone)) {
			DEBUGERR("FAIL: zone-extention for class: %d", zone_class);
			return NULL;
		}
	}
	assert(zone->frees != NULL);
	assert(zone->free_p != NULL);
	// [find vacant blocks for chunk]
	t_listcursor	cursor = init_cursor_from_mid(zone->frees, zone->free_p);
	assert(cursor.curr != NULL);
	while (cursor.curr != NULL && cursor.curr->blocks < blocks_needed) {
		increment_cursor(&cursor);
	}

	if (cursor.curr == NULL) {
		// [extend the zone for requirement]
		DEBUGOUT("NO ENOUGH BLOCKS -> extend current zone: %p", zone);
		if (!extend_zone(zone)) {
			return NULL;
		}
		return yo_actual_malloc(n);
	}

	void*	rv = cursor.curr + 1;

	// [get out a chunk]
	t_block_header	*rest_free;
	const size_t	chunk_size = blocks_needed + 1;
	if (chunk_size < cursor.curr->blocks) {
		// [only necessary blocks]
		rest_free = cursor.curr + chunk_size;
		*rest_free = *cursor.curr;
		rest_free->blocks -= chunk_size;
		cursor.curr->blocks = blocks_needed;
	} else {
		// [taking all blocks]
		rest_free = list_next_head(cursor.curr);
	}

	if (cursor.prev != NULL) {
		concat_item(cursor.prev, rest_free);
	} else {
		zone->frees = rest_free;
	}
	zone->free_p = (cursor.prev != NULL) ? cursor.prev : zone->frees;
	cursor.curr->next = COPYFLAGS(NULL, cursor.curr->next);
	insert_item(&zone->allocated, cursor.curr);
	const size_t	blocks_allocated = cursor.curr->blocks + 1;
	zone->cons.free_blocks -= blocks_allocated;
	zone->cons.used_blocks += blocks_allocated;
	return rv;
}
