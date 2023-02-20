#include "yo_internal.h"

t_listcursor	init_cursor(t_block_header *list)
{
	return (t_listcursor){
		.curr = list,
		.prev = NULL,
	};
}

t_block_header	*list_next_head(t_block_header *head) {
	return ADDRESS(head->next);
}

// head の後ろに item をつなげる
void	concat_item(t_block_header *head, t_block_header *item) {
	assert(head != NULL);
	head->next = COPYFLAGS(item, head->next);
}

void	increment_cursor(t_listcursor *cursor)
{
	cursor->prev = cursor->curr;
	cursor->curr = list_next_head(cursor->curr);
}

// cursor.prev < item < cursor.curr となるような cursor を返す
t_listcursor	find_cross_cursor(t_block_header* list, t_block_header *item) {
	assert(item != NULL);
	t_listcursor	cursor = init_cursor(list);
	while (cursor.curr != NULL && cursor.curr < item) {
		increment_cursor(&cursor);
	}
	assert(cursor.curr != item);
	return cursor;
}

// prev < item == curr となるような cursor を返す
t_listcursor	find_fit_cursor(t_block_header* list, t_block_header *item) {
	assert(item != NULL);
	t_listcursor	cursor = init_cursor(list);
	while (cursor.curr != NULL && cursor.curr != item) {
		increment_cursor(&cursor);
	}
	return cursor;
}

void	insert_item(t_block_header **list, t_block_header *item) {
	if (list == NULL) {
		DEBUGSTR("SOMETHING WRONG: list is null");
		return;
	}
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
}

void	remove_item(t_block_header **list, t_block_header *item) {
	if (list == NULL) {
		DEBUGSTR("SOMETHING WRONG: list is null");
		return;
	}
	t_listcursor	cursor = find_fit_cursor(*list, item);
	if (cursor.curr == NULL) {
		return;
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
}

// 昇順リスト list の要素のうち, item より小さい最大の要素を探す.
// returns max{ m in list | m < item }
t_block_header*	find_inf_item(t_block_header* list, t_block_header *item) {
	return find_cross_cursor(list, item).prev;
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

