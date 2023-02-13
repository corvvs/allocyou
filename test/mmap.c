#include "common.h"


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
  struct s_block_header   *ptr; // 次のブロックヘッダへのポインタ
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
	header->ptr = header;
	return mapped;
}

// 1度にmmapする単位.
// BLOCK_UNIT_SIZE をかけるとバイトサイズになる.
#define MMAP_UNIT ((size_t)1048576)

// n バイト**以上**の領域を確保して返す.
void	*_yk_malloc(size_t n) {
	static	t_block_header	*arena;

	if (arena == NULL) {
		dprintf(STDERR_FILENO, "allocating arena...\n");
		arena = _yk_allocate_arena(MMAP_UNIT);
	}
	if (arena == NULL) {
		// arena 確保失敗
		return NULL;
	}
	dprintf(STDERR_FILENO, "arena head: (%zu, %p) -> %p\n", arena->blocks, arena, arena->ptr);
	// 要求されるサイズ以上のブロックセクションが空いていないかどうか探す
	t_block_header	*head = arena;
	t_block_header	*prev = NULL;
	size_t	blocks_needed = QUANTIZE(n, BLOCK_UNIT_SIZE) / BLOCK_UNIT_SIZE;
	while (head) {
		if (head->blocks >= blocks_needed) {
			// 適合するブロックがあった -> 後処理を行う
			// ブロックヘッダの次のブロックを返す
			void	*rv = head + 1;
			// 見つかったブロックセクションのうち, 最初の blocks_needed 個を除去する
			// head->blocks がちょうど blocks_needed と一致しているかどうかで場合分け
			if (head->blocks == blocks_needed) {
				// -> 見つかったブロックセクションを丸ごと使い尽くす
				dprintf(STDERR_FILENO, "exhausted block-section: (%zu, %p)\n", head->blocks, head);
			} else {
				// -> 見つかったブロックセクションの一部が残る
				t_block_header	*new_head = head + blocks_needed + 1;
				// head->blocks + 1 = (blocks_needed + 1) + (rest_blocks + 1)
				// -> rest_blocks = head->blocks - (blocks_needed + 1)
				new_head->blocks = head->blocks - (blocks_needed + 1);
				new_head->ptr = head->ptr;
				head = new_head;
				dprintf(STDERR_FILENO, "shorten block: (%zu, %p) -> (%zu, %p)\n", head->blocks, head, new_head->blocks, new_head);
			}
			if (prev) {
				prev->ptr = head;
			} else {
				arena = head;
			}
			dprintf(STDERR_FILENO, "returning block: (%zu, %p)\n", blocks_needed, rv);
			return rv;
		}
		prev = head;
		head = head->ptr;
		if (head == arena) {
			// 適合するブロックセクションがなかった
			break;
		}
	}
	return NULL;
}

int main() {
	setvbuf(stdout, NULL, _IONBF, 0);
	int page_size = getpagesize();
	OUT_VAR_INT(page_size);
	OUT_VAR_SIZE_T(BLOCK_UNIT_SIZE);
	OUT_VAR_SIZE_T(MMAP_UNIT);

	{
		char *str = _yk_malloc(11);
		strcpy(str, "helloworld");
		printf("%s\n", str);
	}
	{
		char *str = _yk_malloc(16);
		strcpy(str, "helloworld");
		printf("%s\n", str);
	}

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
