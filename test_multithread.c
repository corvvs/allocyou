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

	int count = 10;
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
					free(basket.strings[k]);
					basket.strings[k] = NULL;
				}
				char* str = malloc(20 + rand() % 200000);
				strcpy(str, "<thread #");
				strcat(str, a);
				strcat(str, ">");
				basket.strings[k] = str;
			}
			DEBUGINFO("#%d *%d releasing %d-th lock", idx, i, k);
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
			DEBUGINFO("#%d *%d releasing %d-th lock", idx, i, k);
			pthread_mutex_unlock(&basket.locks[k]);
			DEBUGINFO("#%d *%d unlocked %d-th lock", idx, i, k);
		}
	}
	// show_alloc_mem();
	DEBUGWARN("#%d closing...", idx);
	return NULL;
}

void	test_multithread_basic(void) {
	yoyo_dprintf(STDOUT_FILENO, "start: %d - %d\n", N_THREADS, N_LOCKS);
	yoyo_dprintf(STDOUT_FILENO, "init %d locks\n", N_LOCKS);
	for (int i = 0; i < N_LOCKS; ++i) {
		pthread_mutex_init(&basket.locks[i], NULL);
		basket.strings[i] = NULL;
	}
	yoyo_dprintf(STDOUT_FILENO, "start %d threads\n", N_THREADS);
	for (int i = 0; i < N_THREADS; ++i) {
		indexes[i] = i + 1;
		int* ip = &indexes[i];
		pthread_create(&basket.threads[i], NULL, test_multithread_basic_sub, ip);
	}

	for (int i = 0; i < N_THREADS; ++i) {
		pthread_join(basket.threads[i], NULL);
	}
	yoyo_dprintf(STDOUT_FILENO, "joined %d threads\n", N_THREADS);
	for (int i = 0; i < N_LOCKS; ++i) {
		pthread_mutex_destroy(&basket.locks[i]);
	}
	yoyo_dprintf(STDOUT_FILENO, "destroyed %d threads\n", N_LOCKS);
	show_alloc_mem_ex();
	for (int i = 0; i < N_LOCKS; ++i) {
		free(basket.strings[i]);
	}
	release_memory();
	show_alloc_mem_ex();
	release_memory();
	show_alloc_mem_ex();
	yoyo_dprintf(STDOUT_FILENO, "done: %d - %d\n", N_THREADS, N_LOCKS);
}


static void*	test_multithread_realloc_sub(void* index) {
	DEBUGWARN("index = %p", index);
	int idx = *(int*)index;

	int count = 10;
	char a[] = "0";
	a[0] += idx;
	for (int i = 0; i < count; ++i) {
		// show_alloc_mem();
		{
			int k = rand() % N_LOCKS;
			int size = 20 + rand() % 200000;
			DEBUGINFO("#%d *%d taking %d-th lock for size %d", idx, i, k, size);
			pthread_mutex_lock(&basket.locks[k]);
			{
				DEBUGINFO("#%d *%d locked %d-th lock for size %d", idx, i, k, size);
				char* str = realloc(basket.strings[k], size);
				strcpy(str, "<thread #");
				strcat(str, a);
				strcat(str, ">");
				basket.strings[k] = str;
			}
			DEBUGINFO("#%d *%d releasing %d-th lock", idx, i, k);
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
			DEBUGINFO("#%d *%d releasing %d-th lock", idx, i, k);
			pthread_mutex_unlock(&basket.locks[k]);
			DEBUGINFO("#%d *%d unlocked %d-th lock", idx, i, k);
		}
	}
	// show_alloc_mem();
	DEBUGWARN("#%d closing...", idx);
	return NULL;
}

void	test_multithread_realloc(void) {
	yoyo_dprintf(STDOUT_FILENO, "start: %d - %d\n", N_THREADS, N_LOCKS);
	yoyo_dprintf(STDOUT_FILENO, "init %d locks\n", N_LOCKS);
	for (int i = 0; i < N_LOCKS; ++i) {
		pthread_mutex_init(&basket.locks[i], NULL);
		basket.strings[i] = NULL;
	}
	yoyo_dprintf(STDOUT_FILENO, "start %d threads\n", N_THREADS);
	for (int i = 0; i < N_THREADS; ++i) {
		indexes[i] = i + 1;
		int* ip = &indexes[i];
		if (pthread_create(&basket.threads[i], NULL, test_multithread_realloc_sub, ip)) {
			DEBUGFATAL("FAILED to create thread: %d", i);
		}
	}

	for (int i = 0; i < N_THREADS; ++i) {
		DEBUGINFO("joining thread #%d\n", i);
		pthread_join(basket.threads[i], NULL);
		DEBUGINFO("joined thread #%d\n", i);
	}
	yoyo_dprintf(STDOUT_FILENO, "joined %d threads\n", N_THREADS);
	for (int i = 0; i < N_LOCKS; ++i) {
		pthread_mutex_destroy(&basket.locks[i]);
	}
	yoyo_dprintf(STDOUT_FILENO, "destroyed %d locks\n", N_LOCKS);
	// show_alloc_mem();
	for (int i = 0; i < N_LOCKS; ++i) {
		free(basket.strings[i]);
	}
	yoyo_dprintf(STDOUT_FILENO, "done: %d - %d\n", N_THREADS, N_LOCKS);
}

void	test_singlethread_realloc(void) {
	yoyo_dprintf(STDOUT_FILENO, "start: %d - %d\n", N_THREADS, N_LOCKS);
	yoyo_dprintf(STDOUT_FILENO, "init %d locks\n", N_LOCKS);
	for (int i = 0; i < N_LOCKS; ++i) {
		pthread_mutex_init(&basket.locks[i], NULL);
		basket.strings[i] = NULL;
	}
	int index = 0;
	test_multithread_realloc_sub(&index);
	yoyo_dprintf(STDOUT_FILENO, "joined %d threads\n", N_THREADS);
	for (int i = 0; i < N_LOCKS; ++i) {
		pthread_mutex_destroy(&basket.locks[i]);
	}
	yoyo_dprintf(STDOUT_FILENO, "destroyed %d locks\n", N_LOCKS);
	// show_alloc_mem();
	for (int i = 0; i < N_LOCKS; ++i) {
		free(basket.strings[i]);
	}
}
