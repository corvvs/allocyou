#include "yo_internal.h"

void	check_double_free(t_block_header *to_free, t_block_header *next_free)
{
	// head が next_free と一致している -> なんかおかしい
	(void)to_free;
	(void)next_free;
	assert(to_free != next_free);
}

void	check_free_invalid_address(t_block_header *to_free, t_block_header *header)
{
	(void)to_free;
	(void)header;
	// header が別のチャンクの中にあるっぽい -> なんかおかしい
	assert(header == NULL || (header + header->blocks) < to_free);
}

// void yo_actual_free(void *addr)
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
void	yo_actual_free(void *addr) {
	if (addr == NULL) {
		DEBUGSTR("freeing NULL");
		return;
	}

	t_block_header*	head = addr;
	--head;
	DEBUGOUT("head: %p, next: %p", head, head->next);

	// [determine zone]
	t_yo_zone_class	zone_class = yo_zone_for_addr(addr);
	if (zone_class == YO_ZONE_LARGE) {
		// [for LARGE zone]
		yo_large_free(addr);
		return;
	}

	// [retrieve zone]
	t_yo_zone*		zone = yo_retrieve_zone(zone_class);
	t_block_header*	prev_free = find_inf_item(zone->frees, head);
	t_block_header*	next_free = prev_free == NULL ? zone->frees : list_next_head(prev_free);

	check_double_free(head, next_free);
	check_free_invalid_address(head, prev_free);

	const size_t	blocks_freed = head->blocks + 1;
	const bool	removed = remove_item(&zone->allocated, head);
	if (removed) {
		zone->cons.used_blocks -= blocks_freed;
	}

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
	zone->cons.free_blocks += blocks_freed;
	zone->free_p = head;
}
