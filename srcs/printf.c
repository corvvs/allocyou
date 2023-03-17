#include "printf_internal.h"

static char*	yoyo_strchr(char* s, char c) {
	while (*s) {
		if (*s == c) {
			return s;
		}
		++s;
	}
	return *s == c ? s : NULL;
}

static size_t	yoyo_strlen(const char* s) {
	size_t	n = 0;
	while (s[n]) {
		++n;
	}
	return n;
}

// buffer の中身が残っている場合は出力する
static int	flush_buffer(t_yoyo_printf_buffer* buffer) {
	if (buffer->printed < 0) { return -1; }
	if (buffer->size == 0) { return 0; }
	ssize_t written = write(buffer->fd, buffer->buffer, buffer->size);
	buffer->size = 0;
	if (written < 0) {
		buffer->printed = -1;
		return -1;
	} else {
		buffer->printed += written;
	}
	return 0;
}

static void	write_into_buffer(t_yoyo_printf_buffer* buffer, const void* data, size_t size) {
	const char*	uc = data;
	for (size_t i = 0; i < size;) {
		if (YOYO_PRINT_BUFSIZE <= buffer->size) {
			if (flush_buffer(buffer)) {
				return;
			}
		}
		buffer->buffer[buffer->size] = uc[i];
		++i;
		++buffer->size;
	}
}

static bool patch_length_modifier(t_yoyo_conversion* conversion, char c) {
	switch (c) {
		case 'l':
			if (conversion->length_modifier == YOYO_LM_DEFAULT) {
				conversion->length_modifier = YOYO_LM_L;
				return true;
			}
			if (conversion->length_modifier == YOYO_LM_L) {
				conversion->length_modifier = YOYO_LM_LL;
				return true;
			}
			break;
		case 'z':
			if (conversion->length_modifier == YOYO_LM_DEFAULT) {
				conversion->length_modifier = YOYO_LM_Z;
				return true;
			}
			break;
		default:
			break;
	}
	return false;
}

static bool	resolve_c(t_yoyo_printf_buffer* buffer, t_yoyo_conversion* conversion, char c) {
	(void)conversion;
	write_into_buffer(buffer, &c, 1);
	return true;
}

static bool	resolve_s(t_yoyo_printf_buffer* buffer, t_yoyo_conversion* conversion, const char* s) {
	(void)conversion;
	write_into_buffer(buffer, s, yoyo_strlen(s));
	return true;
}

static bool	resolve_d(t_yoyo_printf_buffer* buffer, t_yoyo_conversion* conversion, long long d) {
	if (d / 10)
		resolve_d(buffer, conversion, d / 10);
	else
		write_into_buffer(buffer, "-", d < 0);
	write_into_buffer(buffer, &("9876543210123456789"[9 + d % 10]), 1);
	return true;
}

static bool	resolve_u(t_yoyo_printf_buffer* buffer, t_yoyo_conversion* conversion, unsigned long long d) {
	if (d / 10)
		resolve_u(buffer, conversion, d / 10);
	write_into_buffer(buffer, &("0123456789"[d % 10]), 1);
	return true;
}


static bool	resolve_x(t_yoyo_printf_buffer* buffer, t_yoyo_conversion* conversion, unsigned long long d) {
	if (d / 16)
		resolve_x(buffer, conversion, d / 16);
	write_into_buffer(buffer, &("0123456789abcdef"[d % 16]), 1);
	return true;
}

static bool	resolve_p(t_yoyo_printf_buffer* buffer, t_yoyo_conversion* conversion, unsigned long long d) {
	resolve_s(buffer, conversion, "0x");
	resolve_x(buffer, conversion, d);
	return true;
}

static bool	resolve_b_val(t_yoyo_printf_buffer* buffer, t_yoyo_conversion* conversion, unsigned long long d) {
	if (d / 2)
		resolve_b_val(buffer, conversion, d / 2);
	write_into_buffer(buffer, &("01"[d % 2]), 1);
	return true;
}

static bool	resolve_b(t_yoyo_printf_buffer* buffer, t_yoyo_conversion* conversion, unsigned long long d) {
	resolve_s(buffer, conversion, "0b");
	return resolve_b_val(buffer, conversion, d);
}

