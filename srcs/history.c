#include "internal.h"

bool	init_history(bool multi_thread) {
	t_yoyo_history_book*	history = &g_yoyo_realm.history;
	t_yoyo_debug*			debug = &g_yoyo_realm.debug;
	history->preserve = false;
	DEBUGINFO("TAKE_HISTORY: %d", debug->take_history);
	if (!debug->take_history) {
		return true;
	}
	if (multi_thread) {
		if (pthread_mutex_init(&history->lock, NULL)) {
			DEBUGFATAL("failed to init history lock: errno: %d (%s)", errno, strerror(errno));
			return false;
		}
	}
	history->items = NULL;
	history->n_items = 0;
	history->cap_items = 0;
	history->preserve = true;
	DEBUGINFO("%s", "ok.");
	return true;
}

// history->items を必要なら拡張する
// **この関数は, history がロックされた状態で呼び出され, history がロックされた状態で終了する.**
static bool	extend_items(t_yoyo_history_book* history) {
	// [拡張の要否を判定する]
	if (history->in_extend) {
		// すでに拡張中
		return true;
	}
	if (history->n_items + 1 <= history->cap_items) {
		// 拡張しなくて良い
		return true;
	}
	// [拡張が必要]
	// -> 拡張中フラグを立てる
	history->in_extend = true;
	const size_t	new_cap = history->cap_items > 0 ? history->cap_items * 2 : 64;
	DEBUGOUT("EXTEND items: %zu -> %zu", history->cap_items, new_cap);
	// デッドロック防止のため一旦ロックを外す
	pthread_mutex_unlock(&history->lock);
	t_yoyo_history_item*	extended = malloc(sizeof(t_yoyo_history_item) * new_cap);
	if (!extended) {
		pthread_mutex_lock(&history->lock);
		return false;
	}
	// ロックを取り直す
	pthread_mutex_lock(&history->lock);
	// コピー
	void*	old_items = history->items;
	DEBUGOUT("COPYING items: %p -> %p", old_items, extended);
	yo_memcpy(extended, old_items, sizeof(t_yoyo_history_item) * history->n_items);
	history->items = extended;
	history->cap_items = new_cap;

	// ロックを離してから(デッドロック防止)古い items を解放
	DEBUGOUT("DESTROY old items: %p", old_items);
	pthread_mutex_unlock(&history->lock);
	free(old_items);
	pthread_mutex_lock(&history->lock);
	// [拡張終了]
	// -> 拡張中フラグを外す
	history->in_extend = false;
	return true;
}

static	t_yoyo_history_item make_item(t_yoyo_operation_type operation, void* addr, size_t size1, size_t size2) {
	return (t_yoyo_history_item){
		.operation = operation, .addr = (uintptr_t)addr, .size1 = size1, .size2 = size2,
	};
}

void	take_history(t_yoyo_operation_type operation, void* addr, size_t size1, size_t size2) {
	t_yoyo_history_book*	history = &g_yoyo_realm.history;
	// [履歴取得フラグが降りているなら何もしない]
	if (!history->preserve) {
		return;
	}
	// [ロック取得]
	if (pthread_mutex_lock(&history->lock)) {
		DEBUGFATAL("FAILED to take history-lock: %p", &history->lock);
		return;
	}
	// [必要なら items を延長]
	if (!extend_items(history)) {
		pthread_mutex_unlock(&history->lock);
		return;
	}
	// [履歴を追加]
	if (history->in_extend) {
		// 拡張中の場合は temp_buff に追加
		if (history->n_temp < YOYO_HISTORY_TEMP_SIZE) {
			history->temp_buf[history->n_temp] = make_item(operation, addr, size1, size2);
			history->n_temp += 1;
			DEBUGOUT("added item into temp_buf: (%d, %p, %zu, %zu)", operation, addr, size1, size2);
		}
	} else {
		// そうでない場合は items に直接追加
		history->items[history->n_items] = make_item(operation, addr, size1, size2);
		history->n_items += 1;
		DEBUGOUT("added item into items: (%d, %p, %zu, %zu)", operation, addr, size1, size2);
		// temp_buff が空でない場合はそれも追加
		for (size_t i = 0; i < history->n_temp; ++i) {
			history->items[history->n_items] = history->temp_buf[i];
			history->n_items += 1;
		}
		history->n_temp = 0;
	}

	// 終わったのでロック解除
	pthread_mutex_unlock(&history->lock);
}

static void	print_history_item(size_t index, const t_yoyo_history_item* item) {
	yoyo_dprintf(STDOUT_FILENO, "[#%zu] ", index);
	switch (item->operation) {
		case YOYO_OP_MALLOC:
			yoyo_dprintf(STDOUT_FILENO,
				"malloc(%zu) -> %p\n",
				item->size1, (void*)item->addr);
			break;
		case YOYO_OP_FREE:
			yoyo_dprintf(STDOUT_FILENO,
				"free(%p)\n",
				(void*)item->addr);
			break;
		case YOYO_OP_REALLOC:
			yoyo_dprintf(STDOUT_FILENO,
				"realloc(%p, %zu) -> %p\n",
				(void*)item->size2, item->size1, (void*)item->addr);
			break;
		case YOYO_OP_CALLOC:
			yoyo_dprintf(STDOUT_FILENO,
				"calloc(%zu, %zu) -> %p\n",
				item->size1, item->size2, (void*)item->addr);
			break;
		case YOYO_OP_MEMALIGN:
			yoyo_dprintf(STDOUT_FILENO,
				"memalign(%zu, %zu) -> %p\n",
				item->size1, item->size2, (void*)item->addr);
			break;
		default:
			yoyo_dprintf(STDOUT_FILENO,
				"UNKNOWN OP: %d (%p, %zu, %zu)\n",
				item->operation, item->addr, item->size1, item->size2);
			break;
	}
}

// 操作履歴を表示する.
void	show_history(void) {
	t_yoyo_debug*			debug = &g_yoyo_realm.debug;
	t_yoyo_history_book*	history = &g_yoyo_realm.history;
	if (!history->preserve) {
		return; 
	}
	if (pthread_mutex_lock(&history->lock)) {
		DEBUGFATAL("FAILED to lock history: %p", &history->lock);
		return;
	}
	const size_t n_items = history->n_items;
	if (n_items == 0) {
		yoyo_dprintf(STDOUT_FILENO, "<< NO operation history >>\n");
	} else {
		// 開始インデックスを決める
		size_t i0;
		if (debug->history_unlimited) {
			i0 = 0;
			yoyo_dprintf(STDOUT_FILENO, "<< operation history >>\n");
		} else {
			i0 = (n_items > 64 ? n_items : 64) - 64;
			yoyo_dprintf(STDOUT_FILENO, "<< operation history (only latest %zu items) >>\n", n_items - i0);
		}
		// 開始インデックス以降の履歴アイテムを表示
		for (size_t i = i0; i < n_items; ++i) {
			print_history_item(i, &history->items[i]);
		}
	}
	pthread_mutex_unlock(&history->lock);
	return;
}
