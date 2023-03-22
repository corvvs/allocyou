#include "internal.h"

static void	unmap_range(void* begin, void* end) {
	assert((size_t)begin <= (size_t)end);
	if((size_t)begin == (size_t)end) {
		DEBUGOUT("skipped: [%p, %p)", begin, end);
		return;
	}
	int rv;
	{
		SPRINT_START;
		rv = munmap(begin, (size_t)end - (size_t)begin);
		SPRINT_END("munmap");
	}
	if (rv) {
		DEBUGFATAL("FAILED: errno = %d, %s", errno, strerror(errno));
		return;
	}
	DEBUGINFO("unmapped: [%p, %p) (%zd B)", begin, end, end - begin);
}

// bytes バイトの領域を mmap して返す.
// align がtrueなら領域は bytes バイトアラインされる.
// ただしその場合 bytes が2冪でなければならない.
void*	yoyo_map_memory(size_t bytes, bool align) {
	assert(!align || is_power_of_2(bytes));
	const size_t	bulk_size = align ? 2 * bytes : bytes;
	void*	bulk;
	errno = 0;
	{
		bulk = mmap(
			NULL,
			bulk_size,
			PROT_READ | PROT_WRITE,
			MAP_ANON | MAP_PRIVATE,
			-1, 0);
	}
	if (bulk == MAP_FAILED) {
		if (errno != ENOMEM) {
			DEBUGFATAL("for bulk_size %zu B, mmap is failed and errno is not ENOMEM. errno: %d %s", bulk_size, errno, strerror(errno));
		} else {
			errno = ENOMEM;
		}
		return NULL;
	}
	if (align) {
		DEBUGOUT("aligning region %p(%zu B) to %zu B", bulk, bulk_size, bytes);
		void*	mem = (void*)CEIL_BY((size_t)bulk, bytes);
		void*	end = mem + bytes;
		unmap_range(bulk, mem);
		unmap_range(end, bulk + bytes * 2);
		assert(((uintptr_t)mem & ((uintptr_t)bytes - 1)) == 0); // mem は bytes アラインされている
		DEBUGINFO("returning aligned region %p(%zu B)", mem, bytes);
		return mem;
	} else {
		DEBUGINFO("returning bulk region %p(%zu B)", bulk, bulk_size);
		return bulk;
	}
}

void	yoyo_unmap_memory(void* start, size_t size) {
	unmap_range(start, start + size);
}