static bool	resolve_conversion(t_yoyo_printf_buffer* buffer, t_yoyo_conversion* conversion, va_list* args) {
	switch (conversion->conversion_type) {
		case '%':
			return resolve_c(buffer, conversion, '%');
		case 'c':
			return resolve_c(buffer, conversion, va_arg(*args, int));
		case 's':
			return resolve_s(buffer, conversion, va_arg(*args, char*));
		case 'd':
		case 'i':
			switch (conversion->length_modifier) {
				case YOYO_LM_L:
					return resolve_d(buffer, conversion, va_arg(*args, long));
				case YOYO_LM_LL:
					return resolve_d(buffer, conversion, va_arg(*args, long long));
				case YOYO_LM_Z:
					return resolve_d(buffer, conversion, va_arg(*args, ssize_t));
				case YOYO_LM_DEFAULT:
				default:
					return resolve_d(buffer, conversion, va_arg(*args, int));
			}
		case 'u':
			switch (conversion->length_modifier) {
				case YOYO_LM_L:
					return resolve_u(buffer, conversion, va_arg(*args, unsigned long));
				case YOYO_LM_LL:
					return resolve_u(buffer, conversion, va_arg(*args, unsigned long long));
				case YOYO_LM_Z:
					return resolve_u(buffer, conversion, va_arg(*args, size_t));
				case YOYO_LM_DEFAULT:
				default:
					return resolve_u(buffer, conversion, va_arg(*args, unsigned int));
			}
		case 'p':
			return resolve_p(buffer, conversion, (unsigned long long)va_arg(*args, void*));
		case 'x':
			switch (conversion->length_modifier) {
				case YOYO_LM_L:
					return resolve_x(buffer, conversion, va_arg(*args, unsigned long));
				case YOYO_LM_LL:
					return resolve_x(buffer, conversion, va_arg(*args, unsigned long long));
				case YOYO_LM_Z:
					return resolve_x(buffer, conversion, va_arg(*args, size_t));
				case YOYO_LM_DEFAULT:
				default:
					return resolve_x(buffer, conversion, va_arg(*args, unsigned int));
			}
		case 'b':
			switch (conversion->length_modifier) {
				case YOYO_LM_L:
					return resolve_b(buffer, conversion, va_arg(*args, unsigned long));
				case YOYO_LM_LL:
					return resolve_b(buffer, conversion, va_arg(*args, unsigned long long));
				case YOYO_LM_Z:
					return resolve_b(buffer, conversion, va_arg(*args, size_t));
				case YOYO_LM_DEFAULT:
				default:
					return resolve_b(buffer, conversion, va_arg(*args, unsigned int));
			}
		default:
			break;
		}
	return false;
}

static ssize_t	exec_conversion(t_yoyo_printf_buffer* buffer, const char *format, va_list* args) {
	t_yoyo_conversion	conversion = {
		.body = format,
		.size = 0,
		.length_modifier = YOYO_LM_DEFAULT,
		.conversion_type = '\0',
	};
	while (true) {
		char c = format[conversion.size];
		if (!c) {
			write_into_buffer(buffer, conversion.body, conversion.size);
			break;
		} else if (yoyo_strchr(YOYO_CHARSET_LENGTH_MODIFIER, c)) {
			// length modifier
			if (!patch_length_modifier(&conversion, c)) {
				return -1;
			}
		} else if (yoyo_strchr(YOYO_CHARSET_CONVERSION, c)) {
			conversion.conversion_type = c;
			resolve_conversion(buffer, &conversion, args);
			++conversion.size;
			break;
		} else {
			write_into_buffer(buffer, conversion.body, conversion.size);
			break;
		}
		++conversion.size;
	}
	return conversion.size;
}

// dprintf のような関数
int	yoyo_dprintf(int fd, const char* format, ...) {
	va_list	args;
	t_yoyo_printf_buffer	buffer = {
		.fd = fd,
		.printed = 0,
		.size = 0,
	};

	size_t i = 0;
	va_start(args, format);
	while (format[i] && buffer.printed >= 0) {
		if (format[i] != '%') {
			write_into_buffer(&buffer, &format[i], 1);
			++i;
		} else {
			++i;
			i += exec_conversion(&buffer, &format[i], &args);
		}
	}
	va_end(args);

	flush_buffer(&buffer);
	return buffer.printed;
}
