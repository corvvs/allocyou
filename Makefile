
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
			memory_utils.c\
			debug.c\
			printf.c\


SRCS	:=	$(FILES:%.c=$(SRCDIR)/%.c)
OBJS	:=	$(FILES:%.c=$(OBJDIR)/%.o)
NAME	:=	libmalloc.a
LIBFT	:=	libft.a
LIBFT_DIR	:=	libft
RM		:=	rm -rf

FILES_TEST	:=\
			main.c\
			test_basic.c\
			test_mass.c\
			test_multithread.c\
			test_extreme.c\
			test_fine.c\
			test_utils.c\


OBJS_TEST	:=	$(FILES_TEST:%.c=$(OBJDIR)/%.o)

CC			:=	gcc
CCOREFLAGS	:=	-Wall -Wextra -Werror -O2 -I$(INCDIR)
CFLAGS		:=	$(CCOREFLAGS) -D NDEBUG -g #-fsanitize=thread
LIBFLAGS	:=	-fPIC -fpic

BASE_LIBNAME	:=	ft_malloc
SONAME		:=	libft_malloc_$(HOSTTYPE).so
BASE_SONAME	:=	libft_malloc.so
DYLIBNAME	:=	libft_malloc_$(HOSTTYPE).dylib
BASE_DYLIBNAME	:=	libft_malloc.dylib

all:			malloc

$(OBJDIR)/%.o:	$(SRCDIR)/%.c
	@mkdir -p $(OBJDIR)
	$(CC) $(CFLAGS) $(LIBFLAGS) -c $< -o $@

$(OBJDIR)/%.o:	%.c
	@mkdir -p $(OBJDIR)
	$(CC) $(CFLAGS) -c $< -o $@

.PHONY:			malloc
malloc:			$(NAME) $(OBJS_TEST) $(LIBFT) #$(BASE_DYLIBNAME)
	$(CC) $(CFLAGS) -o $@ $(OBJS_TEST) $(LIBFT) $(NAME)
	./$@

$(NAME):		$(OBJS)
	ar rcs $(NAME) $(OBJS)

so:				$(BASE_SONAME)

$(SONAME):		$(OBJS)
	$(CC) -shared -fPIC $^ -o $@

$(BASE_SONAME):	$(SONAME)
	rm -f $@
	ln -s $^ $@

dylib:			$(BASE_DYLIBNAME)

$(DYLIBNAME):	$(OBJS)
	$(CC) -shared -fPIC $^ -o $@
	# $(CC) -dynamiclib $^ -o $@

$(BASE_DYLIBNAME):	$(DYLIBNAME)
	rm -f $@
	ln -s $^ $@

$(LIBFT):
	$(MAKE) -C $(LIBFT_DIR)
	cp $(LIBFT_DIR)/$(LIBFT) .


clean:
	$(RM) $(OBJDIR) $(LIBFT)

fclean:			clean
	$(RM) $(NAME) $(SONAME) $(DYLIBNAME) $(BASE_SONAME) $(BASE_DYLIBNAME)

re:				fclean all


up:
	docker-compose up --build -d

down:
	docker-compose down

it:
	docker-compose exec app bash
