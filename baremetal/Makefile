CXX = g++
CXXFLAGS = -Wall -Wextra -g -I.
LDFLAGS = -lncurses -lmenu -lpanel -lpthread -lrt

# Check for pigpio in multiple ways
HAS_PIGPIO = 0
PIGPIO_AVAILABLE = $(shell pkg-config --exists libpigpio 2>/dev/null && echo yes)
ifneq ($(PIGPIO_AVAILABLE),)
    HAS_PIGPIO = 1
else
    # Try direct library check as fallback
    PIGPIO_LIB = $(shell test -f /usr/lib/libpigpio.so && echo yes)
    ifneq ($(PIGPIO_LIB),)
        HAS_PIGPIO = 1
    endif
endif

# Set up appropriate flags based on pigpio availability
ifeq ($(HAS_PIGPIO),1)
    CXXFLAGS += -DHAS_PIGPIO=1
    LDFLAGS += -lpigpio
else
    CXXFLAGS += -DHAS_PIGPIO=0
endif

# Define targets
TARGETS = minux explorer

# Define source files for each target
MINUX_SOURCES = minux.cpp error_console.cpp
EXPLORER_SOURCES = explorer.cpp error_console.cpp

# Define object files
MINUX_OBJECTS = $(MINUX_SOURCES:.cpp=.o)
EXPLORER_OBJECTS = $(EXPLORER_SOURCES:.cpp=.o)

# Show configuration
all:
	@echo "Building with HAS_PIGPIO=$(HAS_PIGPIO)"
	@$(MAKE) build

# Actual build
build: $(TARGETS)

# Link the object files into the minux target
minux: $(MINUX_OBJECTS)
	$(CXX) -o $@ $^ $(LDFLAGS)

# Link the object files into the explorer target
explorer: $(EXPLORER_OBJECTS)
	$(CXX) -o $@ $^ $(LDFLAGS)

# Compile C++ source files
%.o: %.cpp
	$(CXX) -c $(CXXFLAGS) $< -o $@

# Clean up
clean:
	rm -f $(TARGETS) *.o

# Define dependencies
error_console.o: error_console.cpp error_console.h
minux.o: minux.cpp error_console.h
explorer.o: explorer.cpp error_console.h

.PHONY: all build clean 