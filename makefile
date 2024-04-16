NAME 	=	cpnv
SRC		=	get_next_line.c cpnv.c
OBJ		=	$(SRC:.c=.o)
CC		=	gcc
RM		=	rm -f

GREEN       =   \e[38;5;118m
YELLOW      =   \e[38;5;226m
RESET       =   \e[0m
_SUCCESS    =   [$(GREEN)SUCCESS$(RESET)]
_INFO       =   [$(YELLOW)INFO$(RESET)]

all:	$(NAME)

%.o: %.c
	@$(CC) -Wall -Wextra -Werror -c $< -o $@
	@printf "$(_INFO) OBJ $@ compiled.\n"

$(NAME): $(LIBFT) $(OBJ)
	@$(CC) -g $(OBJ) -D BUFFER_SIZE=32 -o $(NAME)
	@printf "$(_SUCCESS) $(NAME) ready.\n"
clean:
	@ $(RM) $(OBJ)
	@printf "$(_INFO) OBJ removed.\n"

fclean: clean
	@ $(RM) $(NAME)
	@printf "$(_INFO) $(NAME) removed.\n"

re: fclean all

.PHONY: all clean fclean re
