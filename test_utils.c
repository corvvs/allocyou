#include "test_utils.h"

t_yoyo_exit	exec_test(void	(*func)(void), int* signal) {
	pid_t pid = fork();
	if (pid >= 0) {
		if (pid == 0) {
			func();
			exit(0);
		} else {
			int	status;
			wait(&status);
			if (WIFEXITED(status)) {
				// exited by exit() or _exit() or main()
				int sv = WEXITSTATUS(status);
				if (sv == 0)
					return (YOYO_EXIT_OK);
				else
					return (YOYO_EXIT_ERROR);
			}
			if (WIFSIGNALED(status)) {
				// exited by signal
				*signal = WTERMSIG(status);
				return YOYO_EXIT_SIGNAL;
			}
			return (YOYO_EXIT_UNEXPECTED);
		}
	}
	return (YOYO_EXIT_ERROR);
}
