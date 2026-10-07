CC := gcc
CFLAGS := -Isrc -I. -O2 -Wall -Wextra -Werror -Wreturn-type -Wno-unused-variable -Wno-unused-but-set-variable -Wno-unused-parameter -pedantic -std=c11 -Wvla -Isrc -I/opt/homebrew/include -I/opt/homebrew/include/notcurses
LDFLAGS := -L/opt/homebrew/lib
LDLIBS := -lnotcurses -lnotcurses-core -lm

# Point to your source directory
SRC_DIR := src

# Automatically grab all .c files inside src/ (main.c, csv.c, map.c, etc.)
SRCS = $(shell find $(SRC_DIR) -name '*.c')

# Convert all src/*.c paths to src/*.o
OBJECTS := $(SRCS:.c=.o)

TARGET := main

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) $(LDFLAGS) $(OBJECTS) $(LDLIBS) -o $(TARGET)

# Rule to compile src/*.c files into src/*.o
$(SRC_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

.PHONY: clean all
clean:
	$(RM) $(OBJECTS) $(TARGET)

.PHONY: format all
format:
	clang-format -i $(shell find $(SRC_DIR) -name '*.[ch]')