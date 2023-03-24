#include "internal.h"

void*	yo_memset(void* dst, int ch, size_t n) {
	unsigned char*	ud = dst;
	while (n--) {
		*(ud++) = (unsigned char)ch;
	}
	return dst;
}

void*	yo_memcpy(void* dst, const void* src, size_t n) {
	if (dst == src || n == 0) {
		return dst;
	}
	unsigned char*			ud = dst;
	const unsigned char*	us = src;
	while (n--) {
		*(ud++) = *(us++);
	}
	return dst;
}

int		yo_strcmp(const char* s1, const char* s2) {
	const unsigned char*	u1 = (const unsigned char*)s1;
	const unsigned char*	u2 = (const unsigned char*)s2;
	while (*u1 && *u1 == *u2) {
		++u1;
		++u2;
	}
	return *u1 - *u2;
}

int	yo_isprint(int ch)
{
	return (' ' <= ch && ch <= '~');
}

int	yo_isdigit(int ch) {
	return (!((unsigned int)ch & ~63u)
		&& (0x3ff000000000000ull & (1ull << ch)));
}

bool	is_power_of_2(size_t bytes) {
	return bytes && (bytes & (bytes - 1)) == 0;
}

bool	overflow_by_addtion(size_t a, size_t b) {
	return a > SIZE_MAX - b;
}

// アドレス addr をチャンクの先頭アドレスとし, 実際のチャンクヘッダのアドレスを返す
t_yoyo_chunk*	addr_to_actual_header(void* addr) {
	t_yoyo_chunk*	header = addr_to_nominal_header(addr);
	if (IS_PSEUDO_HEADER(header)) {
		DEBUGOUT("JUMP %p -> %p", header, NEXT_OF(header));
		header = NEXT_OF(header);
	}
	return header;
}

// アドレス addr をチャンクの先頭アドレスとし, 名目上のチャンクヘッダのアドレスを返す.
// -> チャンクが擬似ヘッダを持つなら擬似ヘッダ, 持たないなら実際のヘッダ.
t_yoyo_chunk*	addr_to_nominal_header(void* addr) {
	return addr - CEILED_CHUNK_SIZE;
}
