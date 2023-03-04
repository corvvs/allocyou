
ifeq ($(HOSTTYPE),)
	HOSTTYPE := $(shell uname -m)_$(shell uname -s)
endif

SRCDIR	:=	srcs
OBJDIR	:=	objs
INCDIR	:=	includes
FILES	:=	\
			actual_malloc.c\
			actual_free.c\
			actual_realloc.c\
			visualize.c\
			arena_initialize.c\
			lock.c\
			malloc.c\
			memory_alloc.c\
			realm_initialize.c\
			zone_initialize.c\
			zone_bitmap.c\
			zone_operation.c\
			zone_utils.c\
			debug.c\
			printf.c\


SRCS	:=	$(FILES:%.c=$(SRCDIR)/%.c)
OBJS	:=	$(FILES:%.c=$(OBJDIR)/%.o)
NAME	:=	libmalloc.a
RM		:=	rm -rf

CC		:=	gcc
CFLAGS	:=	-Wall -Wextra -Werror -O2 -I$(INCDIR) -g# -fsanitize=undefined

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
