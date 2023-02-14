#include "yo_internal.h"

// addr が malloc された領域ならば解放する.
void	yo_free(void *addr) {
	if (addr == NULL) {
		DEBUGSTR("freeing NULL\n");
		return;
	}

	DEBUGOUT("** %p **\n", addr);
	t_block_header *head = addr;
	--head;
	DEBUGOUT("head: %p, next: %p\n", head, head->next);

	if (GET_IS_LARGE(head->next)) {
		// ラージセクション -> munmapする
		_yo_large_free(head);
		return;
	}

	// addr が malloc された領域なら, addr から sizeof(t_block_header) だけ下がったところにブロックヘッダがあるはず.
	t_block_header *prev_unused = NULL;
	t_block_header *curr_unused = g_root.frees;
	while (curr_unused != NULL && curr_unused < head) {
		prev_unused = curr_unused;
		curr_unused = ADDRESS(curr_unused->next);
	}
	// assertion:
	// (!prev_unused || prev_unused < addr) && addr <= curr_unused
	if (head == curr_unused) {
		// head が curr_unused と一致している -> なんかおかしい
		DEBUGWARN("SOMETHING WRONG: head is equal to free-block-header: %p\n", head);
		return;
	}
	if (prev_unused != NULL && head <= (prev_unused + prev_unused->blocks)) {
		// head が前のブロックセクションの中にあるっぽい -> なんかおかしい
		DEBUGWARN("SOMETHING WRONG: head seems to be within previous block-section: %p\n", head);
		return;
	}
	remove_item(&g_root.allocated, head);
	// curr_unused があるなら, それは head より大きい最小の(=右隣の)ブロックヘッダ.
	DEBUGOUT("curr_unused = %p\n", curr_unused);
	if (curr_unused) {
		if ((head + head->blocks + 1) == curr_unused) {
			// head と curr_unused がくっついている -> 統合する
			DEBUGOUT("head(%zu, %p) + curr_unused(%zu, %p)\n", head->blocks, head, curr_unused->blocks, curr_unused);
			head->blocks += curr_unused->blocks + 1;
			head->next = ADDRESS(curr_unused->next);
			curr_unused->blocks = 0;
			curr_unused->next = 0;
			DEBUGOUT("-> head(%zu, %p)\n", head->blocks, head);
		} else {
			head->next = curr_unused;
		}
	}
	// prev_unused があるなら, それは head より小さい最大の(=左隣の)ブロックヘッダ.
	DEBUGOUT("prev_unused = %p\n", prev_unused);
	if (prev_unused) {
		DEBUGOUT("head = %p\n", head);
		if (prev_unused + (prev_unused->blocks + 1) == head) {
			// prev_unused と head がくっついている -> 統合する
			DEBUGOUT("prev_unused(%zu, %p) + head(%zu, %p)\n", prev_unused->blocks, prev_unused, head->blocks, head);
			prev_unused->blocks += head->blocks + 1;
			prev_unused->next = ADDRESS(head->next);
			DEBUGOUT("-> prev_unused(%zu, %p)\n", prev_unused->blocks, prev_unused);
			head->blocks = 0;
			head->next = 0;
			head = prev_unused;
		} else {
			prev_unused->next = head;
		}
	} else {
		// prev_unused がない -> head が最小のブロックヘッダ
		g_root.frees = head;
	}
	DEBUGSTR("** free end **\n");
}


// n バイト**以上**の領域を確保して返す.
void	*yo_malloc(size_t n) {

	size_t mmap_unit = QUANTIZE(getpagesize(), BLOCK_UNIT_SIZE);

	size_t	blocks_needed = QUANTIZE(n, BLOCK_UNIT_SIZE) / BLOCK_UNIT_SIZE;
	DEBUGOUT("** malloc: B:%zu, blocks: %zu **, mmap_unit: %zu\n", n, blocks_needed, mmap_unit);

	if (blocks_needed >= mmap_unit) {
		// 要求サイズが1つのバンチに収まらない
		// -> 専用のバンチを mmap して返す
		DEBUGWARN("required size %zu(B) is greater than bunch size %zu(B)\n", n, mmap_unit * BLOCK_UNIT_SIZE);
		return _yo_large_malloc(n);
	}
	
	if (g_root.frees == NULL) {
		DEBUGSTR("allocating arena...\n");
		g_root.frees = _yo_allocate_bunch(mmap_unit);
	}
	if (g_root.frees == NULL) {
		// g_root.frees 確保失敗
		return NULL;
	}
	DEBUGOUT("g_root.frees head: (%zu, %p) -> %p\n", g_root.frees->blocks, g_root.frees, g_root.frees->next);
	// 要求されるサイズ以上のブロックセクションが空いていないかどうか探す
	t_block_header	*head = g_root.frees;
	t_block_header	*prev = NULL;
	DEBUGOUT("blocks_needed = %zu\n", blocks_needed);
	while (head != NULL) {
		if (head->blocks >= blocks_needed) {
			// 適合するブロックがあった -> 後処理を行う
			// ブロックヘッダの次のブロックを返す
			t_block_header	*rv = head + 1;

			// 見つかったブロックセクションのうち, 最初の blocks_needed 個を除去する
			// head->blocks がちょうど blocks_needed と一致しているかどうかで場合分け
			if (head->blocks <= blocks_needed + 1) {
				// -> 見つかったブロックセクションを丸ごと使い尽くす
				DEBUGOUT("exhausted block-section: (%zu, %p)\n", head->blocks, head);
				head = ADDRESS(head->next);
			} else {
				// -> 見つかったブロックセクションの一部が残る
				t_block_header	*new_head = head + blocks_needed + 1;

				// head->blocks + 1 = (blocks_needed + 1) + (rest_blocks + 1)
				// -> rest_blocks = head->blocks - (blocks_needed + 1)
				new_head->blocks = head->blocks - (blocks_needed + 1);
				new_head->next = ADDRESS(head->next);
				DEBUGOUT("shorten block: (%zu, %p) -> (%zu, %p)\n", head->blocks, head, new_head->blocks, new_head);
				head->blocks = blocks_needed;
				head = new_head;
			}
			if (prev) {
				prev->next = head;
			} else {
				g_root.frees = head;
			}
			DEBUGOUT("returning block: (%zu, %p)\n", blocks_needed, rv);
			insert_item(&g_root.allocated, (rv - 1));
			DEBUGSTR("** malloc end **\n");
			return rv;
		}
		prev = head;
		head = ADDRESS(head->next);
	}
	// 適合するブロックセクションがなかった
	DEBUGSTR("NO ENOUGH BLOCKS\n");
	t_block_header	*new_bunch = _yo_allocate_bunch(mmap_unit);
	if (new_bunch == NULL) {
		return NULL;
	}
	DEBUGOUT("new_bunch = %p\n", new_bunch);
	yo_free(new_bunch + 1);
	return NULL;
}

void show_alloc_mem() {
	DEBUGSTR("allocated: "); show_list(g_root.allocated);
	DEBUGSTR("free:      "); show_list(g_root.frees);
	DEBUGSTR("large:     "); show_list(g_root.large);
}
