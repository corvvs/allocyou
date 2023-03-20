#ifndef TEST_UTILS_H
# define TEST_UTILS_H

# include <stdlib.h>
# include <unistd.h>
# include <signal.h>
# include <sys/wait.h>
# include "includes/internal.h"

typedef enum e_yoyo_exit {
	YOYO_EXIT_OK,
	YOYO_EXIT_ERROR,
	YOYO_EXIT_SIGNAL,
	YOYO_EXIT_UNEXPECTED,
}	t_yoyo_exit;

#endif
