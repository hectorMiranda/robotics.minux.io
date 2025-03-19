   CC = gcc
   CFLAGS = -Wall -Wextra -g -I/usr/include/libcamera -I/usr/include/pigpio
   LDFLAGS = -lncurses -lmenu -lpanel -lpigpio

   all: minux

   minux: minux.o
       $(CC) $(CFLAGS) -o minux minux.o $(LDFLAGS)

   minux.o: minux.c
       $(CC) $(CFLAGS) -c minux.c

   clean:
       rm -f minux minux.o