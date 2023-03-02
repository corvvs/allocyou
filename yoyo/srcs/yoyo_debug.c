#include "yoyo_internal.h"

// zone の状態を出力する.
// ロックは必要なら取っておくこと.
void	print_zone_state(const t_yoyo_zone* zone) {
	DEBUGOUT(
		"ZONE %p: MT: %s, class: %d, blocks: zone %u, heap %u, free %u, used %u",
		zone, zone->multi_thread ? "Y" : "N",
		zone->zone_type, zone->blocks_zone, zone->blocks_heap, zone->blocks_free, zone->blocks_used
	);
}
