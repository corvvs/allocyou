#ifndef YOYO_PRINTF_INTERNAL_H
# define YOYO_PRINTF_INTERNAL_H

# include <stdlib.h>
# include <unistd.h>
# include <stdarg.h>
# include <stdbool.h>
# include <assert.h>

# define YOYO_PRINT_BUFSIZE (1024 * 8)
# define YOYO_CHARSET_LENGTH_MODIFIER "lz"
# define YOYO_CHARSET_CONVERSION "csdiupxb%"

typedef struct	s_yoyo_printf_buffer {
	int		fd;
	int		printed;
	size_t	size;
	char	buffer[YOYO_PRINT_BUFSIZE];
}	t_yoyo_printf_buffer;

typedef enum	e_yoyo_length_modifier {
	YOYO_LM_DEFAULT,
	YOYO_LM_L,
	YOYO_LM_LL,
	YOYO_LM_Z,
}	t_yoyo_length_modifier;

typedef struct	s_yoyo_conversion {
	const char*				body;
	size_t					size;
	t_yoyo_length_modifier	length_modifier;
	char					conversion_type;
}	t_yoyo_conversion;

#endif
