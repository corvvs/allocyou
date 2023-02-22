#include "yo_internal.h"

void	check_double_free(t_block_header *to_free, t_block_header *next_free)
{
	// head が next_free と一致している -> なんかおかしい
	assert(to_free != next_free);
}

void	check_free_invalid_address(t_block_header *to_free, t_block_header *header)
{
	// header が別のチャンクの中にあるっぽい -> なんかおかしい
	assert(header == NULL || (header + header->blocks) < to_free);
}

void	*set_for_zone(void *addr, t_yo_zone_class zone)
{
	switch (zone) {
		case YO_ZONE_TINY:
			return SET_IS_TINY(addr);
		case YO_ZONE_SMALL:
			return SET_IS_SMALL(addr);
		case YO_ZONE_LARGE:
			return SET_IS_LARGE(addr);
	}
}

static bool	extend_zone(t_yo_zone *zone) {
	t_block_header	*new_heap = _yo_allocate_heap(zone->heap_blocks, zone->zone_class);
	if (new_heap == NULL) {
		errno = ENOMEM;
		return false;
	}
	zone->cons.total_blocks += new_heap->blocks + 1;
	yo_free_actual(new_heap + 1);
	return true;
}

static void	nullify_chunk(t_block_header *chunk) {
	*chunk = (t_block_header){};
}

static t_block_header	*unify_chunk(t_block_header *b1, t_block_header *b2) {
	b1->blocks += b2->blocks + 1;
	concat_item(b1, b2->next);
	nullify_chunk(b2);
	return b1;
}


// void yo_free_actual(void *addr)
//
// [task]
// - `addr`が malloc された領域の先頭であれば, それを未使用領域に戻す.
// - `addr`が NULL ならば, なにもしない.
// - それ以外の場合の動作は未定義.
//
// [mechanism]
// `addr`からヘッダ1つ分前のアドレスがチャンクのヘッダ`head`であると仮定し,
// `head`から対応するゾーンを取得する.
// ゾーンが LARGE である場合は当該チャンクを即座に unmap する.
//
// 以下, ゾーンが LARGE でないとする.
// まず, `head`をアロケーションリストから除去する.
// 続いて, ゾーンのフリーリストから, `prev` < `head` < `curr` となるような隣接する2要素`prev`, `curr`を探す.
// この後フリーリストの`prev`と`curr`の間に`head`を挿入するのだが,
// `prev`と`head`, および`head`と`curr`が「隣接」している場合は1つのチャンクに統合する.
// ここで「隣接」とは, `2つの領域が隙間なく連続していることを意味する.
// 例えば, `head`と`curr`が隣接しているとは, `head + (head->blocks + 1) == curr` が成り立つこと.
//
void	yo_free_actual(void *addr) {
	if (addr == NULL) {
		DEBUGSTR("freeing NULL");
		return;
	}

	t_block_header*	head = addr;
	--head;
	DEBUGOUT("head: %p, next: %p", head, head->next);

	// [determine zone]
	t_yo_zone_class	zone_class = _yo_zone_for_addr(addr);
	if (zone_class == YO_ZONE_LARGE) {
		// [for LARGE zone]
		_yo_free_large_chunk(head);
		return;
	}

	// [retrieve zone]
	t_yo_zone*		zone = _yo_retrieve_zone_for_class(zone_class);
	t_block_header*	prev_free = find_inf_item(zone->frees, head);
	t_block_header*	next_free = prev_free == NULL ? zone->frees : list_next_head(prev_free);

	check_double_free(head, next_free);
	check_free_invalid_address(head, prev_free);

	remove_item(&zone->allocated, head);

	if (prev_free) {
		const bool is_unifiable = (prev_free + (prev_free->blocks + 1)) == head;
		if (is_unifiable) {
			t_block_header	*unified = unify_chunk(prev_free, head);
			concat_item(unified, next_free);
			head = unified;
		} else {
			concat_item(prev_free, head);
		}
	} else {
		zone->frees = head;
	}

	if (next_free) {
		const bool is_unifiable = (head + (head->blocks + 1)) == next_free;
		if (is_unifiable) {
			unify_chunk(head, next_free);
		} else {
			concat_item(head, next_free);
		}
	}

	zone->free_p = head;
}

