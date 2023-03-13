#include "malloc.h"
#include "internal.h"

#define N_THREADS 400
#define N_LOCKS 20

typedef struct s_basket_1 {
	pthread_mutex_t	locks[N_LOCKS];
	pthread_t		threads[N_THREADS];
	char*			strings[N_LOCKS];
}	t_basket_1;
static t_basket_1	basket;
static int			indexes[N_THREADS];

static void*	test_multithread_basic_sub(void* index) {
	DEBUGWARN("index = %p", index);
	int idx = *(int*)index;
	DEBUGWARN("#%d launched", idx);

	int count = 1000;
	char a[] = "0";
	a[0] += idx;
	for (int i = 0; i < count; ++i) {
		// show_alloc_mem();
		{
			int k = rand() % N_LOCKS;
			DEBUGINFO("#%d *%d taking %d-th lock", idx, i, k);
			pthread_mutex_lock(&basket.locks[k]);
			{
				DEBUGINFO("#%d *%d locked %d-th lock", idx, i, k);
				if (basket.strings[k]) {
					yoyo_free(basket.strings[k]);
					basket.strings[k] = NULL;
				}
				char* str = yoyo_malloc(20 + rand() % 200000);
				strcpy(str, "<thread #");
				strcat(str, a);
				strcat(str, ">");
				basket.strings[k] = str;
			}
			pthread_mutex_unlock(&basket.locks[k]);
			DEBUGINFO("#%d *%d unlocked %d-th lock", idx, i, k);
		}
		// show_alloc_mem();

		{
			int k = rand() % N_LOCKS;
			DEBUGINFO("#%d *%d taking %d-th lock", idx, i, k);
			pthread_mutex_lock(&basket.locks[k]);
			{
				DEBUGINFO("#%d *%d locked %d-th lock", idx, i, k);
				if (basket.strings[k]) {
					DEBUGINFO("#%d *%d says: %s", idx, i, basket.strings[k]);
				} else {
					DEBUGINFO("#%d *%d says: <EMPTY>", idx, i);
				}
			}
			pthread_mutex_unlock(&basket.locks[k]);
			DEBUGINFO("#%d *%d unlocked %d-th lock", idx, i, k);
		}
	}
	// show_alloc_mem();
	DEBUGWARN("#%d closing...", idx);
	return NULL;
}

void	test_multithread_basic(void) {
	for (int i = 0; i < N_LOCKS; ++i) {
		pthread_mutex_init(&basket.locks[i], NULL);
		basket.strings[i] = NULL;
	}
	for (int i = 0; i < N_THREADS; ++i) {
		indexes[i] = i + 1;
		int* ip = &indexes[i];
		pthread_create(&basket.threads[i], NULL, test_multithread_basic_sub, ip);
	}

	for (int i = 0; i < N_THREADS; ++i) {
		pthread_join(basket.threads[i], NULL);
	}
	for (int i = 0; i < N_LOCKS; ++i) {
		pthread_mutex_destroy(&basket.locks[i]);
	}
	show_alloc_mem();
	for (int i = 0; i < N_LOCKS; ++i) {
		yoyo_free(basket.strings[i]);
	}
	DEBUGWARN("done: %d - %d", N_THREADS, N_LOCKS);
	dprintf(2, "%zu\n", sizeof(t_basket_1));
	dprintf(2, "%zu\n", sizeof(basket));
	dprintf(2, "%zu\n", sizeof(g_yoyo_realm));
	dprintf(2, "%p\n", &basket);
	dprintf(2, "%p\n", (void*)&basket + sizeof(basket));
	dprintf(2, "%p\n", &g_yoyo_realm);
}


static void*	test_multithread_realloc_sub(void* index) {
	DEBUGWARN("index = %p", index);
	int idx = *(int*)index;
	DEBUGWARN("#%d launched", idx);

	int count = 1000;
	char a[] = "0";
	a[0] += idx;
	for (int i = 0; i < count; ++i) {
		// show_alloc_mem();
		{
			int k = rand() % N_LOCKS;
			DEBUGINFO("#%d *%d taking %d-th lock", idx, i, k);
			pthread_mutex_lock(&basket.locks[k]);
			{
				DEBUGINFO("#%d *%d locked %d-th lock", idx, i, k);
				char* str = yoyo_realloc(basket.strings[k], 20 + rand() % 200000);
				strcpy(str, "<thread #");
				strcat(str, a);
				strcat(str, ">");
				basket.strings[k] = str;
			}
			pthread_mutex_unlock(&basket.locks[k]);
			DEBUGINFO("#%d *%d unlocked %d-th lock", idx, i, k);
		}
		// show_alloc_mem();

		{
			int k = rand() % N_LOCKS;
			DEBUGINFO("#%d *%d taking %d-th lock", idx, i, k);
			pthread_mutex_lock(&basket.locks[k]);
			{
				DEBUGINFO("#%d *%d locked %d-th lock", idx, i, k);
				if (basket.strings[k]) {
					DEBUGINFO("#%d *%d says: %s", idx, i, basket.strings[k]);
				} else {
					DEBUGINFO("#%d *%d says: <EMPTY>", idx, i);
				}
			}
			pthread_mutex_unlock(&basket.locks[k]);
			DEBUGINFO("#%d *%d unlocked %d-th lock", idx, i, k);
		}
	}
	show_alloc_mem();
	DEBUGWARN("#%d closing...", idx);
	return NULL;
}

void	test_multithread_realloc(void) {
	for (int i = 0; i < N_LOCKS; ++i) {
		pthread_mutex_init(&basket.locks[i], NULL);
		basket.strings[i] = NULL;
	}
	for (int i = 0; i < N_THREADS; ++i) {
		indexes[i] = i + 1;
		int* ip = &indexes[i];
		pthread_create(&basket.threads[i], NULL, test_multithread_realloc_sub, ip);
	}

	for (int i = 0; i < N_THREADS; ++i) {
		pthread_join(basket.threads[i], NULL);
	}
	for (int i = 0; i < N_LOCKS; ++i) {
		pthread_mutex_destroy(&basket.locks[i]);
	}
	show_alloc_mem();
	for (int i = 0; i < N_LOCKS; ++i) {
		yoyo_free(basket.strings[i]);
	}
	DEBUGWARN("done: %d - %d", N_THREADS, N_LOCKS);
	dprintf(2, "%zu\n", sizeof(t_basket_1));
	dprintf(2, "%zu\n", sizeof(basket));
	dprintf(2, "%zu\n", sizeof(g_yoyo_realm));
	dprintf(2, "%p\n", &basket);
	dprintf(2, "%p\n", (void*)&basket + sizeof(basket));
	dprintf(2, "%p\n", &g_yoyo_realm);
}

