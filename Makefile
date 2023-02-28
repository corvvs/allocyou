
CC		:= clang
CFLAGS	:= -Wall -Wextra -Werror -O2 -g -D SPRINT
# CFLAGS	:= -Wall -Wextra -Werror -g -D USE_LIBC -D NDEBUG

ifeq ($(HOSTTYPE),)
	HOSTTYPE := $(shell uname -m)_$(shell uname -s)
endif

SRCDIR	:=	.
OBJDIR	:=	objs
SRCS	:=\
			yo_malloc.c\
			yo_actual_free.c\
			yo_actual_malloc.c\
			yo_actual_realloc.c\
			yo_actual_show_alloc_mem.c\
			yo_zone.c\
			yo_list.c\
			yo_heap.c\
			yo_large.c\
			yo_utils.c\
			yo_predicates.c\
			yo_consistency.c

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
