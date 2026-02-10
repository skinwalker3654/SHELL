SRC = src/main.c src/shell.c
CC = gcc
TAR = main

$(TAR): $(SRC)
	@echo "executing the code..."
	$(CC) $^ -o $@ -lm

run:
	@echo "Running..."
	./$(TAR)

clean:
	@echo "deleting executable..."
	rm -rf data.txt
	rm -rf $(TAR)

.PHONY: run clean
