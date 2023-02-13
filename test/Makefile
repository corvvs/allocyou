
CC		:= gcc
CFLAGS	:= -Wall -Wextra -Werror -g -fsanitize=address

all: mmap


.PHONY:	mmap
mmap:
	$(CC) $(CFLAGS) -o $@ $@.c
	./$@
