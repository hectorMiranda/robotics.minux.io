CC = g++
CFLAGS = -Wall -Wextra -g

# Add include paths only if they exist
ifneq ($(wildcard /usr/include/libcamera),)
    CFLAGS += -I/usr/include/libcamera
endif

ifneq ($(wildcard /usr/include/pigpio),)
    CFLAGS += -I/usr/include/pigpio
endif

LDFLAGS = -lncurses -lmenu -lpanel

# Detect if we're on Raspberry Pi
ifeq ($(shell uname -m), armv7l)
    LDFLAGS += -lpigpio
endif

all: minux explorer

minux: minux.o error_console.o
	$(CC) $(CFLAGS) -o minux minux.o error_console.o $(LDFLAGS)

explorer: explorer.o
	$(CC) $(CFLAGS) -o explorer explorer.o $(LDFLAGS)

minux.o: minux.cpp error_console.h
	$(CC) $(CFLAGS) -c minux.cpp

explorer.o: explorer.cpp
	$(CC) $(CFLAGS) -c explorer.cpp

error_console.o: error_console.cpp error_console.h
	$(CC) $(CFLAGS) -c error_console.cpp

debug: CFLAGS += -DDEBUG
debug: all

clean:
	rm -f *.o minux explorer

.PHONY: all debug clean 