CXX = g++
CXXFLAGS = -Wall -Wextra -g -I.
LDFLAGS = -lncurses -lmenu -lpanel -lpthread -lrt

# Optional libraries - only link if available
OPTIONAL_LIBS = 
ifneq ($(shell pkg-config --exists libpigpio && echo yes),)
    OPTIONAL_LIBS += -lpigpio
endif

# Define targets
TARGETS = minux explorer

# Define source files for each target
MINUX_SOURCES = minux.cpp error_console.cpp
EXPLORER_SOURCES = explorer.cpp error_console.cpp

# Define object files
MINUX_OBJECTS = $(MINUX_SOURCES:.cpp=.o)
EXPLORER_OBJECTS = $(EXPLORER_SOURCES:.cpp=.o)

# Define the rule to build all targets
all: $(TARGETS)

# Link the object files into the minux target
minux: $(MINUX_OBJECTS)
	$(CXX) -o $@ $^ $(LDFLAGS) $(OPTIONAL_LIBS)

# Link the object files into the explorer target
explorer: $(EXPLORER_OBJECTS)
	$(CXX) -o $@ $^ $(LDFLAGS) $(OPTIONAL_LIBS)

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

.PHONY: all clean 