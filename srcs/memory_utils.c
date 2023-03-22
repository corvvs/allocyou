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
		header = NEXT_OF(header);
	}
	return header;
}

// アドレス addr をチャンクの先頭アドレスとし, 名目上のチャンクヘッダのアドレスを返す.
// -> チャンクが擬似ヘッダを持つなら擬似ヘッダ, 持たないなら実際のヘッダ.
t_yoyo_chunk*	addr_to_nominal_header(void* addr) {
	return addr - CEILED_CHUNK_SIZE;
}
