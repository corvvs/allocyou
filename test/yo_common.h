#ifndef YO_COMMON_H
# define YO_COMMON_H

# include <unistd.h>
# include <stdio.h>

# define DEBUGSTR(format) dprintf(STDERR_FILENO, "[%s:%d %s] " format, __FILE__, __LINE__, __func__)
# define DEBUGOUT(format, ...) dprintf(STDERR_FILENO, "[%s:%d %s] " format, __FILE__, __LINE__, __func__, __VA_ARGS__)
# define DEBUGWARN(format, ...) dprintf(STDERR_FILENO, "[%s:%d %s] " format, __FILE__, __LINE__, __func__, __VA_ARGS__)
# define OUT_VAR_INT(var) printf(#var " = %d\n", var)
# define OUT_VAR_SIZE_T(var) printf(#var " = %zu\n", var)
# define OUT_VAR_ADDR(var) printf(#var " = %p\n", var)
# define OUT_VAR_STR(var) printf(#var " = \"%s\"\n", var)
# define PRINT_STATE_AFTER(proc) proc; DEBUGSTR("DA: " #proc "\n"); show_alloc_mem();

#endif
