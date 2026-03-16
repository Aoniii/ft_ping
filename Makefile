-include ./parser/source.mk
-include ./source.mk

NAME		=	ft_ping

SRCS		=	$(addprefix parser/, $(PARSER_SRCS))
SRCS		+=	$(MAIN_SRCS)

OBJS		=	$(SRCS:.c=.o)
OBJS_DIR 	=	objects
OBJS_PATH	=	$(addprefix $(OBJS_DIR)/, $(OBJS))

CC			=	gcc
CFLAGS		=	-Wall -Wextra -Werror

$(OBJS_DIR)/%.o: %.c
	mkdir -p $(@D)
	$(CC) $(CFLAGS) -c $< -o $@

$(NAME): $(OBJS_PATH)
	$(CC) $(CFLAGS) $(OBJS_PATH) -o $(NAME)

all: $(NAME)

clean:
	rm -rf $(OBJS_DIR)

fclean: clean
	rm -f $(NAME)

re: fclean all

.PHONY: all clean fclean re
