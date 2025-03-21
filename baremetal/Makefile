CC = gcc
CXX = g++
CFLAGS = -Wall -Wextra -g -I/usr/include/libcamera -I/usr/include/pigpio
CXXFLAGS = -Wall -Wextra -g -I/usr/include/libcamera -I/usr/include/pigpio
LDFLAGS = -lncurses -lmenu -lpanel -lpigpio -lpthread -lrt -lcamera

# Object files
OBJS = minux.o error_console.o

# Default target
all: minux

# Compile C files
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Compile C++ files
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Link target
minux: $(OBJS)
	$(CXX) -o $@ $^ $(LDFLAGS)

# Clean target
clean:
	rm -f $(OBJS) minux

# Debug target
debug: CFLAGS += -DDEBUG
debug: CXXFLAGS += -DDEBUG
debug: all

.PHONY: all clean debug 