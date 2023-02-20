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

// addr が malloc された領域ならば解放する.
void	yo_free_actual(void *addr) {
	if (addr == NULL) {
		DEBUGSTR("freeing NULL");
		return;
	}

	DEBUGOUT("** addr: %p **", addr);
	t_block_header *head = addr;
	--head;
	DEBUGOUT("head: %p, next: %p", head, head->next);

	// [ゾーンの取得]
	t_yo_zone_class	zone_class = _yo_zone_for_addr(addr);
	if (zone_class == YO_ZONE_LARGE) {
		// ラージセクション -> munmapする
		_yo_free_large_chunk(head);
		return;
	}

	// [アロケーションリストからの除去]
	// 対応するゾーンから, addr を挟むブロックを探してくる.
	t_yo_zone	*zone = _yo_retrieve_zone_for_class(zone_class);

	// addr が malloc された領域なら, addr から sizeof(t_block_header) だけ下がったところにブロックヘッダがあるはず.
	t_block_header *prev_free = find_inf_item(zone->frees, head);
	t_block_header *next_free = prev_free == NULL ? zone->frees : list_next_head(prev_free);

	check_double_free(head, next_free);
	check_free_invalid_address(head, prev_free);

	// アロケーションリストから除去すべきブロックが見つかったので除去する.
	remove_item(&zone->allocated, head);

	// [フリーリストへの追加]
	// show_list(zone->frees);

	// prev_free があるなら, それは head より小さい最大の(=左隣の)ブロックヘッダ.
	if (prev_free) {
		const void	*left_adjacent_to_prev_free = prev_free + (prev_free->blocks + 1);
		if (left_adjacent_to_prev_free == head) {
			// prev_free と head がくっついている -> 統合する
			prev_free->blocks += head->blocks + 1;
			prev_free->next = COPYFLAGS(next_free, prev_free->next);
			*head = (t_block_header){};
			head = prev_free;
		} else {
			prev_free->next = COPYFLAGS(head, FLAGS(prev_free->next));
		}
	} else {
		// prev_free がない -> head が最小のブロックヘッダ
		zone->frees = head;
	}

	// show_list(zone->frees);
	// next_free があるなら, それは head より大きい最小の(=右隣の)ブロックヘッダ.
	if (next_free) {
		const void	*left_adjacent_to_head = head + (head->blocks + 1);
		if (left_adjacent_to_head == next_free) {
			// head と next_free がくっついている -> 統合する
			head->blocks += next_free->blocks + 1;
			head->next = next_free->next;
			*next_free = (t_block_header){};
		} else {
			head->next = COPYFLAGS(next_free, FLAGS(head->next));
		}
	}
	// show_list(zone->frees);

	zone->free_p = head;
	DEBUGSTR("** free end **");
}