// void* yo_malloc_actual(size_t n)
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
void*	yo_malloc_actual(size_t n) {
	size_t	blocks_needed = BLOCKS_FOR_SIZE(n);

	// [determine zone class]
	t_yo_zone_class	zone_class = _yo_zone_for_bytes(n);

	if (zone_class == YO_ZONE_LARGE) {
		// [for LARGE zone]
		DEBUGWARN("required size %zu(B) is for LARGE", n);
		return _yo_large_malloc(n);
	}

	// [retrieve zone]
	t_yo_zone	*zone = _yo_retrieve_zone_for_class(zone_class);
	if (zone->frees == NULL) {
		// g_root.frees 確保失敗
		errno = ENOMEM;
		DEBUGERR("failed to allocate zone for class: %d", zone_class);
		return NULL;
	}
	assert(zone->frees != NULL);
	assert(zone->free_p != NULL);
	// [find vacant blocks for chunk]
	t_listcursor	cursor = init_cursor_from_mid(zone->frees, zone->free_p);
	assert(cursor.curr != NULL);
	DEBUGOUT("blocks_needed = %zu", blocks_needed);
	while (cursor.curr != NULL && cursor.curr->blocks < blocks_needed) {
		increment_cursor(&cursor);
	}

	if (cursor.curr == NULL) {
		// [extend the zone for requirement]
		DEBUGOUT("NO ENOUGH BLOCKS -> extend current zone: %p", zone);
		check_consistency();
		if (!extend_zone(zone)) {
			return NULL;
		}
		check_consistency();
		return yo_malloc_actual(n);
	}

	void*	rv = cursor.curr + 1;
	check_consistency();

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
	if (cursor.prev != NULL) {
		zone->free_p = cursor.prev;
	} else {
		zone->free_p = zone->frees;
	}
	cursor.curr->next = set_for_zone(NULL, zone_class);
	insert_item(&zone->allocated, cursor.curr);
	DEBUGOUT("** returning %p **", cursor.curr);
	return rv;
}

// void* yo_realloc_actual(void *addr, size_t n)
//
// [task]
// - `addr`が NULL の場合は, malloc(n) に処理を移譲する.
// - `addr`が malloc されたチャンクである場合:
//   - まずチャンクのアドレスを変えないままチャンクサイズの変更を試みる.
//   - 変更ができない場合は, malloc(n) の後, 確保されたチャンクに元のチャンクのデータをすべてコピーし, 元のチャンクを解放する.
// - いずれでもない場合の動作は未定義.
//
// [mechanism]
// `addr`は NULL でないとする.
// 大まかな手順は:
// - チャンクを引っ越す(リロケートする)必要があるかどうか判定する.
// - リロケートする場合は, malloc, copy and free を行う.
// - リロケートしない場合は, チャンクのサイズ調整を行う.
//
// (リロケーションの要否の決定)
// `n`(与えられた要求バイトサイズ)から, 必要なメモリブロック数`blocks_needed`を算出する.
// さらに`blocks_needed`から対応するゾーン種別`zone_class_to`を決定する.
// また, `addr`からヘッダ1つ分前のアドレスがチャンクのヘッダ`head`であると仮定し,
// チャンクのブロック数`blocks_current`と対応するゾーン種別`zone_class_from`を決定する.
// zone_class_from != zone_class_to なら** その時点でリロケーションが必要 **.
// zone_class_from == zone_class_to である場合,
// 対象チャンクのブロック数を`blocks_needed`ブロックに変更できるかどうかを調べる.
// ** 変更できない場合はリロケーションが必要 **.
//
// (リロケーションする場合)
// malloc し, データコピーし, free して 新しいアドレスを返して終わり.
//
// (リロケーションしない場合)
// データコピーは不要.
// 以下の3パターンが考えられる:
// `blocks_needed`と`blocks_current`の大小に応じてやることが違う.
// - blocks_current - 1 <= blocks_needed <= blocks_current
//   - 何もしない.
//   - ブロック数は小さくなるのだが, 小さくしても新しいブロックを生成できないため.
// - blocks_needed < blocks_current - 1
//   - 必要なブロック数が現在よりも減る.
//   - 現在のチャンクのブロック数を小さくし, 余ったブロックを新たな未使用チャンクとする.
// - blocks_current < blocks_needed
//   - 必要なブロック数が現在よりも増える.
//   - 現在のチャンクのすぐ右に十分なブロックを持つ未使用チャンクがある(事前にあることを確認している).
//   - この未使用チャンクを解体し, 現在チャンクに合体させる.
//
void*	yo_realloc_actual(void *addr, size_t n) {
	if (addr == NULL) {
		DEBUGWARN("addr == NULL -> delegate to malloc(%zu)", n);
		return yo_malloc_actual(n);
	}

	t_block_header	*head = addr;
	--head;
	const bool	blocks_increasing = BLOCKS_FOR_SIZE(n) > head->blocks;
	t_yo_zone_class	zone_class = _yo_zone_for_addr(addr);
	if (zone_class == YO_ZONE_LARGE) {
		if (blocks_increasing) {
			DEBUGSTR("** RELOCATE LARGE!! **");
			return _yo_relocate_chunk(head, n);
		}
		DEBUGSTR("do nothing for large chunk");
		return addr;
	}

	t_yo_zone	*zone = _yo_retrieve_zone_for_class(zone_class);
	if (blocks_increasing) {
		t_block_header*	extended = _yo_try_extend_chunk(zone, head, n);
		if (extended != NULL) {
			return extended;
		}

		return _yo_relocate_chunk(head, n);
	} else {
		// -> shrink
		_yo_shrink_chunk(head, n);
		return addr;
	}
}

