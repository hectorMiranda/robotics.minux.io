# Baremetal File Explorer

A lightweight, ncurses-based file explorer with multi-tab support and modern features, designed for minimal resource usage and maximum efficiency.

## Technical Overview

### Core Components

1. **Window Management**
   - Utilizes ncurses panels and windows for UI rendering
   - Main components: menu bar, file panel, preview panel, status bar, tab bar
   - Custom window buffering for efficient screen updates

2. **Data Structures**
   ```c
   typedef struct {
       char name[MAX_NAME_LENGTH];
       char path[MAX_PATH];
       char *content;
       size_t content_size;
       int scroll_pos;
       int cursor_x, cursor_y;
       int modified;
   } Tab;

   typedef struct {
       WINDOW *win;
       char items[MAX_ITEMS][MAX_NAME_LENGTH + 1];
       int count;
       int selected;
       int start;
       int scroll_pos;
   } Panel;
   ```

3. **Memory Management**
   - Dynamic content allocation for file buffers
   - Automatic cleanup on tab closure
   - Buffer size limits: MAX_ITEMS (1024), MAX_PATH (4096)

### Key Features

1. **File Operations**
   - Safe path joining with buffer overflow protection
   - Directory traversal with error handling
   - File content preview with memory-mapped loading
   - Tab-based file editing

2. **UI Components**
   - Drop-down menus with keyboard shortcuts
   - Color-coded file types
   - Status messages with timeout
   - Mouse support for navigation

3. **Menu System**
   - Alt key shortcuts (Alt+F, Alt+E, Alt+V, Alt+H)
   - Context-aware menu items
   - Keyboard navigation in dropdowns
   - Customizable actions per menu item

### Error Handling

1. **Path Operations**
   - Buffer overflow prevention
   - Directory access verification
   - Path length validation
   - Safe string operations

2. **Memory Operations**
   - NULL pointer checks
   - Memory allocation verification
   - Proper resource cleanup
   - Buffer size validation

## Build Requirements

### Dependencies
- ncurses 6.0+
- libmenu-dev (Ubuntu/Debian)
- libpanel-dev
- GCC with C11 support

### Installation on Ubuntu/Debian
```bash
sudo apt-get update
sudo apt-get install libncurses5-dev libncursesw5-dev
```

### Installation on Raspberry Pi
```bash
sudo apt-get update
sudo apt-get install libncurses5-dev libncursesw5-dev
```

### Compilation
```bash
make
```

## Technical Usage

### Keyboard Shortcuts
- `Alt+F`: File menu
- `Alt+E`: Edit menu
- `Alt+V`: View menu
- `Alt+H`: Help menu
- `Ctrl+S`: Save file
- `Ctrl+W`: Close tab
- `Tab`: Switch between tabs
- `Enter`: Open file/directory
- `Q`: Quit application

### Program Flow

1. **Initialization**
   ```c
   initscr();
   raw();
   noecho();
   keypad(stdscr, TRUE);
   mousemask(ALL_MOUSE_EVENTS, NULL);
   init_colors();
   ```

2. **Main Event Loop**
   - Window drawing cycle
   - Input handling
   - Menu management
   - File operations

3. **Cleanup**
   - Memory deallocation
   - Window destruction
   - ncurses shutdown

### Error Logging

Status messages are displayed in the status bar with three severity levels:
- Normal: Standard operations
- Error (Red): Operation failures
- Info (Yellow): Status updates

## Performance Considerations

1. **Memory Usage**
   - Fixed-size buffers for predictable memory usage
   - Content loading on demand
   - Efficient string handling

2. **Display Updates**
   - Selective screen refreshes
   - Double-buffered drawing
   - Optimized window updates

3. **Input Handling**
   - Non-blocking input for Alt key detection
   - Efficient event processing
   - Debounced mouse events

## Debugging

Debug builds can be created using:
```bash
make debug
```

This enables:
- Additional logging
- Memory tracking
- Performance metrics
- Assert statements

## Known Limitations

1. Maximum file size: Limited by available memory
2. Maximum path length: 4096 characters
3. Maximum items per directory: 1024
4. Maximum open tabs: 10

## Future Enhancements

1. Async file loading
2. Plugin system
3. Custom keybinding support
4. Search functionality
5. Split-pane support