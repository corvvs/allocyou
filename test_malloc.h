#ifndef TEST_MALLOC_H
# define TEST_MALLOC_H

# include <sys/time.h>
typedef struct timeval t_tv;

// test_mass.c
void	test_mass_basic(void);

// test_multithread.c
void	test_multithread_basic(void);
void	test_multithread_realloc(void);

// test_extreme.c
void	test_extreme_malloc(void);
void	test_extreme_realloc(void);

// test_fine.c
void	test_tiny_fine(void);
void	test_realloc_fine(void);


// テキスト色変
# define TX_RED "\e[31m"
# define TX_GRN "\e[32m"
# define TX_BLU "\e[34m"
# define TX_YLW "\e[33m"
# define TX_GRY "\e[30m"
# define TX_RST "\e[0m"

// 時間計測と表示
# define STIME(t0) (1000000 * (unsigned long long)t0.tv_sec + (unsigned long long)t0.tv_usec)
# define TIME_DIFF(t0, t1) (STIME(t1) - STIME(t0))

// テストラベル表示
# define OUT_TEST_LABEL(prefix, label, width) {\
	yoyo_dprintf(STDOUT_FILENO, "%s", TX_BLU);\
	int n = yoyo_dprintf(STDOUT_FILENO, "== < %s %s > ", prefix, label);\
	if (n < width) {\
		for (int i = 0; i < width - n; ++i) {\
			yoyo_dprintf(STDOUT_FILENO, "%s", "=");\
		}\
	}\
	yoyo_dprintf(STDOUT_FILENO, "%s\n", TX_RST);\
}
// テストケース結果表示
# define OUT_OK(format, ...) yoyo_dprintf(STDOUT_FILENO, "%s[ok] " format "%s\n", TX_GRN, __VA_ARGS__, TX_RST)
# define OUT_KO(format, ...) yoyo_dprintf(STDOUT_FILENO, "%s[KO] " format " @ %s:%d %s %s\n", TX_RED, __VA_ARGS__, __FILE__, __LINE__, __func__, TX_RST)

// テスト開始・終了時表示
# define START_TEST OUT_TEST_LABEL("start:", __func__, 120)
# define END_TEST OUT_TEST_LABEL("end:", __func__, 120)

// テストキッカー
# define EXEC_TEST(func) {\
	t_tv yo_t0; gettimeofday(&yo_t0, NULL);\
	OUT_TEST_LABEL("start:", #func, 120);\
	pid_t pid = fork();\
	if (pid >= 0) {\
		if (pid == 0) {\
			func();\
			exit(0);\
		} else {\
			int	status;\
			waitpid(pid, &status, 0);\
		}\
	}\
	OUT_TEST_LABEL("end:", #func, 120);\
	t_tv yo_t1; gettimeofday(&yo_t1, NULL);\
	yoyo_dprintf(STDOUT_FILENO, "%sduration: %llums%s\n", TX_BLU, TIME_DIFF(yo_t0, yo_t1) / 1000, TX_RST);\
}

// アサーション

# include <assert.h>
# include "libft/libft.h"


# define EXPECT_IS_NULL(ptr) {\
	if (ptr != NULL) {\
		OUT_KO("ptr %s is NOT NULL: %p", #ptr, ptr);\
		assert(ptr != NULL);\
		return;\
	}\
	OUT_OK("ptr %s is NULL", #ptr);\
}

# define EXPECT_IS_NOT_NULL(ptr) {\
	if (ptr == NULL) {\
		OUT_KO("ptr %s is NULL", #ptr);\
		assert(ptr == NULL);\
		return;\
	}\
	OUT_OK("ptr %s is not NULL: %p", #ptr, ptr);\
}

# define EXPECT_EQ(ptr1, ptr2) {\
	if (ptr1 != ptr2) {\
		OUT_KO("ptr1 %s %p != ptr2 %s %p", #ptr1, ptr1, #ptr2, ptr2);\
		assert(ptr1 == ptr2);\
		return;\
	}\
	OUT_OK("ptr1 %s == ptr2 %s %p", #ptr1, #ptr2, ptr2);\
}

# define EXPECT_NE(ptr1, ptr2) {\
	if (ptr1 == ptr2) {\
		OUT_KO("ptr1 %s == ptr2 %s %p", #ptr1, #ptr2, ptr2);\
		assert(ptr1 != ptr2);\
		return;\
	}\
	OUT_OK("ptr1 %s %p != ptr2 %s %p", #ptr1, ptr1, #ptr2, ptr2);\
}

# define EXPECT_EQ_I(v1, v2) {\
	if (v1 != v2) {\
		OUT_KO("v1 %s != v2 %s", #v1, #v2);\
		assert(v1 == v2);\
		return;\
	}\
	OUT_OK("v1 %s == v2 %s", #v1, #v2);\
}

# define EXPECT_NE_STR(v1, v2) {\
	if (!ft_strncmp(v1, v2, -1)) {\
		OUT_KO("v1 %s == v2 %s", #v1, #v2);\
		assert(0);\
		return;\
	}\
	OUT_OK("v1 %s != v2 %s", #v1, #v2);\
}

# define EXPECT_EQ_STR(v1, v2) {\
	if (ft_strncmp(v1, v2, -1)) {\
		OUT_KO("v1 %s != v2 %s", #v1, #v2);\
		assert(0);\
		return;\
	}\
	OUT_OK("v1 %s == v2 %s", #v1, #v2);\
}

#endif
