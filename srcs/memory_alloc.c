#include "internal.h"

static void	unmap_range(void* begin, void* end) {
	YOYO_ASSERT((size_t)begin <= (size_t)end);
	size_t range_size = (size_t)end - (size_t)begin;
	if (range_size == 0) {
		// DEBUGOUT("skipped: [%p, %p)", begin, end);
		return;
	} else if (begin > end) {
		DEBUGFATAL("skipped: [%p, %p)", begin, end);
		return;
	}
	int rv = munmap(begin, range_size);
	if (rv) {
		DEBUGFATAL("FAILED: errno = %d", errno);
		return;
	}
	// DEBUGOUT("unmap memory: [%p, %p) - %zx", begin, end, range_size);
	// DEBUGINFO("unmapped: [%p, %p) (%zd B)", begin, end, range_size);
}

// bytes バイトの領域を mmap して返す.
// align がtrueなら領域は bytes バイトアラインされる.
// ただしその場合 bytes が2冪でなければならない.
void*	yoyo_map_memory(size_t bytes, bool align) {
	YOYO_ASSERT(!align || is_power_of_2(bytes));
	const size_t	bulk_size = align ? 2 * bytes : bytes;
	errno = 0;
	void*	bulk = mmap(
		NULL,
		bulk_size,
		PROT_READ | PROT_WRITE,
		MAP_ANON | MAP_PRIVATE,
		-1, 0);
	if (bulk == MAP_FAILED) {
		if (errno != ENOMEM) {
			DEBUGFATAL("for bulk_size %zu B, mmap is failed and errno is not ENOMEM. errno: %d", bulk_size, errno);
		} else {
			errno = ENOMEM;
		}
		return NULL;
	}
	void*	bulk_end = bulk + bulk_size;
	// DEBUGOUT("map memory: [%p, %p) - %zx", bulk, bulk_end, bulk_size);
	if (align) {
		// DEBUGOUT("aligning region %p(%zx B) to %zu B", bulk, bulk_size, bytes);
		void*	mem = (void*)CEIL_BY((size_t)bulk, bytes);
		void*	mem_end = mem + bytes;
		unmap_range(bulk, mem);
		YOYO_ASSERT(((uintptr_t)mem & ((uintptr_t)bytes - 1)) == 0); // mem は bytes アラインされている
		unmap_range(mem_end, bulk_end);
		// DEBUGOUT("returning aligned region %p(%zu B)", mem, bytes);
		return mem;
	} else {
		// DEBUGOUT("returning bulk region %p(%zu B)", bulk, bulk_size);
		return bulk;
	}
}

void	yoyo_unmap_memory(void* start, size_t size) {
	unmap_range(start, start + size);
}
