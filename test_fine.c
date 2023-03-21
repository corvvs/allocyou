#include "malloc.h"
#include "internal.h"
#include "test_malloc.h"
#include "libft/libft.h"

#include <limits.h>
#include <errno.h>

void	test_tiny_fine(void) {
	char*	str1 = malloc(16);
	EXPECT_IS_NOT_NULL(str1);
	ft_strlcpy(str1, "hello world", 16);
	EXPECT_EQ_STR(str1, "hello world");

	char*	str2 = malloc(16);
	EXPECT_IS_NOT_NULL(str2);
	ft_strlcpy(str2, "goodbye saitama", 16);
	EXPECT_EQ_STR(str1, "hello world");
	EXPECT_EQ_STR(str2, "goodbye saitama");

	free(str2);
	EXPECT_EQ_STR(str1, "hello world");

	str2 = malloc(16);
	EXPECT_IS_NOT_NULL(str2);
	EXPECT_EQ_STR(str1, "hello world");
}

void	test_realloc_fine(void) {
	char*	str1 = malloc(16);
	EXPECT_IS_NOT_NULL(str1);
	ft_strlcpy(str1, "hello world", 16);
	EXPECT_EQ_STR(str1, "hello world");

	char*	str2 = realloc(str1, 160);
	yoyo_dprintf(STDOUT_FILENO, "str1 should be RELOCATED\n");
	EXPECT_IS_NOT_NULL(str2);
	EXPECT_NE(str1, str2);
	EXPECT_EQ_STR(str2, "hello world");

	char*	str3 = realloc(str2, 16000);
	yoyo_dprintf(STDOUT_FILENO, "str3 should be RELOCATED\n");
	EXPECT_IS_NOT_NULL(str3);
	EXPECT_NE(str2, str3);
	EXPECT_EQ_STR(str3, "hello world");

	char*	str4 = realloc(str3, 18);
	yoyo_dprintf(STDOUT_FILENO, "str4 should be RELOCATED\n");
	EXPECT_IS_NOT_NULL(str4);
	EXPECT_NE(str3, str4);
	EXPECT_EQ_STR(str4, "hello world");

	char*	str5 = realloc(str4, 16);
	yoyo_dprintf(STDOUT_FILENO, "str5 should not be RELOCATED\n");
	EXPECT_EQ(str4, str5);
	EXPECT_IS_NOT_NULL(str5);
	EXPECT_EQ_STR(str5, "hello world");
}

