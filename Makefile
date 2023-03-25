
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
			history.c\
			release.c\
			assert.c\


SRCS		:=	$(FILES:%.c=$(SRCDIR)/%.c)
OBJS		:=	$(FILES:%.c=$(OBJDIR)/%.o)
LIB_NAME	:=	libmalloc.a
LIBFT		:=	libft.a
LIBFT_DIR	:=	libft
RM			:=	rm -rf

TEST_NAME	:=	malloc
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
CCOREFLAGS	=	-Wall -Wextra -Werror -O2 -I$(INCDIR)
CFLAGS		=	$(CCOREFLAGS) -DNDEBUG -g #-fsanitize=thread
LIBFLAGS	:=	-fPIC -fpic

ifdef WITH_BONUS
CCOREFLAGS += -D BONUS
endif

BASE_LIBNAME	:=	ft_malloc
SONAME		:=	libft_malloc_$(HOSTTYPE).so
BASE_SONAME	:=	libft_malloc.so
DYLIBNAME	:=	libft_malloc_$(HOSTTYPE).dylib
BASE_DYLIBNAME	:=	libft_malloc.dylib

all:			so

bonus:
	$(MAKE) all WITH_BONUS=1

$(OBJDIR)/%.o:	$(SRCDIR)/%.c
	@mkdir -p $(OBJDIR)
	$(CC) $(CFLAGS) $(LIBFLAGS) -c $< -o $@

$(OBJDIR)/%.o:	%.c
	@mkdir -p $(OBJDIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(LIB_NAME):	$(OBJS)
	ar rcs $(LIB_NAME) $(OBJS)

$(TEST_NAME):	$(LIB_NAME) $(OBJS_TEST) $(LIBFT) #$(BASE_DYLIBNAME)
	$(CC) $(CFLAGS) -o $@ $(OBJS_TEST) $(LIBFT) $(LIB_NAME)
	./$@

.PHONY:	t
t:		$(TEST_NAME)
	./$^


.PHONY:			so
so:				$(BASE_SONAME)

$(SONAME):		$(OBJS)
	$(CC) -shared -fPIC $^ -o $@

$(BASE_SONAME):	$(SONAME)
	rm -f $@
	ln -s $^ $@

.PHONY:			dylib
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


.PHONY:	clean
clean:
	$(RM) $(OBJDIR) $(LIBFT)

.PHONY:	fclean
fclean:			clean
	$(RM) $(LIB_NAME) $(SONAME) $(DYLIBNAME) $(BASE_SONAME) $(BASE_DYLIBNAME)
	$(MAKE) -C $(LIBFT_DIR) fclean

.PHONY:	re
re:				fclean all


.PHONY:	up
up:
	docker-compose up --build -d

.PHONY:	down
down:
	docker-compose down

.PHONY:	it
it:
	docker-compose exec app bash
