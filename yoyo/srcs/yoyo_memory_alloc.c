#include "yoyo_internal.h"

static void	unmap_range(void* begin, void* end) {
	if((size_t)begin >= (size_t)end) {
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
		DEBUGERR("FAILED: errno = %d, %s", errno, strerror(errno));
		return;
	}
	DEBUGOUT("unmapped: [%p, %p) (%zdB)", begin, end, end - begin);
}

// bytes バイトの領域を mmap して返す.
// 領域は bytes バイトアラインされる.
void*	allocate_memory(size_t bytes) {
	assert((bytes & (bytes - 1)) == 0); // bytes は 2冪である
	void*	bulk;
	{
		SPRINT_START;
		bulk = mmap(
			NULL,
			bytes * 2,
			PROT_READ | PROT_WRITE,
			MAP_ANON | MAP_PRIVATE,
			-1, 0);
		SPRINT_END("mmap");
	}
	if (bulk == MAP_FAILED) {
		return NULL;
	}
	void*	mem = (void*)CEIL_BY((size_t)bulk, bytes);
	void*	end = mem + bytes;
	unmap_range(bulk, mem);
	unmap_range(end, bulk + bytes * 2);
	assert(((uintptr_t)mem & ((uintptr_t)bytes - 1)) == 0); // mem は bytes アラインされている
	return mem;
}

void	deallocate_memory(void* start, size_t size) {
	unmap_range(start, start + size);
}
