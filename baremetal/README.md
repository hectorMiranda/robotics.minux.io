# Baremetal Minux (BMM) v0.01 - Technical Documentation

## Architecture Overview

BMM is implemented as a single-threaded ncurses-based application utilizing a event-driven architecture with the following key components:

### Core Components

1. **Window Management System**
   - Utilizes ncurses panels for z-order management
   - Window hierarchy:
     ```
     root (stdscr)
     ├── menu_bar (1×width)
     ├── tab_bar (1×width)
     ├── main_content
     │   ├── file_panel (height×width/2)
     │   └── preview_panel (height×width/2)
     └── status_bar (1×width)
     ```

2. **Memory Management**
   - Dynamic allocation for file content buffers
   - Tab-based content management with cleanup handlers
   - Fixed-size buffers for paths and filenames
   ```c
   // Core memory constraints
   #define MAX_ITEMS 1024
   #define MAX_PATH 4096
   #define MAX_NAME_LENGTH 255
   #define MAX_TABS 10
   ```

3. **Event Loop Architecture**
   ```c
   while (1) {
       // 1. Update display
       draw_base_windows();
       draw_overlays();
       draw_active_menu();
       
       // 2. Input handling
       int ch = getch();
       if (ch == 27) {  // ESC/Alt handling
           handle_alt_combinations();
       } else {
           handle_normal_input(ch);
       }
   }
   ```

## Data Structures

### Tab Management
```c
typedef struct {
    char name[MAX_NAME_LENGTH];     // File basename
    char path[MAX_PATH];            // Full file path
    char *content;                  // Dynamically allocated content
    size_t content_size;           // Content buffer size
    int scroll_pos;                // Vertical scroll position
    int cursor_x;                  // Cursor column
    int cursor_y;                  // Cursor row
    int modified;                  // Modification flag
} Tab;

typedef struct {
    Tab tabs[MAX_TABS];            // Fixed-size tab array
    int count;                     // Active tab count
    int active;                    // Current tab index
} TabBar;
```

### Panel System
```c
typedef struct {
    WINDOW *win;                   // ncurses window pointer
    char items[MAX_ITEMS][MAX_NAME_LENGTH + 1];  // Directory entries
    int count;                     // Total items
    int selected;                  // Selected index
    int start;                     // Display start index
    int scroll_pos;               // Scroll position
} Panel;
```

### Menu System
```c
typedef struct MenuItem {
    char *label;                   // Menu item text
    char *shortcut;                // Keyboard shortcut
    void (*action)(void);         // Callback function
} MenuItem;

typedef struct Menu {
    char *name;                    // Menu name
    MenuItem *items;               // Array of items
    int count;                     // Item count
    int selected;                  // Selected index
    WINDOW *win;                   // Dropdown window
    int is_active;                // Active state
} Menu;
```

## Core Functions

### Window Management
```c
void draw_menu_bar(void)
- Renders top menu bar with underlined shortcuts
- Handles menu highlighting and selection state
- Complexity: O(1) - Fixed number of menus

void draw_panel(Panel *panel, int width, int height, int startx, int is_active)
- Renders file/directory listing with scrolling
- Handles color coding and selection highlighting
- Complexity: O(n) where n = visible items

void draw_file_content(WINDOW *win, Tab *tab)
- Renders file content with line wrapping
- Handles scroll position and cursor placement
- Complexity: O(m) where m = visible lines
```

### File Operations
```c
void load_directory(Panel *panel, const char *path)
- Reads directory entries with error handling
- Filters and sorts entries
- Complexity: O(n log n) where n = directory entries

void load_file_content(Tab *tab)
- Reads file content into dynamic buffer
- Handles large files and error conditions
- Complexity: O(n) where n = file size

int safe_path_join(char *dest, size_t dest_size, const char *path1, const char *path2)
- Safe path concatenation with bounds checking
- Returns -1 on potential buffer overflow
- Complexity: O(n) where n = path length
```

