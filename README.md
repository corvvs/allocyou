## mallocてきとう再実装

### 要件

- 実装するのは `malloc`, `free`, `realloc`
- チャンクを要求サイズによって TINY, SMALL, LARGE の3つのゾーンに分けて管理する
- メモリの確保・解放は `mmap`, `munmap` で行う
  - システムコール `brk`(`sbrk`)は使わない
- LARGE ゾーンのチャンクは直接 `mmap`, `munmap` する

### マクロスイッチ

特記ない場合は, 定義した時の効果を記載:

- `USE_LIBC`
	- テストコードにおいて `malloc`, `free`, `realloc` がlibcのものになる
- `NDEBUG`
	- コード内の`assert`を無効化する
