# Compiler
CC = gcc

# Compiler Flags
CFLAGS = -Wall -Wextra -pedantic -std=c11

# Executables
EXECUTABLES = init kasjer przewodnik turysta

# Default Target
all: $(EXECUTABLES)

# Compile each source file into an executable
init: init.c myfun.c
	$(CC) $(CFLAGS) -o $@ $^

kasjer: kasjer.c myfun.c
	$(CC) $(CFLAGS) -o $@ $^

przewodnik: przewodnik.c myfun.c
	$(CC) $(CFLAGS) -o $@ $^

turysta: turysta.c myfun.c
	$(CC) $(CFLAGS) -o $@ $^

# Clean target to remove generated files
clean:
	rm -f $(EXECUTABLES)