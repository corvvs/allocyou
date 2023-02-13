#include "common.h"


// quantize a by b
// least-greater-than multiple
// a 以上で最小の b の倍数を返す
// a > 0, b > 0 を仮定している
#define QUANTIZE(a, b) ((a - 1) / b + 1) * b

typedef struct s_block_header {
  struct s_block_header   *ptr; // 次のブロックへのポインタ
  size_t                   blocks; // このブロックの長さ in ブロックヘッダ
} t_block_header;

// 1ブロックのサイズ
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
#define MMAP_UNIT 5120

// n バイト**以上**の領域を確保して返す.
void	*_yk_malloc(size_t n) {
	static	t_block_header	*arena;

	if (arena == NULL) {
		arena = _yk_allocate_arena(MMAP_UNIT);
	}
	if (arena == NULL) {
		// arena 確保失敗
		return NULL;
	}
	// 要求されるサイズ
	t_block_header	*started = arena;
}

int main() {
	setvbuf(stdout, NULL, _IONBF, 0);
	int page_size = getpagesize();
	OUT_VAR_INT(page_size);

	OUT_VAR_SIZE_T(BLOCK_UNIT_SIZE);

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