### Input Handling
```c
void handle_menu_input(int ch)
- Processes menu navigation and selection
- Handles keyboard shortcuts and Alt combinations
- Complexity: O(1)

void handle_mouse(void)
- Processes mouse events for all UI elements
- Translates coordinates to actions
- Complexity: O(n) where n = clickable elements
```

## Memory Management

### Allocation Strategy
1. **Static Allocation**
   - Fixed-size arrays for paths and names
   - Pre-allocated tab and panel structures
   ```c
   char current_path[MAX_PATH];
   Panel file_panel;
   TabBar tab_bar;
   ```

2. **Dynamic Allocation**
   - File content buffers
   - Menu dropdown windows
   ```c
   tab->content = malloc(tab->content_size + 1);
   menu->win = newwin(menu->count + 2, width, y, x);
   ```

3. **Cleanup Handlers**
   ```c
   // Tab cleanup
   for (int i = 0; i < tab_bar.count; i++) {
       free(tab_bar.tabs[i].content);
   }
   
   // Window cleanup
   delwin(menu_bar.win);
   delwin(status_bar);
   delwin(file_panel.win);
   delwin(preview_win);
   ```

## Error Handling

### Error Categories and Handling Mechanisms

1. **System Errors**
   ```c
   if (!dir) {
       show_status_message("Error: Cannot open directory", 1);
       return;
   }
   ```

2. **Buffer Overflows**
   ```c
   if (snprintf(full_path, sizeof(full_path), "%s/%s", path, entry->d_name) >= MAX_PATH) {
       continue;  // Skip if path would be too long
   }
   ```

3. **Memory Allocation**
   ```c
   tab->content = malloc(tab->content_size + 1);
   if (!tab->content) {
       show_status_message("Error: Memory allocation failed", 1);
       return;
   }
   ```

## Performance Considerations

1. **Display Updates**
   - Minimal screen updates using `touchwin()` and `wrefresh()`
   - Efficient scrolling with `start` index tracking
   - Window-specific updates rather than full screen refresh

2. **Memory Usage**
   - Fixed-size buffers to prevent fragmentation
   - Content loaded on-demand for large files
   - Proper cleanup of allocated resources

3. **Input Processing**
   - Non-blocking input for Alt key combinations
   - Efficient event handling with switch statements
   - Mouse event batching for smooth interaction

## Debugging

### Debug Build Configuration
```bash
gcc -g -DDEBUG -o bmm baremetal/baremetal.c -lncurses -lmenu -lpanel
```

### Debug Macros
```c
#ifdef DEBUG
#define DEBUG_LOG(fmt, ...) fprintf(stderr, fmt "\n", ##__VA_ARGS__)
#else
#define DEBUG_LOG(fmt, ...)
#endif
```

### Memory Analysis
```bash
valgrind --tool=memcheck --leak-check=full --show-leak-kinds=all ./bmm
```

### Core Dump Analysis
```bash
ulimit -c unlimited
gdb ./bmm core
```

## Build System Integration

### Makefile
```makefile
CC = gcc
CFLAGS = -Wall -Wextra -g
LDFLAGS = -lncurses -lmenu -lpanel

bmm: baremetal.o
    $(CC) $(CFLAGS) -o bmm baremetal.o $(LDFLAGS)

debug: CFLAGS += -DDEBUG
debug: bmm
```

## Testing

### Unit Test Framework
```c
// Test harness for safe_path_join
void test_safe_path_join(void) {
    char dest[MAX_PATH];
    assert(safe_path_join(dest, sizeof(dest), "/path", "file") > 0);
    assert(strcmp(dest, "/path/file") == 0);
}
```

## API Documentation

### Function Documentation Format
```c
/**
 * @brief Load directory contents into panel
 * @param panel Pointer to Panel structure
 * @param path Directory path to load
 * @return void
 * @note Thread-safety: Not thread-safe
 * @complexity O(n log n) where n = directory entries
 */
void load_directory(Panel *panel, const char *path);
```

This technical documentation provides a comprehensive overview of the implementation details, data structures, algorithms, and system architecture. For specific implementation details, refer to the source code and inline documentation.