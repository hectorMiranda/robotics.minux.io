# Baremetal Minux (BMM) v0.01

A lightweight terminal-based file explorer and text editor built with ncurses, designed for efficiency and keyboard-centric operation.

## Features

- File preview functionality
- Directory navigation and file management


## Dependencies

Required libraries:
```bash
libncurses6 (>= 6.0)
libmenu6
libpanel6
```

On Ubuntu/Debian systems, install dependencies with:
```bash
sudo apt-get install libncurses6 libncurses-dev libmenu-dev libpanel-dev
```

## Building

Compile the program using gcc:
```bash
gcc -o bmm baremetal/baremetal.c -lncurses -lmenu -lpanel
```

## Usage

### Launch
```bash
./bmm
```

### Keyboard Shortcuts

#### Menu Navigation
- `Alt+F` - File menu
- `Alt+E` - Edit menu
- `Alt+V` - View menu
- `Alt+H` - Help menu
- `↑/↓` - Navigate menu items
- `Enter` - Select menu item
- `Esc` - Close menu

#### File Operations
- `Ctrl+S` - Save current file
- `Ctrl+W` - Close current tab
- `Ctrl+H` - Toggle hidden files
- `Tab` - Switch between open tabs
- `Enter` - Open file/directory
- `Q` - Quit program

### Mouse Support
- Click on tabs to switch between them
- Click on menu items to open menus
- Click on files/directories to select them

## Program Structure

### Windows and Panels
1. Menu Bar (top)
2. Tab Bar (below menu)
3. File Explorer Panel (left)
4. Preview/Edit Panel (right)
5. Status Bar (bottom)

### Key Components

#### Panel System
- File explorer panel shows current directory contents
- Preview/edit panel displays file contents
- Color-coded display for directories and files
- Scrollable content with proper cursor management

#### Tab Management
- Multiple file editing support
- Tab-based interface with visual indicators
- Modified file indicators (*)
- Maximum 10 concurrent tabs

#### Menu System
1. File Menu
   - New Tab
   - Open
   - Save
   - Close Tab
   - Exit

2. Edit Menu
   - Cut
   - Copy
   - Paste

3. View Menu
   - Toggle Hidden Files
   - Word Wrap

4. Help Menu
   - Keyboard Shortcuts
   - About

## Error Handling

The program handles various error conditions:

1. Path Length Errors
   - Maximum path length: 4096 characters
   - Error message when path exceeds limit

2. File Operations
   - Directory access errors
   - File open/save failures
   - Tab limit exceeded warnings

3. Memory Management
   - Proper allocation/deallocation of file content
   - Tab content cleanup on closure

## Status Messages

Status messages appear in the bottom bar with different color coding:
- Normal: White text
- Error: Red text with bold
- Info: Yellow text

Messages timeout after 3 seconds.

## Technical Details

### Constants
```c
#define MAX_ITEMS 1024      // Maximum items in directory
#define MAX_PATH 4096       // Maximum path length
#define MAX_NAME_LENGTH 255 // Maximum filename length
#define MAX_TABS 10         // Maximum open tabs
```

### Data Structures

#### Tab Structure
```c
typedef struct {
    char name[MAX_NAME_LENGTH];
    char path[MAX_PATH];
    char *content;
    size_t content_size;
    int scroll_pos;
    int cursor_x;
    int cursor_y;
    int modified;
} Tab;
```

#### Panel Structure
```c
typedef struct {
    WINDOW *win;
    char items[MAX_ITEMS][MAX_NAME_LENGTH + 1];
    int count;
    int selected;
    int start;
    int scroll_pos;
} Panel;
```

## Known Issues

1. Long filenames may be truncated in the display
2. No Unicode support in current version
3. Limited undo/redo functionality
4. Copy/paste operations not fully implemented

## Future Enhancements

Planned features for future releases:
1. Full text editing capabilities
2. Search and replace functionality
3. Syntax highlighting
4. Unicode support
5. Configuration file support
6. Customizable keybindings
7. Split-screen editing
8. File type associations

## Debugging

For debugging purposes, the program can be run with various terminal emulators:
```bash
# Standard terminal
./bmm

# Debug output to file
./bmm 2> debug.log

# With valgrind for memory analysis
valgrind --leak-check=full ./bmm
```

## Contributing

When contributing to this project:
1. Follow the existing code style
2. Document new functions and features
3. Update the README with any new features or changes
4. Test thoroughly before submitting changes

## License

This program is open-source software. See LICENSE file for details.