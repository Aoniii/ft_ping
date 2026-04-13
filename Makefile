-include ./parser/source.mk
-include ./source.mk

NAME		=	ft_ping

CC			=	gcc
CFLAGS		=	-Wall -Wextra -Werror

SRCS		=	$(addprefix parser/, $(PARSER_SRCS)) $(MAIN_SRCS)
OBJS		=	$(SRCS:.c=.o)
OBJS_DIR 	=	objects
OBJS_PATH	=	$(addprefix $(OBJS_DIR)/, $(OBJS))

RED			=	\033[1;31m
GREEN		=	\033[1;32m
YELLOW		=	\033[1;33m
BLUE		=	\033[1;34m
CYAN		=	\033[1;36m
RESET		=	\033[0m
UP			=	\033[A
CUT			=	\033[K

TOTAL_FILES	=	$(words $(SRCS))

$(OBJS_DIR)/%.o: %.c
	@mkdir -p $(@D)
	@if [ ! -f .count ]; then echo 0 > .count; fi
	@COUNT=$$(($$(cat .count) + 1)); \
	echo $$COUNT > .count; \
	PERCENT=$$(($$COUNT * 100 / $(TOTAL_FILES))); \
	printf "$(CUT)$(RESET)[$(YELLOW)%3d%%$(RESET)] 🛰️ $(CYAN)Sending packets to: %s...\n$(RESET)" $$PERCENT $<
	@$(CC) $(CFLAGS) -c $< -o $@
	@printf "$(UP)"

$(NAME): $(OBJS_PATH)
	@printf "$(RESET)[$(GREEN)DONE$(RESET)] 📦 $(CYAN)Echo reply: All objects received!$(RESET)\n"
	@$(CC) $(CFLAGS) $(OBJS_PATH) -o $(NAME) -lm
	@sudo chown root:root $(NAME)
	@sudo chmod u+s $(NAME)
	@rm -f .count
	@printf "$(RESET)[$(GREEN)DONE$(RESET)] 🚀 $(BLUE)$(NAME) is ready to fly!$(RESET)\n"

all:
	@if $(MAKE) -q $(NAME) --no-print-directory; then \
		printf "$(RESET)[$(GREEN)DONE$(RESET)] 📡 $(CYAN)Connection stable: $(NAME) is already at its peak!$(RESET)\n"; \
	else \
		$(MAKE) $(NAME) --no-print-directory; \
	fi

clean:
	@rm -rf $(OBJS_DIR)
	@rm -f .count
	@printf "$(RESET)[$(GREEN)DONE$(RESET)] 🧹 $(YELLOW)TTL expired: Cleaning objects...$(RESET)\n"

fclean: clean
	@rm -f $(NAME)
	@rm -f .count
	@printf "$(RESET)[$(GREEN)DONE$(RESET)] 🚫 $(RED)Destination unreachable: $(NAME) deleted.$(RESET)\n"

re: fclean all

valgrind: $(OBJS_PATH)
	@printf "$(RESET)[$(GREEN)DONE$(RESET)] 📦 $(CYAN)Echo reply: All objects received!$(RESET)\n"
	@$(CC) $(CFLAGS) $(OBJS_PATH) -o $(NAME) -lm
	@rm -f .count
	@printf "$(RESET)[$(GREEN)DONE$(RESET)] 🚀 $(BLUE)$(NAME) is ready to fly!$(RESET)\n"

.PHONY: all clean fclean re valgrind
