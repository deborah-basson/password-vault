# Compiler and flags
CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -I./include -I./ulh

# Source files
SOURCES = src/main.c src/password_manager.c src/security.c src/file_operations.c src/ui.c

# Object files
OBJECTS = $(SOURCES:.c=.o)

# Target executable
TARGET = password_manager

# Default target
all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJECTS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Clean target
clean:
	rm -f $(OBJECTS) $(TARGET)

.PHONY: all clean