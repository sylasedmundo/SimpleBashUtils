CC = gcc
CFLAGS = -Wall -Werror -Wextra -std=c11 -I

SOURCES = s21_grep.c
OBJECTS = $(SOURCES:.c=.o)
TARGET = s21_grep

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(OBJECTS) -o $@

%.o: %.c
	$(CC) $(CFLAGS) -I -c $< -o $@

clean:
	rm -f $(OBJECTS) $(TARGET)

rebuild: clean all

run: $(TARGET)
	./$(TARGET)

.PHONY: all clean rebuild run