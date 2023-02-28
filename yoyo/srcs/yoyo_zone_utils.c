#include "yoyo_internal.h"

size_t	zone_bytes_for_zone_class(t_yoyo_zone_class zone_class) {
	switch (zone_class) {
		case YOYO_ZONE_TINY:
			return ZONE_TINY_BYTE;
		case YOYO_ZONE_SMALL:
			return ZONE_SMALL_BYTE;
		default:
			return 0;
	}
}
