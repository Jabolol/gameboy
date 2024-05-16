CC = gcc
CFLAGS = -Wall -Wextra
LIBS = -lSDL2_ttf -lSDL2
SRCDIR = sources
SRC = $(wildcard $(SRCDIR)/*.c)
OBJ = $(SRC:.c=.o)
DEP = $(SRC:.c=.d)
EXEC = gameboy

RED = \033[0;31m
CYAN = \033[0;36m
GREEN = \033[0;32m
YELLOW = \033[0;33m
NC = \033[0m

all: $(EXEC)

$(EXEC): $(OBJ)
	@$(CC) $(CFLAGS) $^ -o $@ $(LIBS)

%.o: %.c
	@$(CC) $(CFLAGS) -c $< -o $@

%.o: %.c
	@echo "$(CYAN)$<$(NC) ==> $(GREEN)$@$(NC)"
	@$(CC) $(CFLAGS) -c $< -o $@

clean:
	@echo "$(RED)Cleaning object files and dependencies$(NC)"
	@$(RM) $(OBJ) $(DEP)

fclean: clean
	@$(RM) $(EXEC)

re: fclean all

.PHONY: all clean fclean re
