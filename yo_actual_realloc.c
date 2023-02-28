#include "yo_internal.h"

static bool	is_relocation_needed(void *addr, size_t n) {
	const size_t	blocks_needed = blocks_for_size(n);
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
	t_yo_zone*	zone = yo_retrieve_zone(zone_class_current);
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

static void*	yo_relocate(void* addr, size_t n) {
	DEBUGSTR("REALOCATE");
	void*	relocated = yo_actual_malloc(n);
	if (relocated == NULL) {
		return (NULL);
	}
	t_block_header*	head_to = relocated;
	--head_to;
	DEBUGOUT("head_to: (%zu, %p, %p)", head_to->blocks, head_to, head_to->next);
	t_block_header*	head_from = addr;
	--head_from;
	const size_t	bytes_current = YO_MIN(head_from->blocks, head_to->blocks) * BLOCK_UNIT_SIZE;
	yo_memcpy(relocated, addr, bytes_current);
	yo_actual_free(addr);
	return relocated;
}

static void	yo_extend_chunk(t_yo_zone* zone, t_block_header* head, size_t n) {
	const size_t	blocks_needed = blocks_for_size(n);
	const size_t	blocks_current = head->blocks;
	assert(yo_zone_for_bytes(n) == yo_zone_for_addr(head + 1));
	assert(blocks_needed > blocks_current);
	t_block_header*	adjacent = head + head->blocks + 1;
	t_listcursor	cursor = find_fit_cursor(zone->frees, adjacent);
	assert(cursor.curr == adjacent);
	const size_t	blocks_capable = blocks_current + adjacent->blocks + 1;
	assert(blocks_capable >= blocks_needed);
	if (blocks_capable - blocks_needed > 1) {
		DEBUGSTR("SPLIT");
		t_block_header*	new_free = head + blocks_needed + 1;
		*new_free = (t_block_header) {
			.blocks = blocks_capable - (blocks_needed + 1),
			.next = cursor.curr->next,
		};
		head->blocks = blocks_needed;
		if (cursor.prev != NULL) {
			concat_item(cursor.prev, new_free);
		} else {
			zone->frees = new_free;
		}
		nullify_chunk(cursor.curr);
	} else {
		DEBUGSTR("EXHAUST");
		remove_item(&cursor.prev, cursor.curr);
		assimilate_chunk(head, cursor.curr);
	}
	const size_t	block_extended = blocks_needed - blocks_current;
	zone->cons.used_blocks += block_extended;
	zone->cons.free_blocks -= block_extended;
}

static void	yo_shrink_chunk(t_block_header* head, size_t n) {
	assert(!GET_IS_LARGE(head->next));

	size_t	blocks_needed = blocks_for_size(n);
	assert(head->blocks >= blocks_needed);
	const size_t	blocks_shrinked = head->blocks - blocks_needed;
	if (head->blocks < blocks_needed + 2) {
		DEBUGOUT("** MAINTAIN(%zu, %p, %p) **", head->blocks, head, head->next);
		return;
	}
	DEBUGOUT("SHRINK chunk %zu blocks -> %zu blocks", head->blocks, blocks_needed);
	t_block_header	*new_free = head + blocks_needed + 1;
	*new_free = (t_block_header) {
		.blocks	= head->blocks - (blocks_needed + 1),
		.next	= COPYFLAGS(NULL, head->next),
	};
	DEBUGOUT("new_free: (%zu, %p, %p)", new_free->blocks, new_free, new_free->next);
	head->blocks = blocks_needed;
	yo_actual_free(new_free + 1);
	t_yo_zone*	zone = yo_retrieve_zone(yo_zone_for_addr(head + 1));
	assert(zone != NULL);
	zone->cons.free_blocks += blocks_shrinked;
	zone->cons.used_blocks -= blocks_shrinked;
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
	const size_t	blocks_needed = blocks_for_size(n);
	const size_t	blocks_current = head->blocks;
	assert(blocks_current > 0);
	if (blocks_current - 1 <= blocks_needed && blocks_needed <= blocks_current) {
		DEBUGSTR("DO NOTHING");
	} else if (blocks_current < blocks_needed) {
		DEBUGSTR("EXTEND");
		t_yo_zone_class	zone_class = yo_zone_for_addr(addr);
		t_yo_zone*	zone = yo_retrieve_zone(zone_class);
		assert(zone != NULL);
		yo_extend_chunk(zone, head, n);
	} else {
		DEBUGSTR("SHRINK");
		yo_shrink_chunk(head, n);
	}
	return addr;
}
