#include "common.h"


#define DEBUGSTR(format) dprintf(STDERR_FILENO, "[%s:%d %s] " format, __FILE__, __LINE__, __func__)
#define DEBUGOUT(format, ...) dprintf(STDERR_FILENO, "[%s:%d %s] " format, __FILE__, __LINE__, __func__, __VA_ARGS__)
#define DEBUGWARN(format, ...) dprintf(STDERR_FILENO, "[%s:%d %s] " format, __FILE__, __LINE__, __func__, __VA_ARGS__)

#define TX_GRN "\e[32m"
#define TX_BLU "\e[32m"
#define TX_RST "\e[0m"

// quantize a by b
// least-greater-than multiple
// a 以上で最小の b の倍数を返す
// b > 0 を仮定している
#define QUANTIZE(a, b) (a ? ((a - 1) / b + 1) * b : b)

/**
 * ブロックヘッダ
 * ブロックヘッダの先頭から (blocks + 1) * BLOCK_UNIT_SIZE バイト分の領域を
 * 「一続きのブロックのあつまり」という意味で ブロックセクション と呼ぶ.
 */
typedef struct s_block_header {
  struct s_block_header   *next; // 次のブロックヘッダへのポインタ
  size_t                   blocks; // このブロックセクションの長さ
} t_block_header;

// 1ブロックのサイズ
// 16Bであると思っておく
#define BLOCK_UNIT_SIZE (QUANTIZE(sizeof(t_block_header), sizeof(size_t)))

// n + 1 個分のブロックを mmap で確保して返す
// 失敗した場合は NULL が返る
void	*_yk_allocate_arena(size_t n) {
	void	*mapped = mmap(
		NULL,
		(n + 1) * BLOCK_UNIT_SIZE,
		PROT_READ | PROT_WRITE,
		MAP_ANON | MAP_PRIVATE,
		-1, 0);
	if (mapped == MAP_FAILED) {
		return NULL;
	}
	// 確保できたら先頭にブロックヘッダを詰める
	t_block_header	*header = mapped;
	header->blocks = n;
	header->next = NULL;
	return mapped;
}

// 1度にmmapする単位.
// BLOCK_UNIT_SIZE をかけるとバイトサイズになる.
#define MMAP_UNIT ((size_t)1048576)

t_block_header	*g_arena;
t_block_header	*g_allocated;

void	insert_item(t_block_header **list, t_block_header *item) {
	if (list == NULL) {
		DEBUGSTR("SOMETHING WRONG: list is null");
		return;
	}
	if (*list == NULL) {
		*list = item;
		item->next = NULL;
		DEBUGSTR("item is front");
		return;
	}
	t_block_header	*curr = *list;
	t_block_header	*prev = NULL;
	while (curr != NULL && curr <= item) {
		prev = curr;
		curr = curr->next;
	}
	if (curr != NULL) {
		item->next = curr;
	}
	if (prev != NULL) {
		prev->next = item;
	} else {
		*list = item;
	}
}

void	remove_item(t_block_header **list, t_block_header *item) {
	if (list == NULL) {
		DEBUGSTR("SOMETHING WRONG: list is null");
		return;
	}
	t_block_header	*curr = *list;
	t_block_header	*prev = NULL;
	while (curr != NULL && curr != item) {
		prev = curr;
		curr = curr->next;
	}
	if (curr != NULL) {
		if (prev == NULL) {
			*list = curr->next;
		} else {
			prev->next = item->next;
		}
		curr->next = NULL;
	}
}

void	show_list(t_block_header *list) {
	while (list != NULL) {
		dprintf(STDERR_FILENO, "%s[%04lx:%zu]%s", TX_GRN, (uintptr_t)list % (BLOCK_UNIT_SIZE * BLOCK_UNIT_SIZE * BLOCK_UNIT_SIZE), list->blocks, TX_RST);
		list = list->next;
	}
	dprintf(STDERR_FILENO, "\n");
}

// n バイト**以上**の領域を確保して返す.
void	*_yk_malloc(size_t n) {
	DEBUGOUT("** malloc: %zu **\n", n);
	if (g_arena == NULL) {
		DEBUGSTR("allocating arena...\n");
		g_arena = _yk_allocate_arena(MMAP_UNIT);
	}
	if (g_arena == NULL) {
		// g_arena 確保失敗
		return NULL;
	}
	DEBUGOUT("g_arena head: (%zu, %p) -> %p\n", g_arena->blocks, g_arena, g_arena->next);
	// 要求されるサイズ以上のブロックセクションが空いていないかどうか探す
	t_block_header	*head = g_arena;
	t_block_header	*prev = NULL;
	size_t	blocks_needed = QUANTIZE(n, BLOCK_UNIT_SIZE) / BLOCK_UNIT_SIZE;
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
				head = head->next;
			} else {
				// -> 見つかったブロックセクションの一部が残る
				t_block_header	*new_head = head + blocks_needed + 1;

				// head->blocks + 1 = (blocks_needed + 1) + (rest_blocks + 1)
				// -> rest_blocks = head->blocks - (blocks_needed + 1)
				new_head->blocks = head->blocks - (blocks_needed + 1);
				new_head->next = head->next;
				DEBUGOUT("shorten block: (%zu, %p) -> (%zu, %p)\n", head->blocks, head, new_head->blocks, new_head);
				head->blocks = blocks_needed;
				head = new_head;
			}
			if (prev) {
				prev->next = head;
			} else {
				g_arena = head;
			}
			DEBUGOUT("returning block: (%zu, %p)\n", blocks_needed, rv);
			insert_item(&g_allocated, (rv - 1));
			DEBUGSTR("** malloc end **\n");
			return rv;
		}
		prev = head;
		head = head->next;
	}
	// 適合するブロックセクションがなかった
	DEBUGSTR("NO ENOUGH BLOCKS");
	return NULL;
}


