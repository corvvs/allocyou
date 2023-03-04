#ifndef YOYO_COMMON_H
# define YOYO_COMMON_H

// [デバッグ用出力]

# include <unistd.h>
# include <stdio.h>
# include "printf.h"

# define TX_RED "\e[31m"
# define TX_GRN "\e[32m"
# define TX_BLU "\e[34m"
# define TX_YLW "\e[33m"
# define TX_GRY "\e[30m"
# define TX_RST "\e[0m"

# ifdef NDEBUG
#  define DEBUGSTRN(format) (0)
#  define DEBUGSTR(format) (0)
#  define DEBUGOUT(format, ...) (0)
#  define DEBUGINFO(format, ...) (0)
#  define DEBUGWARN(format, ...) (0)
#  define DEBUGERR(format, ...) (0)
#  define PRINT_STATE_AFTER(proc) proc;
# else
#  define DEBUGSTRN(format) yoyo_dprintf(STDERR_FILENO, "%s[%s:%d %s] " format "%s", TX_GRY, __FILE__, __LINE__, __func__, TX_RST)
#  define DEBUGSTR(format) yoyo_dprintf(STDERR_FILENO, "%s[%s:%d %s] " format "%s\n", TX_GRY, __FILE__, __LINE__, __func__, TX_RST)
#  define DEBUGOUT(format, ...) yoyo_dprintf(STDERR_FILENO, "%s[%s:%d %s] " format "%s\n", TX_GRY, __FILE__, __LINE__, __func__, __VA_ARGS__, TX_RST)
#  define DEBUGINFO(format, ...) yoyo_dprintf(STDERR_FILENO, "[%s:%d %s] " format "\n", __FILE__, __LINE__, __func__, __VA_ARGS__)
#  define DEBUGWARN(format, ...) yoyo_dprintf(STDERR_FILENO, "%s[%s:%d %s] " format "%s\n", TX_YLW, __FILE__, __LINE__, __func__, __VA_ARGS__, TX_RST)
#  define DEBUGERR(format, ...) yoyo_dprintf(STDERR_FILENO, "%s[%s:%d %s] " format "%s\n", TX_RED, __FILE__, __LINE__, __func__, __VA_ARGS__, TX_RST)
#  define PRINT_STATE_AFTER(proc) DEBUGSTR("DA: " #proc); proc; show_alloc_mem();
# endif

#endif
