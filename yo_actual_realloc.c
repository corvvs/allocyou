#include "yo_internal.h"

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

static bool	is_relocation_needed(void *addr, size_t n) {
	const size_t	blocks_needed = BLOCKS_FOR_SIZE(n);
	const t_yo_zone_class	zone_class_needed = yo_zone_for_bytes(n);

	t_block_header	*head = addr;
	--head;
	const size_t	blocks_current = head->blocks;
	const t_yo_zone_class	zone_class_current = yo_zone_for_addr(addr);

	if (zone_class_current != zone_class_needed) {
		// ゾーンが異なる -> リロケーション要
		return true;
	}
	if (blocks_needed <= blocks_current) {
		// ブロック数が減る -> リロケーション不要
		return false;
	}
	// ブロック数を増やせる(エクステンション可能)かどうか確認
	t_yo_zone*	zone = yo_retrieve_zone_for_class(zone_class_current);
	assert(zone != NULL);
	t_block_header*	left_adjacent = head + head->blocks + 1;
	t_listcursor	cursor = find_fit_cursor(zone->frees, left_adjacent);
	if (cursor.curr == NULL) {
		// 右隣接するフリーブロックがない -> リロケーション要(エクステンション不可)
		return true;
	}
	const size_t	blocks_capable = blocks_current + cursor.curr->blocks + 1;
	// 現在のブロック数 + 追加可能なブロック数 < 必要なブロック数
	// ならばリロケーション要(エクステンション不可)
	return blocks_capable < blocks_needed;
}



// void* yo_actual_realloc(void *addr, size_t n)
//
// [task]
// - `addr`が NULL の場合は, malloc(n) に処理を移譲する.
// - `addr`が malloc されたチャンクである場合:
//   - まずチャンクのアドレスを変えないままチャンクサイズの変更を試みる.
//   - 変更ができない場合は, malloc(n) の後, 確保されたチャンクに元のチャンクのデータをすべてコピーし, 元のチャンクを解放する.
// - いずれでもない場合の動作は未定義.
//
// [mechanism]
// `addr`は NULL でないとする.
// 大まかな手順は:
// - チャンクを引っ越す(リロケートする)必要があるかどうか判定する.
// - リロケートする場合は, malloc, copy and free を行う.
// - リロケートしない場合は, チャンクのサイズ調整を行う.
//
// (リロケーションの要否の決定)
// `n`(与えられた要求バイトサイズ)から, 必要なメモリブロック数`blocks_needed`を算出する.
// さらに`blocks_needed`から対応するゾーン種別`zone_class_to`を決定する.
// また, `addr`からヘッダ1つ分前のアドレスがチャンクのヘッダ`head`であると仮定し,
// チャンクのブロック数`blocks_current`と対応するゾーン種別`zone_class_from`を決定する.
// zone_class_from != zone_class_to なら** その時点でリロケーションが必要 **.
// zone_class_from == zone_class_to である場合,
// 対象チャンクのブロック数を`blocks_needed`ブロックに変更できるかどうかを調べる.
// ** 変更できない場合はリロケーションが必要 **.
//
// (対象チャンクのブロック数を`blocks_needed`ブロックに変更できるかどうか)
// blocks_needed <= blocks_current なら無条件で可能.
// そうでない場合, 対象チャンクのすぐ右に未使用チャンクがあり, かつその未使用チャンクのブロック数が十分に大きければ変更可能.
// 具体的には,
// blocks_needed <= blocks_current + 1 + 未使用チャンクのブロック数
// ならば変更可能.
//
// (リロケーションする場合)
// malloc し, データコピーし, free して 新しいアドレスを返して終わり.
//
// (リロケーションしない場合)
// データコピーは不要.
// 以下の3パターンが考えられる:
// `blocks_needed`と`blocks_current`の大小に応じてやることが違う.
// - blocks_current - 1 <= blocks_needed <= blocks_current
//   - 何もしない.
//   - ブロック数は小さくなるのだが, 小さくしても新しいブロックを生成できないため.
// - blocks_current < blocks_needed
//   - 必要なブロック数が現在よりも増える.
//   - 対象チャンクのすぐ右に十分なブロックを持つ未使用チャンクがある(事前にあることを確認している).
//   - この未使用チャンクを解体し, 現在チャンクに合体させる.
// - blocks_needed < blocks_current - 1
//   - 必要なブロック数が現在よりも減る.
//   - 対象チャンクのブロック数を小さくし, 余ったブロックを新たな未使用チャンクとする.
//
void*	yo_actual_realloc(void *addr, size_t n) {
	if (addr == NULL) {
		DEBUGWARN("addr == NULL -> delegate to malloc(%zu)", n);
		return yo_actual_malloc(n);
	}

	if (is_relocation_needed(addr, n)) {
		return yo_relocate(addr, n);
	}

	t_block_header	*head = addr;
	--head;
	const size_t	blocks_needed = BLOCKS_FOR_SIZE(n);
	const size_t	blocks_current = head->blocks;
	assert(blocks_current > 0);
	if (blocks_current - 1 <= blocks_needed && blocks_needed <= blocks_current) {
		DEBUGSTR("DO NOTHING");
	} else if (blocks_current < blocks_needed) {
		DEBUGSTR("EXTEND");
		t_yo_zone_class	zone_class = yo_zone_for_addr(addr);
		t_yo_zone*	zone = yo_retrieve_zone_for_class(zone_class);
		assert(zone != NULL);
		yo_extend_chunk(zone, head, n);
	} else {
		DEBUGSTR("SHRINK");
		yo_shrink_chunk(head, n);
	}
	return addr;
}
