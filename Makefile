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
	@echo "$(YELLOW)Linking...$(NC)"
	$(CC) $(CFLAGS) $^ -o $@ $(LIBS)
	@echo "$(GREEN)Done!$(NC)"

%.o: %.c
	@echo "$(YELLOW)Compiling...$(NC)"
	$(CC) $(CFLAGS) -c $< -o $@
	@echo "$(GREEN)Done!$(NC)"

%.d: %.c
	@echo "$(YELLOW)Generating dependencies...$(NC)"
	$(CC) -MM $< -MT '$(subst .d,.o,$@)' > $@
	@echo "$(GREEN)Done!$(NC)"

-include $(DEP)

clean:
	@echo "$(YELLOW)Cleaning...$(NC)"
	$(RM) $(OBJ) $(DEP)
	@echo "$(GREEN)Done!$(NC)"

fclean: clean
	@echo "$(YELLOW)Removing executable and generated files...$(NC)"
	$(RM) $(EXEC)
	@echo "$(GREEN)Done!$(NC)"

re: fclean all

.PHONY: all clean fclean re
