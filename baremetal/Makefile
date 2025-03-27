CXX = g++
CXXFLAGS = -Wall -Wextra -g -I. -std=c++11
# Detect if we're on Linux
UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Linux)
    LDFLAGS = -lncurses -lmenu -lpanel -lpthread -lrt -lsecp256k1 -lcrypto
else
    LDFLAGS = -lncurses -lmenu -lpanel -lpthread -lsecp256k1 -lcrypto
endif

# Improved Raspberry Pi detection
IS_PI = 0

# First check directly for Raspberry Pi mention in model file
PI_MODEL_CHECK = $(shell cat /sys/firmware/devicetree/base/model 2>/dev/null | grep -q "Raspberry Pi" && echo yes)
ifneq ($(PI_MODEL_CHECK),)
    IS_PI = 1
endif

# Second, check the CPU info for BCM or Raspberry
ifeq ($(IS_PI),0)
    PI_CPU_CHECK = $(shell grep -q -E "BCM|Raspberry" /proc/cpuinfo 2>/dev/null && echo yes)
    ifneq ($(PI_CPU_CHECK),)
        IS_PI = 1
    endif
endif

# Third, check /dev/gpiomem which exists on Raspberry Pi
ifeq ($(IS_PI),0)
    PI_GPIO_CHECK = $(shell test -e /dev/gpiomem && echo yes)
    ifneq ($(PI_GPIO_CHECK),)
        IS_PI = 1
    endif
endif

# Manual override - uncomment if needed
# For Ubuntu development: leave commented
# For Raspberry Pi if detection fails: uncomment
# IS_PI = 1

# Check for pigpio header and library
HAS_PIGPIO = 0
PIGPIO_HEADER = $(shell test -f /usr/include/pigpio.h && echo yes)

ifneq ($(PIGPIO_HEADER),)
    # Check for pigpio library only if header exists
    PIGPIO_LIB = $(shell test -f /usr/lib/libpigpio.so -o -f /usr/lib/*/libpigpio.so && echo yes)
    ifneq ($(PIGPIO_LIB),)
        HAS_PIGPIO = 1
    endif
endif

# On Raspberry Pi, we prefer to have pigpio, but don't force it if not installed
ifeq ($(IS_PI),1)
    ifeq ($(HAS_PIGPIO),1)
        $(info Raspberry Pi detected with pigpio library - enabling pigpio support)
    else
        $(info Raspberry Pi detected but pigpio library not found - building without pigpio)
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

# Show detailed configuration information
all:
	@echo "--- Build Configuration ---"
	@if [ "$(IS_PI)" = "1" ]; then \
		echo "Raspberry Pi detection: $(IS_PI)"; \
	else \
		echo "Building for non-Raspberry Pi platform (like Ubuntu)"; \
	fi
	@echo "PIGPIO support: $(HAS_PIGPIO)"
	@if [ "$(IS_PI)" = "1" ] && [ "$(HAS_PIGPIO)" = "0" ]; then \
		echo "NOTE: Building without pigpio on Raspberry Pi."; \
		echo "To add hardware GPIO support: sudo apt-get install libpigpio-dev"; \
		echo "For Pi 5, you may need newer alternatives like libgpiod-dev and gpiod"; \
	fi
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