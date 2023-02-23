#include "yo_internal.h"

void	*yo_memcpy(void* dst, const void* src, size_t n)
{
	unsigned char*			ud;
	const unsigned char*	us;

	ud = dst;
	us = src;
	while (n--)
		*ud++ = *us++;
	return dst;
}
