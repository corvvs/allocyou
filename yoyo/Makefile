
ifeq ($(HOSTTYPE),)
	HOSTTYPE := $(shell uname -m)_$(shell uname -s)
endif

SRCDIR	:=	srcs
OBJDIR	:=	objs
INCDIR	:=	includes
FILES	:=	\
			yoyo_actual_malloc.c\
			yoyo_actual_free.c\
			yoyo_arena_initialize.c\
			yoyo_lock.c\
			yoyo_malloc.c\
			yoyo_memory_alloc.c\
			yoyo_realm_initialize.c\
			yoyo_zone_initialize.c\
			yoyo_zone_operation.c\
			yoyo_zone_utils.c\

SRCS	:=	$(FILES:%.c=$(SRCDIR)/%.c)
OBJS	:=	$(FILES:%.c=$(OBJDIR)/%.o)
NAME	:=	libmalloc.a
RM		:=	rm -rf

CC		:=	gcc
CFLAGS	:=	-Wall -Wextra -Werror -O2 -I$(INCDIR) -g -fsanitize=undefined

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
