
CC		:= gcc
CFLAGS	:= -Wall -Wextra -Werror -g -fsanitize=address

ifeq ($(HOSTTYPE),)
	HOSTTYPE := $(shell uname -m)_$(shell uname -s)
endif

SRCDIR	:=
OBJDIR	:=
SRCS	:=\
			yo_front.c\
			yo_malloc.c\
			yo_list.c\
			yo_allocation.c\
			yo_realloc.c\
			yo_utils.c\
			yo_predicates.c\

OBJS	:= $(SRCS:$(SRCDIR)%.c=$(OBJDIR)%.o)
NAME	:= libmalloc.a
RM		:= rm -rf

SONAME	:= 

all:	malloc


$(OBJDIR):
	@mkdir -p $(OBJDIR)

.PHONY:	malloc
malloc:	$(NAME)
	$(CC) $(CFLAGS) -o $@ main.c $(NAME)
	./$@


$(NAME):	$(OBJDIR) $(OBJS)
	ar rcs $(NAME) $(OBJS)

clean:
	$(RM)	$(OBJS)

fclean:	clean
	$(RM)	$(NAME)

re:		fclean all