static double	get_fragmentation_rate(t_block_header *list)
{
	size_t	blocks = 0;
	size_t	headers = 0;
	while (list != NULL) {
		blocks += list->blocks;
		headers += 1;
		list = list_next_head(list);
	}
	return blocks > 0 ? (double)headers / (double)blocks : 0;
}

static void	show_zone(t_yo_zone *zone) {
	DEBUGSTRN("  allocated: "); show_list(zone->allocated);
	DEBUGSTRN("  free:      "); show_list(zone->frees);
	DEBUGOUT( "  fragmentation: %1.4f%%", get_fragmentation_rate(zone->frees) * 100);
}

void	show_alloc_mem(void) {
	DEBUGSTR("TINY:");
	show_zone(&g_root.tiny);
	DEBUGSTR("SMALL:");
	show_zone(&g_root.small);
	DEBUGSTR("LARGE:  ");
	DEBUGSTRN("  used: "); show_list(g_root.large);
}

void	check_zone_consistency(t_yo_zone *zone) {
	t_block_header	*h;
	size_t block_in_use = 0;
	size_t block_free = 0;
	h = zone->frees;
	while (h) {
		assert(h->blocks > 0);
		block_free += h->blocks + 1;
		h = list_next_head(h);
	}
	h = zone->allocated;
	while (h) {
		assert(h->blocks > 0);
		block_in_use += h->blocks + 1;
		h = list_next_head(h);
	}
	ssize_t block_diff = zone->cons.total_blocks - (block_free + block_in_use);
	DEBUGOUT("total: %zu, free: %zu, in use: %zu, diff: %zd blocks",
			zone->cons.total_blocks, block_free, block_in_use, block_diff);
	DEBUGOUT("fragmentation: %1.4f%%", get_fragmentation_rate(zone->frees) * 100);
	// DEBUGSTRN("  free:      "); show_list(zone->frees);
	if (block_diff) {
		// show_alloc_mem();
		// DEBUGSTRN("  free:      "); show_list(zone->frees);
		DEBUGERR("consistency KO!!: zone %p", zone);
		assert(zone->cons.total_blocks == block_free + block_in_use);
	}
}

void	check_consistency(void) {
	if (g_root.tiny.frees) {
		DEBUGSTR("check consistency: TINY");
		check_zone_consistency(&g_root.tiny);
		DEBUGSTR("-> ok.");
	}
	if (g_root.small.frees) {
		DEBUGSTR("check consistency: SMALL");
		check_zone_consistency(&g_root.small);
		DEBUGSTR("-> ok.");
	}
}

