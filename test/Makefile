
CC		:= gcc
CFLAGS	:= -Wall -Wextra -Werror -g -fsanitize=address

SRCDIR	:= ./
OBJDIR	:= ./
SRCS	:=\
			yo_malloc.c\
			yo_list.c\
			yo_allocation.c\

OBJS	:= $(SRCS:$(SRCDIR)%.c=$(OBJDIR)%.o)

all: malloc


.PHONY:	malloc
malloc:	$(OBJS)
	$(CC) $(CFLAGS) -o $@ main.c $(OBJS)
	./$@


