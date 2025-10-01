SRC = src/main.c
CC = gcc
TAR = main

$(TAR): $(SRC)
	@echo "executing the code..."
	$(CC) $< -o $@

run:
	@echo "Running..."
	./$(TAR)

clean:
	@echo "deleting executable...";
	rm -rf $(TAR)

.PHONY: run clean


