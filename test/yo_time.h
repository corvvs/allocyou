#ifndef YO_TIME_H
# define YO_TIME_H

# include <sys/time.h>
typedef struct timeval t_tv;
# define STIME(t0) (1000000 * (unsigned long long)t0.tv_sec + (unsigned long long)t0.tv_usec)
# define TIME_DIFF(t0, t1) (STIME(t1) - STIME(t0))
# define SPRINT_START t_tv yo_t0; gettimeofday(&yo_t0, NULL);
# define SPRINT_END t_tv yo_t1; gettimeofday(&yo_t1, NULL); DEBUGINFO("time: %llu us\n", TIME_DIFF(yo_t0, yo_t1))

#endif
