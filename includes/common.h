#ifndef YOYO_COMMON_H
# define YOYO_COMMON_H

// [デバッグ用出力]

# include <unistd.h>
# include <stdio.h>
# include "printf.h"

# define TX_RED "\e[31m"
# define BG_RED "\e[41m"
# define TX_GRN "\e[32m"
# define TX_BLU "\e[34m"
# define TX_YLW "\e[33m"
# define TX_GRY "\e[30m"
# define TX_RST "\e[0m"

# ifdef DEBUG
// #  define DEBUGSTRN(format) yoyo_dprintf(     STDERR_FILENO,     "%s[%s:%d %s][%d:%d] " format "%s", TX_GRY,   __FILE__, __LINE__, __func__, yoyo_thread_id, yoyo_invokation_id, TX_RST)
// #  define DEBUGSTR(format) yoyo_dprintf(      STDERR_FILENO,     "%s[%s:%d %s][%d:%d] " format "%s\n", TX_GRY, __FILE__, __LINE__, __func__, yoyo_thread_id, yoyo_invokation_id, TX_RST)
// #  define DEBUGOUT(format, ...) yoyo_dprintf( STDERR_FILENO, "%s[D] [%s:%d %s][%d:%d] " format "%s\n", TX_GRY, __FILE__, __LINE__, __func__, yoyo_thread_id, yoyo_invokation_id, __VA_ARGS__, TX_RST)
// #  define DEBUGINFO(format, ...) yoyo_dprintf(STDERR_FILENO,   "[I] [%s:%d %s][%d:%d] " format "\n",           __FILE__, __LINE__, __func__, yoyo_thread_id, yoyo_invokation_id, __VA_ARGS__)
// #  define DEBUGWARN(format, ...) yoyo_dprintf(STDERR_FILENO, "%s[W] [%s:%d %s][%d:%d] " format "%s\n", TX_YLW, __FILE__, __LINE__, __func__, yoyo_thread_id, yoyo_invokation_id, __VA_ARGS__, TX_RST)
// #  define DEBUGERR(format, ...) yoyo_dprintf( STDERR_FILENO, "%s[E] [%s:%d %s][%d:%d] " format "%s\n", TX_RED, __FILE__, __LINE__, __func__, yoyo_thread_id, yoyo_invokation_id, __VA_ARGS__, TX_RST)
// #  define DEBUGSAY(format, ...) yoyo_dprintf( STDERR_FILENO,   "[s] [%s:%d %s][%d:%d] " format "\n",           __FILE__, __LINE__, __func__, yoyo_thread_id, yoyo_invokation_id, __VA_ARGS__)
#  define DEBUGSTRN(format) yoyo_dprintf(     STDERR_FILENO,     "%s[%s:%d %s] " format "%s", TX_GRY,   __FILE__, __LINE__, __func__, TX_RST)
#  define DEBUGSTR(format) yoyo_dprintf(      STDERR_FILENO,     "%s[%s:%d %s] " format "%s\n", TX_GRY, __FILE__, __LINE__, __func__, TX_RST)
#  define DEBUGOUT(format, ...) yoyo_dprintf( STDERR_FILENO, "%s[D] [%s:%d %s] " format "%s\n", TX_GRY, __FILE__, __LINE__, __func__, __VA_ARGS__, TX_RST)
#  define DEBUGINFO(format, ...) yoyo_dprintf(STDERR_FILENO,   "[I] [%s:%d %s] " format "\n",           __FILE__, __LINE__, __func__, __VA_ARGS__)
#  define DEBUGWARN(format, ...) yoyo_dprintf(STDERR_FILENO, "%s[W] [%s:%d %s] " format "%s\n", TX_YLW, __FILE__, __LINE__, __func__, __VA_ARGS__, TX_RST)
#  define DEBUGERR(format, ...) yoyo_dprintf( STDERR_FILENO, "%s[E] [%s:%d %s] " format "%s\n", TX_RED, __FILE__, __LINE__, __func__, __VA_ARGS__, TX_RST)
#  define DEBUGSAY(format, ...) yoyo_dprintf( STDERR_FILENO,   "[s] [%s:%d %s] " format "\n",           __FILE__, __LINE__, __func__, __VA_ARGS__)
# else
#  define DEBUGSTRN(format) ((void)0)
#  define DEBUGSTR(format) ((void)0)
#  define DEBUGOUT(format, ...) ((void)0)
#  define DEBUGINFO(format, ...) ((void)0)
#  define DEBUGWARN(format, ...) ((void)0)
#  define DEBUGERR(format, ...) ((void)0)
// #  define DEBUGSAY(format, ...) yoyo_dprintf(STDERR_FILENO, "[s:%d] [%s:%d %s] " format "\n", getpid(), __FILE__, __LINE__, __func__, __VA_ARGS__)
#  define DEBUGSAY(format, ...) ((void)0)
# endif
// # define DEBUGFATAL(format, ...) yoyo_dprintf(STDERR_FILENO, "%s[F] [%s:%d %s][%d:%d] " format "%s\n", BG_RED, __FILE__, __LINE__, __func__, yoyo_thread_id, yoyo_invokation_id, __VA_ARGS__, TX_RST)
# define DEBUGFATAL(format, ...) yoyo_dprintf(STDERR_FILENO, "%s[F] [%s:%d %s] " format "%s\n", BG_RED, __FILE__, __LINE__, __func__, __VA_ARGS__, TX_RST)

#endif
