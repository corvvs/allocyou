#ifndef YOYO_TIME_H
# define YOYO_TIME_H

# include <sys/time.h>
# include "printf.h"
typedef struct timeval t_tv;
# define STIME(t0) (1000000 * (unsigned long long)t0.tv_sec + (unsigned long long)t0.tv_usec)
# define TIME_DIFF(t0, t1) (STIME(t1) - STIME(t0))
# ifdef USE_SPRINT
#  define SPRINT_START t_tv yo_t0; gettimeofday(&yo_t0, NULL)
#  define SPRINT_END(label) t_tv yo_t1; gettimeofday(&yo_t1, NULL); yoyo_dprintf(STDERR_FILENO, "<time> %s %llu\n", label, TIME_DIFF(yo_t0, yo_t1))
# else
#  define SPRINT_START (void)(0)
#  define SPRINT_END(label) ((void)label)
# endif

#endif
