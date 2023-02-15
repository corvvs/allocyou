
CC		:= gcc
CFLAGS	:= -Wall -Wextra -Werror -g -fsanitize=address

SRCDIR	:= 
OBJDIR	:= 
SRCS	:=\
			yo_malloc.c\
			yo_list.c\
			yo_allocation.c\
			yo_realloc.c\
			yo_utils.c\

OBJS	:= $(SRCS:$(SRCDIR)%.c=$(OBJDIR)%.o)
NAME	:= libmalloc.a
RM		:= rm -rf

all:	malloc


.PHONY:	malloc
malloc:	$(NAME)
	$(CC) $(CFLAGS) -o $@ main.c $(NAME)
	./$@


$(NAME):	$(OBJS)
	ar rcs $(NAME) $(OBJS)

clean:
	$(RM)	$(OBJS)

fclean:	clean
	$(RM)	$(NAME)

re:		fclean all
