#include "yo_internal.h"

void	insert_item(t_block_header **list, t_block_header *item) {
	if (list == NULL) {
		DEBUGSTR("SOMETHING WRONG: list is null");
		return;
	}
	if (*list == NULL) {
		*list = item;
		item->next = NULL;
		DEBUGSTR("item is front");
		return;
	}
	t_block_header	*curr = *list;
	t_block_header	*prev = NULL;
	while (curr != NULL && curr <= item) {
		prev = curr;
		curr = ADDRESS(curr->next);
	}
	if (curr != NULL) {
		item->next = COPYFLAGS(curr, item->next);
	}
	if (prev != NULL) {
		prev->next = item;
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
		dprintf(STDERR_FILENO, "%s[nothing]%s\n", TX_GRN, TX_RST);
		return;
	}
	while (list != NULL) {
		dprintf(STDERR_FILENO, "%s[%012lx:%zu]%s", TX_GRN, (uintptr_t)list % (BLOCK_UNIT_SIZE << 24), list->blocks, TX_RST);
		list = ADDRESS(list->next);
	}
	dprintf(STDERR_FILENO, "\n");
}