// addr が malloc された領域ならば解放する.
void	_yk_free(void *addr) {
	if (addr == NULL) {
		DEBUGSTR("freeing NULL\n");
		return;
	}
	DEBUGOUT("** free: %p **\n", addr);
	t_block_header *head = addr;
	--head;
	DEBUGOUT("head: %p\n", head);
	// addr が malloc された領域なら, addr から sizeof(t_block_header) だけ下がったところにブロックヘッダがあるはず.
	t_block_header *prev_unused = NULL;
	t_block_header *curr_unused = g_arena;
	while (curr_unused != NULL && curr_unused < head) {
		prev_unused = curr_unused;
		curr_unused = curr_unused->next;
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
	remove_item(&g_allocated, head);
	// curr_unused があるなら, それは head より大きい最小の(=右隣の)ブロックヘッダ.
	if (curr_unused) {
		if ((head + head->blocks + 1) == curr_unused) {
			// head と curr_unused がくっついている -> 統合する
			DEBUGOUT("head(%zu, %p) + curr_unused(%zu, %p)\n", head->blocks, head, curr_unused->blocks, curr_unused);
			head->blocks += curr_unused->blocks + 1;
			head->next = curr_unused->next;
			curr_unused->blocks = 0;
			curr_unused->next = 0;
			DEBUGOUT("-> head(%zu, %p)\n", head->blocks, head);
		} else {
			head->next = curr_unused;
		}
	}
	// prev_unused があるなら, それは head より小さい最大の(=左隣の)ブロックヘッダ.
	if (prev_unused) {
		if (prev_unused + (prev_unused->blocks + 1) == head) {
			// prev_unused と head がくっついている -> 統合する
			DEBUGOUT("prev_unused(%zu, %p) + head(%zu, %p)\n", prev_unused->blocks, prev_unused, head->blocks, head);
			prev_unused->blocks += head->blocks + 1;
			prev_unused->next = head->next;
			DEBUGOUT("-> prev_unused(%zu, %p)\n", prev_unused->blocks, prev_unused);
			head->blocks = 0;
			head->next = 0;
			head = prev_unused;
		} else {
			prev_unused->next = head;
		}
	} else {
		// prev_unused がない -> head が最小のブロックヘッダ
		g_arena = head;
	}
	DEBUGSTR("** free end **\n");
}

void print_state() {
	DEBUGSTR("allocated: "); show_list(g_allocated);
	DEBUGSTR("free:      "); show_list(g_arena);
}

#define PRINT_STATE_AFTER(proc) proc; DEBUGSTR("DA: " #proc "\n"); print_state();

int main() {
	setvbuf(stdout, NULL, _IONBF, 0);
	int page_size = getpagesize();
	OUT_VAR_INT(page_size);
	OUT_VAR_SIZE_T(BLOCK_UNIT_SIZE);
	OUT_VAR_SIZE_T(MMAP_UNIT);


	print_state();
	PRINT_STATE_AFTER(char *a = _yk_malloc(1));
	PRINT_STATE_AFTER(char *b = _yk_malloc(20));
	PRINT_STATE_AFTER(char *c = _yk_malloc(50));
	PRINT_STATE_AFTER(_yk_free(b));
	PRINT_STATE_AFTER(_yk_free(a));
	PRINT_STATE_AFTER(char *d = _yk_malloc(72));
	PRINT_STATE_AFTER(char *e = _yk_malloc(1));
	PRINT_STATE_AFTER(char *f = _yk_malloc(1));
	PRINT_STATE_AFTER(char *g = _yk_malloc(123456789));
	PRINT_STATE_AFTER(_yk_free(c));
	PRINT_STATE_AFTER(_yk_free(e));
	PRINT_STATE_AFTER(_yk_free(g));
	PRINT_STATE_AFTER(_yk_free(f));
	PRINT_STATE_AFTER(_yk_free(d));

	// void *mmap(void *addr, size_t len, int prot, int flags, int fd, off_t offset);
	// - addr: マッピング領域の開始点を決めるために使われるアドレス
	//   (addr からマッピングが始まるとは限らない)
	// - len: マッピング領域の**最大**サイズ


	// size_t len = 99;
	// void	*mapped = mmap(0, len, PROT_READ | PROT_WRITE, MAP_ANON | MAP_PRIVATE, -1, 0);
	// OUT_VAR_ADDR(mapped);
	// OUT_VAR_INT(errno);
	// OUT_VAR_STR(strerror(errno));
	// unsigned char *mem = mapped;
	// for (int i = 0; i < page_size; ++i) {
	// 	// write(STDOUT_FILENO, mem + i, 1);
	// 	mem[i] = 'X';
	// 	write(STDOUT_FILENO, mem + i, 1);
	// }
	// OUT_VAR_INT(munmap(mapped, len));
	// // munmap の len は mmap の len とは関係ないが, 揃えておくのが無難
	// OUT_VAR_INT(errno);
	// OUT_VAR_STR(strerror(errno));

	// f a b -> (a 以上で最小の b の倍数) 
	// ((a - 1) / b + 1) * b; a > 0, b > 1 ならこれでよいはず
}