// n バイト**以上**の領域を確保して返す.
void*	yo_malloc_actual(size_t n) {
	size_t	blocks_needed = BLOCKS_FOR_SIZE(n);
	DEBUGOUT("** bytes: %zu, blocks: %zu **", n, blocks_needed);

	// [ゾーンの取得]
	t_yo_zone_class	zone_class = _yo_zone_for_bytes(n);

	if (zone_class == YO_ZONE_LARGE) {
		// ラージセクション
		DEBUGWARN("required size %zu(B) is for LARGE", n);
		return _yo_large_malloc(n);
	}

	t_yo_zone	*zone = _yo_retrieve_zone_for_class(zone_class);
	
	if (zone->frees == NULL) {
		DEBUGSTR("allocating arena...");
		zone->frees = _yo_allocate_heap(zone->heap_blocks, zone_class);
		zone->free_p = zone->frees;
		zone->cons.total_blocks += zone->frees->blocks + 1;
	}
	if (zone->frees == NULL) {
		// g_root.frees 確保失敗
		errno = ENOMEM;
		DEBUGERR("failed to allocate zone for class: %d", zone_class);
		return NULL;
	}
	assert(zone->frees != NULL);
	assert(zone->free_p != NULL);
	// 要求されるサイズ以上のチャンクが空いていないかどうか探す
	// (prev, next) is:
	// - free_p-> next がある:
	//   - (free_p, free_p->next)
	// - ない:
	//   - (NULL, frees)
	t_block_header	*head;
	t_block_header	*prev;
	if (list_next_head(zone->free_p) != NULL) {
		prev = zone->free_p;
		head = list_next_head(prev);
	} else {
		prev = NULL;
		head = zone->frees;
	}
	// t_block_header	*head = zone->frees;
	// t_block_header	*prev = NULL;
	int				visited_freep = 0;
	DEBUGOUT("blocks_needed = %zu", blocks_needed);
	while (1) {
		// DEBUGOUT("head = %p, free_p = %p", head, zone->free_p);
		if (blocks_needed <= head->blocks) {
			// 適合するチャンクがあった -> 後処理を行う
			// ブロックヘッダの次のブロックを返す
			t_block_header	*rv = head + 1;
			if (prev != NULL) {
				DEBUGOUT("FOUND: prev: (%zu, %p, %p)", prev->blocks, prev, prev->next);
			}
			DEBUGOUT("FOUND: head: (%zu, %p, %p)", head->blocks, head, head->next);
			check_consistency();

			// 見つかったチャンクのうち, 最初の blocks_needed 個を除去する
			// head->blocks がちょうど blocks_needed と一致しているかどうかで場合分け
			t_block_header	*new_free;
			if (head->blocks - 1 <= blocks_needed) {
				// -> 見つかったチャンクを丸ごと使い尽くす
				new_free = list_next_head(head);
				// !! head が NULL である場合の考慮が必要 !!
			} else {
				// -> 見つかったチャンクの一部が残る
				new_free = head + blocks_needed + 1;
				new_free->blocks = head->blocks - (blocks_needed + 1);
				new_free->next = head->next;
				head->blocks = blocks_needed;
			}
			// head を zone->frees につなぐ
			if (prev != NULL) {
				prev->next = new_free;
				zone->free_p = prev;
			} else {
				zone->frees = new_free;
				zone->free_p = zone->frees;
			}
			head->next = set_for_zone(NULL, zone_class);
			insert_item(&zone->allocated, head);
			DEBUGSTR("** malloc end **");
			return rv;
		}
		prev = head;
		head = list_next_head(head);
		if (head == NULL) {
			head = zone->frees;
			prev = NULL;
			DEBUGSTR("TUNRNED BACK to front of frees");
		}
		if (head == zone->free_p) {
			if (visited_freep) {
				break;
			}
			visited_freep = 1;
		}
		if (head == NULL) {
			break;
		}
	}
	// 適合するチャンクがなかった
	DEBUGOUT("NO ENOUGH BLOCKS -> extend current zone: %p", zone);
	t_block_header	*new_heap = _yo_allocate_heap(zone->heap_blocks, zone_class);
	if (new_heap == NULL) {
		errno = ENOMEM;
		return NULL;
	}
	check_consistency();
	zone->cons.total_blocks += new_heap->blocks + 1;
	yo_free_actual(new_heap + 1);
	check_consistency();
	return yo_malloc_actual(n);
}

void*	yo_realloc(void *addr, size_t n) {
	DEBUGOUT("** addr: %p, size: %zu **", addr, n);

	if (addr == NULL) {
		// -> malloc に移譲する
		DEBUGWARN("addr == NULL -> delegate to malloc(%zu)", n);
		return yo_malloc_actual(n);
	}

	t_block_header	*head = addr;
	--head;
	t_yo_zone_class	zone_class = _yo_zone_for_addr(addr);
	if (zone_class == YO_ZONE_LARGE) {
		if (n > head->blocks * BLOCK_UNIT_SIZE) {
			DEBUGSTR("** RELOCATE LARGE!! **");
			return _yo_relocate_chunk(head, n);
		}
		DEBUGSTR("do nothing for large chunk");
		return addr;
	}

	t_yo_zone	*zone = _yo_retrieve_zone_for_class(zone_class);
	if (n > head->blocks * BLOCK_UNIT_SIZE) {

		// 後続に十分な空きチャンクがあるなら, それらを現在のチャンクに取り込む.
		t_block_header*	extended = _yo_try_extend_chunk(zone, head, n);
		if (extended != NULL) {
			return extended;
		}

		// 後続に十分な空きチャンクがない場合はリロケートする
		DEBUGSTR("** RELOCATE!! **");
		return _yo_relocate_chunk(head, n);
	} else {
		// -> shrink
		return _yo_shrink_chunk(head, n);
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

