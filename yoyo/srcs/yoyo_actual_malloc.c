#include "yoyo_internal.h"

// ロックされた arena を1つ返す.
static t_yoyo_arena*	occupy_arena(t_yoyo_zone_class zone_class) {
	t_yoyo_arena* arena = NULL;
	for (unsigned int i = 0; i < g_yoyo_realm.arena_count; ++i) {
		arena = &g_yoyo_realm.arenas[i];
		if (try_lock_arena(arena, zone_class)) {
			DEBUGOUT("locked arenas[%u]", i);
			break;
		}
		arena = NULL;
	}
	if (arena == NULL) {
		arena = &g_yoyo_realm.arenas[0];
		if (!lock_arena(arena, zone_class)) {
			return NULL;
		}
		DEBUGOUT("locked arenas[%u]", 0);
	}
	return arena;
}

// stub
void*	allocate_from_large(t_yoyo_large_arena* subarena, size_t n) {
	(void)subarena;
	(void)n;
	assert(false);
	return NULL;
}

static void	zone_push_front(t_yoyo_zone** list, t_yoyo_zone* zone) {
	assert(list != NULL);
	zone->next = *list;
	*list = zone;
}

// zone からサイズ n の chunk を取得しようとする.
void*	try_allocate_from_zone(t_yoyo_zone* zone, size_t n) {
	(void)zone;
	(void)n;
	DEBUGOUT("try from: %p - %zu", zone, n);
	return NULL;
}

void*	allocate_from_zone_list(t_yoyo_arena* arena, t_yoyo_zone_class zone_class, size_t n) {
	t_yoyo_normal_arena* subarena = (t_yoyo_normal_arena*)get_subarena(arena, zone_class);
	t_yoyo_zone*	head = subarena->head;
	while (head != NULL) {
		if (lock_zone(head)) {
			// zone ロックが取れた -> アロケートを試みる
			void*	mem = try_allocate_from_zone(head, n);
			unlock_zone(head);
			if (mem != NULL) {
				return mem;
			}
		}
		// zone ロックを取らないオペレーションは, 既存の zone の next をいじらないので,
		// zone ロックを取らなくても zone->next を触っていいはず. たぶん.
		head = head->next;
	}
	// どの zone からもアロケートできなかった -> zone を増やしてもう一度
	t_yoyo_zone*	new_zone = allocate_zone(arena, zone_class);
	if (new_zone == NULL) {
		return NULL;
	}
	zone_push_front(&subarena->head, new_zone);
	// new_zone はロック取らなくていい.
	return try_allocate_from_zone(new_zone, n);
}

void*	allocate_from_arena(t_yoyo_arena* arena, t_yoyo_zone_class zone_class, size_t n) {
	if (zone_class == YOYO_ZONE_LARGE) {
		return allocate_from_large(&arena->large, n);
	}
	// TINY / SMALL からアロケートする
	t_yoyo_subarena* subarena = get_subarena(arena, zone_class);
	assert(subarena != NULL);
	assert((void*)subarena != (void*)&arena->large);
	return allocate_from_zone_list(arena, zone_class, n);
}

void*	actual_malloc(size_t n) {
	// ゾーン種別決定
	t_yoyo_zone_class zone_class = zone_class_for_bytes(n);

	// 対応するアリーナを選択してロックする
	t_yoyo_arena* arena = occupy_arena(zone_class);
	if (arena == NULL) {
		DEBUGWARN("%s", "failed: couldn't lock any arena");
		return NULL;
	}

	void*	mem = allocate_from_arena(arena, zone_class, n);

	// アンロックする
	unlock_arena(arena, zone_class);
	return mem;
}

