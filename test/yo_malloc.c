#include "yo_internal.h"

// addr が malloc された領域ならば解放する.
void	yo_free_actual(void *addr) {
	if (addr == NULL) {
		DEBUGSTR("freeing NULL");
		return;
	}

	t_yo_zone_class	zone_class = _yo_zone_for_addr(addr);

	DEBUGOUT("** addr: %p **", addr);
	t_block_header *head = addr;
	--head;
	DEBUGOUT("head: %p, next: %p", head, head->next);

	if (zone_class == YO_ZONE_LARGE) {
		// ラージセクション -> munmapする
		_yo_free_large_chunk(head);
		return;
	}

	t_yo_zone	*zone = _yo_retrieve_zone_for_class(zone_class);

	// addr が malloc された領域なら, addr から sizeof(t_block_header) だけ下がったところにブロックヘッダがあるはず.
	t_block_header *prev_unused = find_item(zone->frees, head);
	t_block_header *curr_unused = prev_unused == NULL ? zone->frees : ADDRESS(prev_unused->next);
	// assertion:
	// (!prev_unused || prev_unused < addr) && addr <= curr_unused
	if (head == curr_unused) {
		// head が curr_unused と一致している -> なんかおかしい
		DEBUGERR("SOMETHING WRONG: head is equal to free-block-header: %p", head);
		return;
	}
	if (prev_unused != NULL && head <= (prev_unused + prev_unused->blocks)) {
		// head が前のチャンクの中にあるっぽい -> なんかおかしい
		DEBUGERR("SOMETHING WRONG: head seems to be within previous chunk: %p", head);
		return;
	}
	remove_item(&zone->allocated, head);
	// curr_unused があるなら, それは head より大きい最小の(=右隣の)ブロックヘッダ.
	DEBUGOUT("curr_unused = %p", curr_unused);
	if (curr_unused) {
		if ((head + head->blocks + 1) == curr_unused) {
			// head と curr_unused がくっついている -> 統合する
			DEBUGOUT("head(%zu, %p) + curr_unused(%zu, %p)", head->blocks, head, curr_unused->blocks, curr_unused);
			head->blocks += curr_unused->blocks + 1;
			head->next = ADDRESS(curr_unused->next);
			curr_unused->blocks = 0;
			curr_unused->next = 0;
			DEBUGOUT("-> head(%zu, %p)", head->blocks, head);
		} else {
			head->next = curr_unused;
		}
	}
	// prev_unused があるなら, それは head より小さい最大の(=左隣の)ブロックヘッダ.
	DEBUGOUT("prev_unused = %p", prev_unused);
	if (prev_unused) {
		DEBUGOUT("head = %p", head);
		if (prev_unused + (prev_unused->blocks + 1) == head) {
			// prev_unused と head がくっついている -> 統合する
			DEBUGOUT("prev_unused(%zu, %p) + head(%zu, %p)", prev_unused->blocks, prev_unused, head->blocks, head);
			prev_unused->blocks += head->blocks + 1;
			prev_unused->next = ADDRESS(head->next);
			DEBUGOUT("-> prev_unused(%zu, %p)", prev_unused->blocks, prev_unused);
			head->blocks = 0;
			head->next = 0;
			head = prev_unused;
		} else {
			prev_unused->next = head;
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
		zone->frees = _yo_allocate_heap(zone->heap_blocks);
	}
	if (zone->frees == NULL) {
		// g_root.frees 確保失敗
		return NULL;
	}
	DEBUGOUT("g_root.frees head: (%zu, %p) -> %p", zone->frees->blocks, zone->frees, zone->frees->next);
	// 要求されるサイズ以上のチャンクが空いていないかどうか探す
	t_block_header	*head = zone->frees;
	t_block_header	*prev = NULL;
	DEBUGOUT("blocks_needed = %zu", blocks_needed);
	while (head != NULL) {
		if (head->blocks >= blocks_needed) {
			// 適合するチャンクがあった -> 後処理を行う
			// ブロックヘッダの次のブロックを返す
			t_block_header	*rv = head + 1;

			// 見つかったチャンクのうち, 最初の blocks_needed 個を除去する
			// head->blocks がちょうど blocks_needed と一致しているかどうかで場合分け
			if (head->blocks <= blocks_needed + 1) {
				// -> 見つかったチャンクを丸ごと使い尽くす
				DEBUGOUT("exhausted chunk: (%zu, %p)", head->blocks, head);
				head = ADDRESS(head->next);
			} else {
				// -> 見つかったチャンクの一部が残る
				t_block_header	*new_head = head + blocks_needed + 1;

				// head->blocks + 1 = (blocks_needed + 1) + (rest_blocks + 1)
				// -> rest_blocks = head->blocks - (blocks_needed + 1)
				new_head->blocks = head->blocks - (blocks_needed + 1);
				new_head->next = ADDRESS(head->next);
				DEBUGOUT("shorten block: (%zu, %p) -> (%zu, %p)", head->blocks, head, new_head->blocks, new_head);
				head->blocks = blocks_needed;
				head = new_head;
			}
			if (prev) {
				prev->next = head;
			} else {
				zone->frees = head;
			}
			if (zone_class == YO_ZONE_TINY) {
				head->next = SET_IS_TINY(head->next);
			} else {
				head->next = SET_IS_SMALL(head->next);
			}
			DEBUGOUT("head->next = %p", head->next);
			DEBUGOUT("returning block: (%zu, %p)", blocks_needed, rv);
			insert_item(&zone->allocated, (rv - 1));
			DEBUGSTR("** malloc end **");
			return rv;
		}
		prev = head;
		head = ADDRESS(head->next);
	}
	// 適合するチャンクがなかった
	DEBUGSTR("NO ENOUGH BLOCKS -> extend current zone");
	t_block_header	*new_heap = _yo_allocate_heap(zone->heap_blocks);
	if (new_heap == NULL) {
		errno = ENOMEM;
		return NULL;
	}
	DEBUGOUT("new_heap = %p", new_heap);
	yo_free_actual(new_heap + 1);
	return NULL;
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
	DEBUGSTR("  allocated: "); show_list(zone->allocated);
	DEBUGSTR("  free:      "); show_list(zone->frees);
}

void show_alloc_mem() {
	DEBUGSTR("TINY:");
	show_zone(&g_root.tiny);
	DEBUGSTR("SMALL:");
	show_zone(&g_root.small);
	DEBUGSTR("LARGE:  ");
	DEBUGSTR("  used: ");
	show_list(g_root.large);
}
