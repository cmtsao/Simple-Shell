CC=gcc
CFLAGS=-g -pedantic -std=gnu17 -Wall -Werror -Wextra

.PHONY: all
all: simpleshell

simpleshell: simpleshell.o

simpleshell.o: simpleshell.c

.PHONY: clean
clean:
	rm -f *.o simpleshell