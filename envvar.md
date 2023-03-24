## 環境変数

以下の環境変数が定義されている状態をプログラムを実行すると、所定の効果を発揮する。

### `MALLOC_PERTURB_` **チャンク埋めバイト**

定義されている場合, 1文字目がチャンク埋めに使うバイトになる.

```
root@e844760c33e0:/home# cat l.c
#include <stdlib.h>
#include <unistd.h>

int main() {
        char* mem = malloc(10);
        mem[9] = '\n';
        write(1, mem, 10);
        free(mem);
        write(1, mem, 10);
}
root@e844760c33e0:/home# gcc l.c 
root@e844760c33e0:/home# LD_PRELOAD=./libft_malloc.so ./a.out | cat -e # 指定なし
^@^@^@^@^@^@^@^@^@$
^@^@^@^@^@^@^@^@^@$
root@e844760c33e0:/home# LD_PRELOAD=./libft_malloc.so MALLOC_PERTURB_=a ./a.out | cat -e # "a"を指定
aaaaaaaaa$
M-^^M-^^M-^^M-^^M-^^M-^^M-^^M-^^M-^^M-^^root@e844760c33e0:/home# LD_PRELOAD=./libft_malloc.so MALLOC_PERTURB_=x ./a.out | cat -e # "x"を指定
xxxxxxxxx$
M-^GM-^GM-^GM-^GM-^GM-^GM-^GM-^GM-^GM-^Groot@e844760c33e0:/home# 
```

### `MALLOC_SINGLE_THREAD_` **シングルスレッドモード**

定義されている場合, アロケータはシングルスレッドモードで動作する.\
すなわち一切のロック取得操作を省略する.


### `MALLOC_HISTORY_` **操作履歴マスタースイッチ**

定義されている場合, 操作履歴の取得を行う.\
対象となる操作:

- `malloc`
- `free`
- `realloc`
- `calloc`
- `memalign`

```
root@e844760c33e0:/home# cat l.c 
#include <stdlib.h>
#include <unistd.h>

int main() {
        char* mem = malloc(10);
        mem[9] = '\n';
        write(1, mem, 10);
        free(mem);
        write(1, mem, 10);
}
root@e844760c33e0:/home# gcc l.c 
root@e844760c33e0:/home# LD_PRELOAD=./libft_malloc.so MALLOC_HISTORY_= ./a.out | cat -e # プログラム終了時に履歴を表示している
^@^@^@^@^@^@^@^@^@$
^@^@^@^@^@^@^@^@^@$
<< operation history (only latest 4 items) >>$
[#0] malloc(10) -> 0xffffaeb03f90$
[#1] malloc(2048) -> 0xffffae01f910$
[#2] free(0x0)$
[#3] free(0xffffaeb03f90)$
[arena #0 (multi-thread mode)]$
        < TINY >$
                zone 0xffffaeb00000: 65536 blocks ALL FREE$
        1 zone in this subarena$
        < SMALL >$
                zone 0xffffae000000: using 129 blocks / 524288 blocks$
                chunk @ 0xffffae01f900: 129 blocks (2064 B)$
                xd: 01 00 00 00 00 00 00 00  90 3f b0 ae ff ff 00 00  |.........?......|$
        1 zone in this subarena$
root@e844760c33e0:/home# 
```

### `MALLOC_HISTORY_LIMIT_` **操作履歴表示数**

(`MALLOC_HISTORY_`が定義されていない場合は効果なし)

値が"none"の場合, 全履歴を表示する.\
そうでない場合は直近64アイテムを表示する.


### `MALLOC_HISTORY_ONDISK_` **操作履歴出力先指定**

(`MALLOC_HISTORY_`が定義されていない場合は効果なし)

定義されている場合, 操作履歴を`/tmp`以下に作成される一時ファイルに書き出す.

ただし空でない値がある場合は, 値をファイルパスとみなし, そのファイルへの書き出しを試みる.\
(この場合, ファイルは一時ファイルではない)

### `MALLOC_XD_BLOCKS_` **ダンプサイズ**

定義されている場合, 値を10進符号なし整数として解釈し,\
その結果を「`show_alloc_mem_ex` において使用済みチャンクのダンプを行う際の最大ブロック数」とする.

デフォルトの値は1.\
環境変数値が解釈できない場合は0.\
最大値は`YOYO_MAX_XD_BLOCKS`(256).

### `MALLOC_DEBUG_ONDISK_` **デバッグ出力先指定**

(stub)

定義されている場合, 「デバッグ出力」を`/tmp`以下に作成される一時ファイルに書き出す.

ただし空でない値がある場合は, 値をファイルパスとみなし, そのファイルへの書き出しを試みる.\
(この場合, ファイルは一時ファイルではない)

なお「デバッグ出力」とは

- `show_alloc_mem`の出力
- `show_alloc_mem_ex`の出力
- `release_memory`の出力
- コンパイルオプションに`-D DEBUG`を指定した場合のログ出力

を指す.\
デフォルトではいずれも`STDERR_FILENO`に出力する.




