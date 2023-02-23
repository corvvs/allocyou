
CC		:= gcc
CFLAGS	:= -Wall -Wextra -Werror -g -fsanitize=address

ifeq ($(HOSTTYPE),)
	HOSTTYPE := $(shell uname -m)_$(shell uname -s)
endif

SRCDIR	:=	.
OBJDIR	:=	objs
SRCS	:=\
			yo_front.c\
			yo_malloc.c\
			yo_list.c\
			yo_allocation.c\
			yo_realloc.c\
			yo_utils.c\
			yo_predicates.c

OBJS	:= $(SRCS:%.c=$(OBJDIR)/%.o)
NAME	:= libmalloc.a
RM		:= rm -rf

SONAME	:= 

all:	malloc

$(OBJDIR)/%.o: $(SRCDIR)/%.c
	@mkdir -p $(OBJDIR)
	$(CC) $(CFLAGS) -c $< -o $@

.PHONY:	malloc
malloc:	$(NAME)
	$(CC) $(CFLAGS) -o $@ main.c $(NAME)
	./$@

$(NAME):	$(OBJS)
	ar rcs $(NAME) $(OBJS)

clean:
	$(RM)	$(OBJDIR)

fclean:	clean
	$(RM)	$(NAME)

re:		fclean all
