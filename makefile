CC = gcc
# CFLAGS = -Wall -Wextra -g
TARGET = tarsau

.PHONY: all clean

all: $(TARGET)

$(TARGET): tarsau.c
	$(CC) $< -o $@

clean:
	rm -f $(TARGET)
