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
	t_block_header *prev_free = find_item(zone->frees, head);
	t_block_header *next_free = prev_free == NULL ? zone->frees : ADDRESS(prev_free->next);

	check_double_free(head, next_free);
	check_free_invalid_address(head, prev_free);

	// アロケーションリストから除去すべきブロックが見つかったので除去する.
	remove_item(&zone->allocated, head);

	// [フリーリストへの追加]
	// curr_unused があるなら, それは head より大きい最小の(=右隣の)ブロックヘッダ.
	DEBUGOUT("curr_unused = %p", next_free);
	if (next_free) {
		if ((head + head->blocks + 1) == next_free) {
			// head と curr_unused がくっついている -> 統合する
			DEBUGOUT("head(%zu, %p) + curr_unused(%zu, %p)", head->blocks, head, next_free->blocks, next_free);
			head->blocks += next_free->blocks + 1;
			head->next = ADDRESS(next_free->next);
			next_free->blocks = 0;
			next_free->next = 0;
			DEBUGOUT("-> head(%zu, %p)", head->blocks, head);
		} else {
			head->next = next_free;
		}
	}
	// prev_unused があるなら, それは head より小さい最大の(=左隣の)ブロックヘッダ.
	DEBUGOUT("prev_unused = %p", prev_free);
	if (prev_free) {
		DEBUGOUT("head = %p", head);
		if (prev_free + (prev_free->blocks + 1) == head) {
			// prev_unused と head がくっついている -> 統合する
			DEBUGOUT("prev_unused(%zu, %p) + head(%zu, %p)", prev_free->blocks, prev_free, head->blocks, head);
			prev_free->blocks += head->blocks + 1;
			prev_free->next = ADDRESS(head->next);
			DEBUGOUT("-> prev_unused(%zu, %p)", prev_free->blocks, prev_free);
			head->blocks = 0;
			head->next = 0;
			head = prev_free;
		} else {
			prev_free->next = head;
		}
	} else {
		// prev_unused がない -> head が最小のブロックヘッダ
		zone->frees = head;
	}
	DEBUGSTR("** free end **");
}


// n バイト**以上**の領域を確保して返す.
void*	yo_malloc_actual(size_t n) {
	t_yo_zone_class	zone_class = _yo_zone_for_bytes(n);
	size_t	blocks_needed = BLOCKS_FOR_SIZE(n);
	DEBUGOUT("** bytes: %zu, blocks: %zu **", n, blocks_needed);

	if (zone_class == YO_ZONE_LARGE) {
		// 要求サイズが1つのバンチに収まらない
		// -> 専用のバンチを mmap して返す
		DEBUGWARN("required size %zu(B) is for LARGE", n);
		return _yo_large_malloc(n);
	}

	t_yo_zone	*zone = _yo_retrieve_zone_for_class(zone_class);
	
	if (zone->frees == NULL) {
		DEBUGSTR("allocating arena...");
		zone->frees = _yo_allocate_heap(zone->heap_blocks, zone_class);
	}
	if (zone->frees == NULL) {
		// g_root.frees 確保失敗
		return NULL;
	}
	DEBUGOUT("g_root.frees head: (%zu, %p, %p)", zone->frees->blocks, zone->frees, zone->frees->next);
	// 要求されるサイズ以上のチャンクが空いていないかどうか探す
	t_block_header	*head = zone->frees;
	t_block_header	*prev = NULL;
	DEBUGOUT("blocks_needed = %zu", blocks_needed);
	while (head != NULL) {
		if (blocks_needed <= head->blocks) {
			// 適合するチャンクがあった -> 後処理を行う
			// ブロックヘッダの次のブロックを返す
			t_block_header	*rv = head + 1;

			// 見つかったチャンクのうち, 最初の blocks_needed 個を除去する
			// head->blocks がちょうど blocks_needed と一致しているかどうかで場合分け
			t_block_header	*new_free;
			if (head->blocks - 1 <= blocks_needed) {
				// -> 見つかったチャンクを丸ごと使い尽くす
				DEBUGOUT("exhausted chunk: (%zu, %p, %p)", head->blocks, head, head->next);
				new_free = ADDRESS(head->next);
				// !! head が NULL である場合の考慮が必要 !!
			} else {
				// -> 見つかったチャンクの一部が残る
				new_free = head + blocks_needed + 1;

				// head->blocks + 1 = (blocks_needed + 1) + (rest_blocks + 1)
				// -> rest_blocks = head->blocks - (blocks_needed + 1)
				new_free->blocks = head->blocks - (blocks_needed + 1);
				new_free->next = ADDRESS(head->next);

				DEBUGOUT("shorten block: (%zu, %p) -> (%zu, %p)", head->blocks, head, new_free->blocks, new_free);
				head->blocks = blocks_needed;
			}
			// head を zone->frees につなぐ
			if (prev) {
				prev->next = new_free;
			} else {
				zone->frees = new_free;
			}
			head->next = set_for_zone(head->next, zone_class);
			DEBUGOUT("returning block: (%zu, %p, %p)", blocks_needed, head, head->next);
			insert_item(&zone->allocated, head);
			DEBUGSTR("** malloc end **");
			return rv;
		}
		prev = head;
		head = ADDRESS(head->next);
	}
	// 適合するチャンクがなかった
	DEBUGSTR("NO ENOUGH BLOCKS -> extend current zone");
	t_block_header	*new_heap = _yo_allocate_heap(zone->heap_blocks, zone_class);
	if (new_heap == NULL) {
		errno = ENOMEM;
		return NULL;
	}
	DEBUGOUT("new_heap = %p", new_heap);
	yo_free_actual(new_heap + 1);
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

static	void show_zone(t_yo_zone	*zone) {
	DEBUGSTRN("  allocated: "); show_list(zone->allocated);
	DEBUGSTRN("  free:      "); show_list(zone->frees);
}

void show_alloc_mem() {
	DEBUGSTR("TINY:");
	show_zone(&g_root.tiny);
	DEBUGSTR("SMALL:");
	show_zone(&g_root.small);
	DEBUGSTR("LARGE:  ");
	DEBUGSTRN("  used: "); show_list(g_root.large);
}
