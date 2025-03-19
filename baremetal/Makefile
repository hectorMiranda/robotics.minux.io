CC = gcc
CFLAGS = -Wall -Wextra -g
LDFLAGS = -lncurses -lmenu -lpanel

all: explorer

explorer: explorer.o
	$(CC) $(CFLAGS) -o explorer explorer.o $(LDFLAGS)

explorer.o: explorer.c
	$(CC) $(CFLAGS) -c explorer.c

clean:
	rm -f explorer explorer.o

.PHONY: all debug clean 