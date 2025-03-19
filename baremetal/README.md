# Baremetal Minux

A lightweight, ncurses-based file explorer with multi-tab support and modern features, designed for minimal resource usage and maximum efficiency.


## Installation and Usage

### Prerequisites

Before compiling and running the `minux` shell, ensure that you have the following packages installed on your Raspberry Pi:

1. **libcamera**: Required for camera functionality.
2. **ncurses**: Required for terminal UI.
3. **GPIO library**: Required for controlling GPIO pins.

You can install these packages using the following commands:

```bash
sudo apt-get update
sudo apt-get install libcamera-dev libncurses5-dev libncursesw5-dev pigpio
```

### Compiling the Program

To compile the `minux` shell, navigate to the project directory and run the following command:

```bash
make
```

This will compile the source code and generate the executable.

### Running the Program

Once compiled, you can run the `minux` shell with the following command:

```bash
./minux
```

### Testing the Camera

To test the camera functionality, enter the following command in the `minux` shell:

```bash
test camera
```

This command captures images using the Arducam camera and saves them in the `test_images` directory.

### Notes

- Ensure that the Arducam camera is properly connected to the Raspberry Pi.
- Captured images will be saved in the `test_images` directory within the current working directory.


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

## Error Console System

### DOOM-Style Error Terminal
The system features a retro-style error console inspired by classic DOS games:

1. **Activation**
   - Automatically appears when errors occur
   - Toggle with `~` key (like classic DOOM console)
   - Dismiss with `ESC` key
   - Error history persists between sessions

2. **Visual Style**
   - Full-screen overlay with semi-transparent background
   - DOS-style monospace font
   - Color-coded error levels:
     - RED: Critical errors
     - YELLOW: Warnings
     - WHITE: Information
     - GREEN: Success messages

3. **Features**
   - Scrollable error history
   - Timestamp for each error
   - Error categorization
   - Command history
   - Error source tracking
   - Stack trace for critical errors

4. **Error Categories**
   ```c
   typedef enum {
       ERROR_CRITICAL,    // System-level failures
       ERROR_WARNING,     // Non-fatal warnings
       ERROR_INFO,        // Information messages
       ERROR_SUCCESS      // Success confirmations
   } ErrorLevel;
   ```

5. **Error Format**
   ```
   [TIMESTAMP] <ERROR_LEVEL> [SOURCE] Message
   Example:
   [2024-03-14 15:23:45] <CRITICAL> [EXPLORER] Failed to open file: Permission denied
   Stack trace:
   -> main() at minux.c:245
   -> launch_explorer() at minux.c:167
   -> open_file() at explorer.c:89
   ```

6. **Integration**
   - Errors logged to both console and file
   - Persistent error log in ~/.minux/error.log
   - Real-time console updates
   - System status monitoring

7. **Commands**
   - `clear` - Clear console history
   - `save` - Save console buffer to file
   - `filter` - Filter by error level
   - `search` - Search error messages
   - `help` - Show console commands

### Status Bar Integration
The Windows 95-style status bar provides:
- Current system status
- Last error message preview
- Error count indicator
- Quick access to error console


## Camera Functionality

The `minux` shell now supports controlling the Arducam Day-Night Vision camera. You can capture images in both daylight and night vision modes using the following command:

### Command: `test camera`

This command captures images using the Arducam camera and saves them in the `test_images` directory. The process includes:

1. **Creating a Directory**: A folder named `test_images` is created to store the captured images.
2. **Capturing Daylight Image**: The camera captures an image in daylight mode and saves it as `daylight.jpg`.
3. **Simulating Night Mode**: The IR-Cut filter is disabled to activate night vision mode, and the camera captures an image saved as `night_vision.jpg`.
4. **Restoring Day Mode**: The IR-Cut filter is re-enabled, and another image is captured and saved as `restored_daylight.jpg`.

### Usage

To test the camera, simply enter the following command in the `minux` shell:

```bash
test camera
```

### Requirements

- Ensure that the Arducam camera is properly connected to the Raspberry Pi.
- The `libcamera` library must be installed on your system. You can install it using:

```bash
sudo apt-get install libcamera-dev
```

### Notes

- The command also controls GPIO pins to manage the IR-Cut filter for switching between day and night modes.
- Captured images will be saved in the `test_images` directory within the current working directory.

... existing content ...