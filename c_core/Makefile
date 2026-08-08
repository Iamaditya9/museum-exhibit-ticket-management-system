CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -pedantic -g

TARGET = ../museum_core
SOURCES = main.c museum.c

all: $(TARGET)

$(TARGET): $(SOURCES) museum.h
	$(CC) $(CFLAGS) $(SOURCES) -o $(TARGET)

clean:
	rm -f $(TARGET)

run: $(TARGET)
	cd .. && ./museum_core

.PHONY: all clean run