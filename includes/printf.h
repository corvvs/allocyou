#ifndef YOYO_PRINTF_H
# define YOYO_PRINTF_H

# include <unistd.h>
# include <stdlib.h>

ssize_t	yoyo_dprintf(int fd, const char* format, ...);

#endif
