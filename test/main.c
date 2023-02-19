#include "yo_common.h"
#include "yo_malloc.h"
#include <string.h>

void	test1() {
	show_alloc_mem();
	PRINT_STATE_AFTER(char *a = yo_malloc(1));
	PRINT_STATE_AFTER(char *b = yo_malloc(20));
	PRINT_STATE_AFTER(char *c = yo_malloc(50));
	PRINT_STATE_AFTER(yo_free(b));
	PRINT_STATE_AFTER(yo_free(a));
	PRINT_STATE_AFTER(char *d = yo_malloc(72));
	PRINT_STATE_AFTER(char *e = yo_malloc(1));
	PRINT_STATE_AFTER(char *f = yo_malloc(1));
	PRINT_STATE_AFTER(char *g = yo_malloc(123456789));
	PRINT_STATE_AFTER(yo_free(c));
	PRINT_STATE_AFTER(yo_free(e));
	PRINT_STATE_AFTER(yo_free(g));
	PRINT_STATE_AFTER(yo_free(f));
	PRINT_STATE_AFTER(yo_free(d));
}

void	test2() {
	uintptr_t d = 0;
	for (size_t i = 0; i < 300; ++i) {
		char *s = malloc(100);
		printf("s = %lu %lu\n", (uintptr_t)s, (uintptr_t)s - d);
		d = (uintptr_t)s;
		free(s);
	}
}

void	test3() {
	PRINT_STATE_AFTER(char *a = yo_malloc(12345678));
	printf("a = %p\n", a);
	PRINT_STATE_AFTER(char *b = yo_malloc(12345678));
	printf("b = %p\n", b);
	PRINT_STATE_AFTER(char *c = yo_malloc(12345678));
	printf("c = %p\n", c);
	PRINT_STATE_AFTER(char *d = yo_malloc(12345678));
	printf("d = %p\n", d);
	PRINT_STATE_AFTER(yo_free(d));
	PRINT_STATE_AFTER(yo_free(c));
	PRINT_STATE_AFTER(yo_free(a));
	PRINT_STATE_AFTER(yo_free(b));
}

void	test4() {
	size_t n = 123456789;
	PRINT_STATE_AFTER(char *a = yo_malloc(n));
	PRINT_STATE_AFTER(char *b = yo_malloc(n));
	memset(a, 'a', n);
	memset(b, 'b', n);
	a[n] = 0;
	b[n] = 0;
	printf("%c %c\n", a[n - 1], b[n - 1]);
	PRINT_STATE_AFTER(yo_free(b));
	PRINT_STATE_AFTER(yo_free(a));
}

void	test5() {
	size_t n = 1;
	size_t m = 2;
	char *a = yo_realloc(NULL, n);
	char *_ = yo_realloc(NULL, 1);
	memset(a, 'A', n);
	char *b = yo_realloc(a, m);
	DEBUGOUT("%p(%zu) -> %p(%zu)", a, n, b, m);
	a = b;
	a[0] = 'B';
	a[m - 1] = 0;
	m = 3;
	b = yo_realloc(a, m);
	DEBUGOUT("-> %p(%zu)", b, m);
	write(STDERR_FILENO, b, m);
	write(STDERR_FILENO, "\n", 1);
	a = b;
	m = 6;
	b = yo_realloc(a, m);
	DEBUGOUT("-> %p(%zu)", b, m);
	write(STDERR_FILENO, b, m);
	write(STDERR_FILENO, "\n", 1);
	a = b;
	m = 12;
	b = yo_realloc(a, m);
	DEBUGOUT("-> %p(%zu)", b, m);
	write(STDERR_FILENO, b, m);
	write(STDERR_FILENO, "\n", 1);
	a = b;
	m = 24;
	b = yo_realloc(a, m);
	DEBUGOUT("-> %p(%zu)", b, m);
	write(STDERR_FILENO, b, m);
	write(STDERR_FILENO, "\n", 1);
	a = b;
	yo_free(a);
	yo_free(_);
}

#define MASS_RANDOM_N 10000

void	mass_random() {
	void	*m[MASS_RANDOM_N];

	for (int i = 0; i < MASS_RANDOM_N; ++i) {
		m[i] = yo_malloc(rand() % 100000 + 1);
	}
	for (int i = 0; i < MASS_RANDOM_N; ++i) {
		yo_free(m[i]);
	}
}

int main() {
	setvbuf(stdout, NULL, _IONBF, 0);
	// test1();
	// test2();
	// test3();
	// test4();
	// test5();
	mass_random();

}
