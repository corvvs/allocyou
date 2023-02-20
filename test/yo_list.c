#include "yo_internal.h"

void	insert_item(t_block_header **list, t_block_header *item) {
	if (list == NULL) {
		DEBUGSTR("SOMETHING WRONG: list is null");
		return;
	}
	if (*list == NULL) {
		*list = item;
		item->next = COPYFLAGS(NULL, item->next);
		DEBUGSTR("item is front");
		return;
	}
	t_block_header	*curr = *list;
	t_block_header	*prev = NULL;
	while (curr != NULL && curr <= item) {
		prev = curr;
		curr = ADDRESS(curr->next);
	}
	// 挿入場所に挿入
	if (curr != NULL) {
		item->next = COPYFLAGS(curr, item->next);
	}
	if (prev != NULL) {
		prev->next = COPYFLAGS(item, item->next);
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
		curr = ADDRESS(curr->next);
	}
	if (curr != NULL) {
		if (prev == NULL) {
			*list = ADDRESS(curr->next);
		} else {
			prev->next = item->next;
		}
		curr->next = NULL;
	}
}

// 昇順リスト list の要素のうち, item より小さい最大の要素を探す.
// returns max{ m in list | m < item }
t_block_header*	find_item(t_block_header* list, t_block_header *item) {
	t_block_header *prev = NULL;
	t_block_header *curr = list;
	while (curr != NULL && curr < item) {
		prev = curr;
		curr = ADDRESS(curr->next);
	}
	return prev;
}

void	show_list(t_block_header *list) {
	if (list == NULL) {
		dprintf(STDERR_FILENO, "%s[nothing]%s\n", TX_YLW, TX_RST);
		return;
	}
	dprintf(STDERR_FILENO, TX_GRN);
	while (list != NULL) {
		const int is_adjacent = ADDRESS(list->next) == NULL || (uintptr_t)ADDRESS(list->next) == (uintptr_t)(list + list->blocks + 1);
		dprintf(STDERR_FILENO, "[%0lx:%zu]", (uintptr_t)list, list->blocks);
		if (!is_adjacent) {
			dprintf(STDERR_FILENO, "-");
		}
		list = ADDRESS(list->next);
	}
	dprintf(STDERR_FILENO, TX_RST "\n");
}

