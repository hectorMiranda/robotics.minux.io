CC = gcc
CFLAGS = -Wall -Wextra -g
LDFLAGS = -lncurses -lmenu -lpanel

all: minux explorer

minux: minux.c
	$(CC) $(CFLAGS) -o minux minux.c $(LDFLAGS)

explorer: explorer.c
	$(CC) $(CFLAGS) -o explorer explorer.c $(LDFLAGS)

debug: CFLAGS += -DDEBUG
debug: all

clean:
	rm -f minux explorer *.o

.PHONY: all debug clean 