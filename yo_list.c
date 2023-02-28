#include "yo_internal.h"

t_listcursor	init_cursor(t_block_header *list) {
	return (t_listcursor) {
		.curr			= list,
		.prev			= NULL,
		.head			= NULL,
		.start			= list,
		.visited_once	= false,
	};
}

t_listcursor	init_cursor_from_mid(t_block_header *list, t_block_header *mid) {
	if (list_next_head(mid) != NULL) {
		return (t_listcursor) {
			.prev			= mid,
			.curr			= list_next_head(mid),
			.head			= list,
			.start			= list_next_head(mid),
			.visited_once	= false,
		};
	} else {
		return (t_listcursor) {
			.prev			= NULL,
			.curr			= list,
			.head			= list,
			.start			= list,
			.visited_once	= false,
		};
	}
}

t_block_header	*list_next_head(t_block_header *head) {
	return ADDRESS(head->next);
}

// head の後ろに item をつなげる
void	concat_item(t_block_header *head, t_block_header *item) {
	assert(head != NULL);
	head->next = COPYFLAGS(item, head->next);
}

void	increment_cursor(t_listcursor *cursor) {
	cursor->prev = cursor->curr;
	cursor->curr = list_next_head(cursor->curr);
	if (cursor->curr == NULL && cursor->head != NULL) {
		cursor->curr = cursor->head;
		cursor->prev = NULL;
	}
	if (cursor->curr == cursor->start) {
		if (cursor->visited_once) {
			cursor->curr = NULL;
		} else {
			cursor->visited_once = true;
		}
	}
}

// cursor.prev < item < cursor.curr となるような cursor を返す
t_listcursor	find_cross_cursor(t_block_header* list, t_block_header *item) {
	assert(item != NULL);
	t_listcursor	cursor = init_cursor(list);
	size_t	n = 0;
	while (cursor.curr != NULL && cursor.curr < item) {
		increment_cursor(&cursor);
		++n;
	}
	DEBUGOUT("n = %zu", n);
	assert(cursor.curr != item);
	return cursor;
}

// prev < item == curr となるような cursor を返す
t_listcursor	find_fit_cursor(t_block_header* list, t_block_header *item) {
	assert(item != NULL);
	t_listcursor	cursor = init_cursor(list);
	size_t	n = 0;
	while (cursor.curr != NULL && cursor.curr < item) {
		increment_cursor(&cursor);
		++n;
	}
	DEBUGOUT("cursor.curr = %p, item = %p", cursor.curr, item);
	if (cursor.curr != item) {
		cursor.curr = NULL;
	}
	DEBUGOUT("n = %zu", n);
	// ↑ cursor.curr == NULL になるのは, 新しく確保したヒープを(free で) zone の末尾に追加するときのみ.
	return cursor;
}

// 昇順リスト list の要素のうち, item より小さい最大の要素を探す.
// returns max{ m in list | m < item }
t_block_header*	find_inf_item(t_block_header* list, t_block_header *item) {
	return find_cross_cursor(list, item).prev;
}


void	insert_item(t_block_header **list, t_block_header *item) {
	SPRINT_START;

	assert(list != NULL);
	if (*list == NULL) {
		*list = item;
		concat_item(item, NULL);
		DEBUGSTR("item is front");
		return;
	}
	t_listcursor	cursor = find_cross_cursor(*list, item);
	// concat: item and cursor.curr
	concat_item(item, cursor.curr);
	// concat: cursor.prev and item
	if (cursor.prev != NULL) {
		concat_item(cursor.prev, item);
	} else {
		*list = item;
	}
	SPRINT_END("insert_item");
}

// 削除が実施されたかどうかを bool で返す
bool	remove_item(t_block_header **list, t_block_header *item) {
	assert(list != NULL);
	t_listcursor	cursor = find_fit_cursor(*list, item);
	if (cursor.curr == NULL) {
		return false;
	}
	assert(cursor.curr == item);
	// concat: cursor.prev and cursor.curr->next
	if (cursor.prev == NULL) {
		*list = list_next_head(cursor.curr);
	} else {
		concat_item(cursor.prev, cursor.curr->next);
	}
	// isolate: cursor.curr
	concat_item(cursor.curr, NULL);
	return true;
}

void	nullify_chunk(t_block_header *chunk) {
	*chunk = (t_block_header){};
}

t_block_header*	unify_chunk(t_block_header *b1, t_block_header *b2) {
	b1->blocks += b2->blocks + 1;
	concat_item(b1, b2->next);
	nullify_chunk(b2);
	return b1;
}

// ASSERT: b2 should be separated from list
t_block_header*	assimilate_chunk(t_block_header *b1, t_block_header *b2) {
	b1->blocks += b2->blocks + 1;
	nullify_chunk(b2);
	return b1;
}

void	show_list(t_block_header *list) {
	if (list == NULL) {
		dprintf(STDERR_FILENO, "%s[nothing]%s\n", TX_YLW, TX_RST);
		return;
	}
	dprintf(STDERR_FILENO, TX_GRN);
	while (list != NULL) {
		const int is_adjacent = list_next_head(list) == NULL || (uintptr_t)list_next_head(list) == (uintptr_t)(list + list->blocks + 1);
		dprintf(STDERR_FILENO, "[%0lx:%zu]", (uintptr_t)list, list->blocks);
		if (!is_adjacent) {
			dprintf(STDERR_FILENO, "-");
		}
		list = list_next_head(list);
	}
	dprintf(STDERR_FILENO, TX_RST "\n");
}
