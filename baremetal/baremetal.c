#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ncurses.h>
#include <menu.h>
#include <panel.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <ctype.h>
#include <time.h>

#define MAX_ITEMS 1024
#define MAX_PATH 4096
#define MAX_NAME_LENGTH 255
#define MAX_TABS 10
#define VERSION "v0.01"
#define STATUS_MESSAGE_TIMEOUT 3000  // 3 seconds in milliseconds
#define KEY_ALT_F 6 // Alt+F
#define KEY_ALT_E 5 // Alt+E
#define KEY_ALT_V 22 // Alt+V
#define KEY_ALT_H 8 // Alt+H
#define KEY_ESC 27
#define KEY_CTRL(x) ((x) & 0x1F)
#define KEY_ALT(x) (0x200 + (x))

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

typedef struct {
    Tab tabs[MAX_TABS];
    int count;
    int active;
} TabBar;

typedef struct {
    WINDOW *win;
    char items[MAX_ITEMS][MAX_NAME_LENGTH + 1];
    int count;
    int selected;
    int start;
    int scroll_pos;
} Panel;

typedef struct {
    WINDOW *win;
    char *items[10];
    int count;
    int selected;
} MenuBar;

typedef struct MenuItem {
    char *label;
    char *shortcut;
    void (*action)(void);
} MenuItem;

typedef struct Menu {
    char *name;
    MenuItem *items;
    int count;
    int selected;
    WINDOW *win;
    int is_active;
} Menu;

// Global state
TabBar tab_bar;
Panel file_panel;
MenuBar menu_bar;
WINDOW *status_bar;
WINDOW *preview_win;
int screen_width, screen_height;
char status_message[256] = "";
int status_message_type = 0;  // 0: normal, 1: error, 2: info
clock_t status_message_time = 0;
Menu menus[4];  // File, Edit, View, Help
int active_menu = -1;
char current_path[MAX_PATH];  // Move current_path to global scope

// Forward declarations
void show_status_message(const char *message, int type);
void load_directory(Panel *panel, const char *path);

int safe_path_join(char *dest, size_t dest_size, const char *path1, const char *path2) {
    // First try with snprintf to get the required length
    int required_len = snprintf(NULL, 0, "%s/%s", path1, path2);
    if (required_len < 0) return -1;  // snprintf error
    
    // Check if we have enough space including null terminator
    if ((size_t)required_len >= dest_size) return -1;  // would truncate
    
    // Now do the actual concatenation
    return snprintf(dest, dest_size, "%s/%s", path1, path2);
}

void init_colors() {
    start_color();
    init_pair(1, COLOR_CYAN, COLOR_BLACK);     // Directory color
    init_pair(2, COLOR_WHITE, COLOR_BLACK);    // File color
    init_pair(3, COLOR_BLACK, COLOR_CYAN);     // Selected item
    init_pair(4, COLOR_YELLOW, COLOR_BLACK);   // Preview text
    init_pair(5, COLOR_BLACK, COLOR_WHITE);    // Menu bar
    init_pair(6, COLOR_WHITE, COLOR_BLUE);     // Status bar
    init_pair(7, COLOR_BLACK, COLOR_GREEN);    // Active tab
    init_pair(8, COLOR_WHITE, COLOR_BLACK);    // Inactive tab
}

void draw_menu_bar() {
    wattron(menu_bar.win, COLOR_PAIR(5));
    mvwhline(menu_bar.win, 0, 0, ' ', screen_width);
    
    int x = 2;
    const char *items[] = {"File", "Edit", "View", "Help"};
    for (int i = 0; i < 4; i++) {
        if (i == menu_bar.selected || i == active_menu)
            wattron(menu_bar.win, A_REVERSE);
        
        // Draw menu item with highlighted shortcut letter
        mvwprintw(menu_bar.win, 0, x, " ");
        wattron(menu_bar.win, A_UNDERLINE);
        waddch(menu_bar.win, items[i][0]);
        wattroff(menu_bar.win, A_UNDERLINE);
        wprintw(menu_bar.win, "%s ", items[i] + 1);
        
        if (i == menu_bar.selected || i == active_menu)
            wattroff(menu_bar.win, A_REVERSE);
        x += strlen(items[i]) + 3;
    }
    wattroff(menu_bar.win, COLOR_PAIR(5));
    wrefresh(menu_bar.win);
}

void draw_status_bar(const char *path) {
    wattron(status_bar, COLOR_PAIR(6));
    mvwhline(status_bar, 0, 0, ' ', screen_width);

    // Show current path on the left
    mvwprintw(status_bar, 0, 1, " %s ", path);

    // Show cursor position on the right
    if (tab_bar.active >= 0) {
        mvwprintw(status_bar, 0, screen_width - 20, "Line %d, Col %d", 
                  tab_bar.tabs[tab_bar.active].cursor_y + 1,
                  tab_bar.tabs[tab_bar.active].cursor_x + 1);
    }

    // Show status message if it exists and hasn't timed out
    if (status_message[0] != '\0') {
        clock_t current_time = clock();
        if ((current_time - status_message_time) * 1000 / CLOCKS_PER_SEC < STATUS_MESSAGE_TIMEOUT) {
            // Calculate center position for message
            int msg_len = strlen(status_message);
            int msg_x = (screen_width - msg_len) / 2;
            
            // Change color based on message type
            if (status_message_type == 1) {  // Error
                wattron(status_bar, COLOR_PAIR(1) | A_BOLD);
            } else if (status_message_type == 2) {  // Info
                wattron(status_bar, COLOR_PAIR(4));
            }
            
            mvwprintw(status_bar, 0, msg_x, "%s", status_message);
            
            if (status_message_type > 0) {
                wattroff(status_bar, COLOR_PAIR(status_message_type == 1 ? 1 : 4) | 
                        (status_message_type == 1 ? A_BOLD : 0));
            }
        } else {
            status_message[0] = '\0';  // Clear the message after timeout
        }
    }

    wattroff(status_bar, COLOR_PAIR(6));
    wrefresh(status_bar);
}

void draw_tabs() {
    int x = 0;
    for (int i = 0; i < tab_bar.count; i++) {
        if (i == tab_bar.active)
            wattron(stdscr, COLOR_PAIR(7));
        else
            wattron(stdscr, COLOR_PAIR(8));

        mvhline(1, x, ' ', strlen(tab_bar.tabs[i].name) + 4);
        mvprintw(1, x + 2, "%s%s", tab_bar.tabs[i].name,
                 tab_bar.tabs[i].modified ? "*" : "");
        
        if (i == tab_bar.active)
            wattroff(stdscr, COLOR_PAIR(7));
        else
            wattroff(stdscr, COLOR_PAIR(8));

        x += strlen(tab_bar.tabs[i].name) + 4;
    }
    refresh();
}

void load_file_content(Tab *tab) {
    FILE *file = fopen(tab->path, "r");
    if (!file) return;

    fseek(file, 0, SEEK_END);
    tab->content_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    tab->content = malloc(tab->content_size + 1);
    fread(tab->content, 1, tab->content_size, file);
    tab->content[tab->content_size] = '\0';
    
    fclose(file);
}

void open_file_in_tab(const char *path) {
    if (tab_bar.count >= MAX_TABS) return;

    Tab *tab = &tab_bar.tabs[tab_bar.count];
    strncpy(tab->path, path, MAX_PATH - 1);
    char *name = strrchr(path, '/');
    strncpy(tab->name, name ? name + 1 : path, MAX_NAME_LENGTH - 1);
    
    tab->scroll_pos = 0;
    tab->cursor_x = 0;
    tab->cursor_y = 0;
    tab->modified = 0;
    
    load_file_content(tab);
    
    tab_bar.active = tab_bar.count++;
}

void draw_file_content(WINDOW *win, Tab *tab) {
    wclear(win);
    box(win, 0, 0);

    if (!tab->content) {
        mvwprintw(win, 1, 1, "Empty file");
        wrefresh(win);
        return;
    }

    int y = 1;
    char *line = tab->content;
    int lines_to_skip = tab->scroll_pos;
    
    while (line && *line && y < getmaxy(win) - 1) {
        if (lines_to_skip > 0) {
            char *next = strchr(line, '\n');
            if (next) line = next + 1;
            lines_to_skip--;
            continue;
        }

        char display[256];
        int len = 0;
        while (len < 255 && line[len] && line[len] != '\n') {
            display[len] = line[len];
            len++;
        }
        display[len] = '\0';

        mvwprintw(win, y++, 1, "%-*.*s", getmaxx(win) - 2, getmaxx(win) - 2, display);
        
        char *next = strchr(line, '\n');
        if (next) line = next + 1;
        else break;
    }

    // Draw cursor
    wmove(win, tab->cursor_y - tab->scroll_pos + 1, tab->cursor_x + 1);
    wrefresh(win);
}

void handle_mouse() {
    MEVENT event;
    if (getmouse(&event) == OK) {
        // Handle tab bar clicks
        if (event.y == 1) {
            int x = 0;
            for (int i = 0; i < tab_bar.count; i++) {
                int tab_width = strlen(tab_bar.tabs[i].name) + 4;
                if (event.x >= x && event.x < x + tab_width) {
                    tab_bar.active = i;
                    return;
                }
                x += tab_width;
            }
        }
        
        // Handle menu bar clicks
        if (event.y == 0) {
            int x = 2;
            for (int i = 0; i < 4; i++) {
                if (event.x >= x && event.x < x + strlen(menu_bar.items[i]) + 2) {
                    menu_bar.selected = i;
                    return;
                }
                x += strlen(menu_bar.items[i]) + 3;
            }
        }

        // Handle file panel clicks
        if (event.x < screen_width / 2) {
            int clicked_index = event.y - 3 + file_panel.scroll_pos;
            if (clicked_index >= 0 && clicked_index < file_panel.count) {
                file_panel.selected = clicked_index;
            }
        }
    }
}

void draw_panel(Panel *panel, int width, int height, int startx, int is_active) {
    box(panel->win, 0, 0);
    int max_display = height - 2;
    
    if (panel->selected >= panel->count) panel->selected = panel->count - 1;
    if (panel->selected < 0) panel->selected = 0;
    
    if (panel->selected >= panel->start + max_display)
        panel->start = panel->selected - max_display + 1;
    if (panel->selected < panel->start)
        panel->start = panel->selected;

    for (int i = 0; i < max_display && i + panel->start < panel->count; i++) {
        int color_pair;
        char *item = panel->items[i + panel->start];
        
        if (i + panel->start == panel->selected && is_active)
            wattron(panel->win, COLOR_PAIR(3));
        else if (item[strlen(item) - 1] == '/')
            wattron(panel->win, COLOR_PAIR(1));
        else
            wattron(panel->win, COLOR_PAIR(2));

        mvwprintw(panel->win, i + 1, 1, "%-*.*s", width - 2, width - 2, item);
        
        if (i + panel->start == panel->selected && is_active)
            wattroff(panel->win, COLOR_PAIR(3));
        else if (item[strlen(item) - 1] == '/')
            wattroff(panel->win, COLOR_PAIR(1));
        else
            wattroff(panel->win, COLOR_PAIR(2));
    }
    wrefresh(panel->win);
}

void load_directory(Panel *panel, const char *path) {
    DIR *dir = opendir(path);
    if (!dir) {
        show_status_message("Error: Cannot open directory", 1);
        return;
    }

    // Clear existing items
    memset(panel->items, 0, sizeof(panel->items));
    panel->count = 0;
    panel->selected = 0;
    panel->start = 0;
    panel->scroll_pos = 0;

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL && panel->count < MAX_ITEMS) {
        if (strcmp(entry->d_name, ".") == 0) continue;
        
        // Check if filename length is within bounds
        size_t name_len = strlen(entry->d_name);
        if (name_len >= MAX_NAME_LENGTH - 1) continue;  // Skip too long filenames
        
        char full_path[MAX_PATH];
        if (snprintf(full_path, sizeof(full_path), "%s/%s", path, entry->d_name) >= MAX_PATH) {
            continue;  // Skip if path would be too long
        }
        
        struct stat st;
        if (stat(full_path, &st) == 0) {
            if (S_ISDIR(st.st_mode)) {
                if (snprintf(panel->items[panel->count], MAX_NAME_LENGTH + 1, "%s/", entry->d_name) < MAX_NAME_LENGTH) {
                    panel->count++;
                }
            } else {
                if (snprintf(panel->items[panel->count], MAX_NAME_LENGTH + 1, "%s", entry->d_name) < MAX_NAME_LENGTH) {
                    panel->count++;
                }
            }
        }
    }
    closedir(dir);
}

void preview_file(WINDOW *win, const char *path) {
    wclear(win);
    box(win, 0, 0);
    
    FILE *file = fopen(path, "r");
    if (!file) {
        mvwprintw(win, 1, 1, "Cannot open file");
        wrefresh(win);
        return;
    }

    wattron(win, COLOR_PAIR(4));
    char line[256];
    int y = 1;
    while (fgets(line, sizeof(line), file) && y < getmaxy(win) - 1) {
        line[strcspn(line, "\n")] = 0;
        mvwprintw(win, y++, 1, "%-*.*s", getmaxx(win) - 2, getmaxx(win) - 2, line);
    }
    wattroff(win, COLOR_PAIR(4));
    
    fclose(file);
    wrefresh(win);
}

void show_splash_screen() {
    clear();
    int max_y, max_x;
    getmaxyx(stdscr, max_y, max_x);

    // Calculate center position
    const char *title = "BMM";
    const char *subtitle = "Bare Metal Minux " VERSION;
    const char *press_key = "Press any key to start...";

    int title_x = (max_x - strlen(title)) / 2;
    int subtitle_x = (max_x - strlen(subtitle)) / 2;
    int press_key_x = (max_x - strlen(press_key)) / 2;
    int start_y = max_y / 2 - 2;

    // Draw splash screen
    attron(A_BOLD);
    mvprintw(start_y, title_x, "%s", title);
    attroff(A_BOLD);
    
    mvprintw(start_y + 1, subtitle_x, "%s", subtitle);
    mvprintw(start_y + 3, press_key_x, "%s", press_key);
    
    refresh();
    getch();  // Wait for any key
    clear();
    refresh();
}

void show_status_message(const char *message, int type) {
    strncpy(status_message, message, sizeof(status_message) - 1);
    status_message_type = type;
    status_message_time = clock();
}

void close_current_tab() {
    if (tab_bar.count > 0 && tab_bar.active >= 0) {
        // Free the content of the current tab
        free(tab_bar.tabs[tab_bar.active].content);
        
        // Shift remaining tabs left
        for (int i = tab_bar.active; i < tab_bar.count - 1; i++) {
            tab_bar.tabs[i] = tab_bar.tabs[i + 1];
        }
        
        tab_bar.count--;
        if (tab_bar.active >= tab_bar.count) {
            tab_bar.active = tab_bar.count - 1;
        }
        show_status_message("Tab closed", 2);
    }
}

void save_current_tab() {
    if (tab_bar.count > 0 && tab_bar.active >= 0) {
        Tab *tab = &tab_bar.tabs[tab_bar.active];
        FILE *file = fopen(tab->path, "w");
        if (file) {
            fwrite(tab->content, 1, tab->content_size, file);
            fclose(file);
            tab->modified = 0;
            show_status_message("File saved", 2);
        } else {
            show_status_message("Error: Could not save file", 1);
        }
    }
}

void toggle_hidden_files() {
    static int show_hidden = 0;
    show_hidden = !show_hidden;
    // Reload directory with new setting
    load_directory(&file_panel, current_path);
    show_status_message(show_hidden ? "Showing hidden files" : "Hiding hidden files", 2);
}

void show_help() {
    // Create help content
    const char *help_text = 
        "Keyboard Shortcuts:\n"
        "Alt+F: File menu\n"
        "Alt+E: Edit menu\n"
        "Alt+V: View menu\n"
        "Alt+H: Help menu\n"
        "Ctrl+S: Save file\n"
        "Ctrl+W: Close tab\n"
        "Tab: Switch tabs\n"
        "Enter: Open file/folder\n"
        "Q: Quit\n";
    
    // Create a new tab with help content
    if (tab_bar.count < MAX_TABS) {
        Tab *tab = &tab_bar.tabs[tab_bar.count];
        strncpy(tab->name, "Help", MAX_NAME_LENGTH - 1);
        strncpy(tab->path, "help", MAX_PATH - 1);
        tab->content_size = strlen(help_text);
        tab->content = strdup(help_text);
        tab->scroll_pos = 0;
        tab->cursor_x = 0;
        tab->cursor_y = 0;
        tab->modified = 0;
        tab_bar.active = tab_bar.count++;
    }
}

// Initialize menus
void init_menus() {
    // File menu
    static MenuItem file_items[] = {
        {"New Tab", "Ctrl+T", NULL},
        {"Open", "Ctrl+O", NULL},
        {"Save", "Ctrl+S", save_current_tab},
        {"Close Tab", "Ctrl+W", close_current_tab},
        {"Exit", "Q", NULL}
    };
    menus[0].name = "File";
    menus[0].items = file_items;
    menus[0].count = 5;
    
    // Edit menu
    static MenuItem edit_items[] = {
        {"Cut", "Ctrl+X", NULL},
        {"Copy", "Ctrl+C", NULL},
        {"Paste", "Ctrl+V", NULL}
    };
    menus[1].name = "Edit";
    menus[1].items = edit_items;
    menus[1].count = 3;
    
    // View menu
    static MenuItem view_items[] = {
        {"Toggle Hidden Files", "Ctrl+H", toggle_hidden_files},
        {"Word Wrap", "Alt+Z", NULL}
    };
    menus[2].name = "View";
    menus[2].items = view_items;
    menus[2].count = 2;
    
    // Help menu
    static MenuItem help_items[] = {
        {"Keyboard Shortcuts", "F1", show_help},
        {"About", "", NULL}
    };
    menus[3].name = "Help";
    menus[3].items = help_items;
    menus[3].count = 2;
}

void draw_menu_dropdown(Menu *menu, int x, int y) {
    if (!menu->win) {
        // Create window for dropdown
        int width = 0;
        for (int i = 0; i < menu->count; i++) {
            int item_width = strlen(menu->items[i].label) + 
                           (menu->items[i].shortcut ? strlen(menu->items[i].shortcut) + 4 : 0) + 4;
            width = width > item_width ? width : item_width;
        }
        menu->win = newwin(menu->count + 2, width, y, x);
        keypad(menu->win, TRUE);
    }
    
    // Draw menu items
    werase(menu->win);  // Clear the window first
    wattron(menu->win, COLOR_PAIR(5));
    box(menu->win, 0, 0);
    for (int i = 0; i < menu->count; i++) {
        if (i == menu->selected)
            wattron(menu->win, A_REVERSE);
            
        mvwprintw(menu->win, i + 1, 2, "%s", menu->items[i].label);
        
        // Draw shortcut right-aligned
        if (menu->items[i].shortcut) {
            wattron(menu->win, A_DIM);
            mvwprintw(menu->win, i + 1, getmaxx(menu->win) - strlen(menu->items[i].shortcut) - 2,
                     "%s", menu->items[i].shortcut);
            wattroff(menu->win, A_DIM);
        }
            
        if (i == menu->selected)
            wattroff(menu->win, A_REVERSE);
    }
    wattroff(menu->win, COLOR_PAIR(5));
    touchwin(menu->win);  // Mark the entire window for redraw
    wrefresh(menu->win);
}

void handle_menu_input(int ch) {
    if (active_menu >= 0) {
        Menu *menu = &menus[active_menu];
        switch (ch) {
            case KEY_UP:
                menu->selected = (menu->selected - 1 + menu->count) % menu->count;
                break;
            case KEY_DOWN:
                menu->selected = (menu->selected + 1) % menu->count;
                break;
            case '\n':
                if (menu->items[menu->selected].action) {
                    menu->items[menu->selected].action();
                }
                // Fall through to ESC to close menu
            case KEY_ESC:
                if (menu->win) {
                    delwin(menu->win);
                    menu->win = NULL;
                }
                active_menu = -1;
                break;
        }
    } else {
        // Menu activation shortcuts
        ch = tolower(ch);  // Convert to lowercase for case-insensitive comparison
        switch (ch) {
            case 'f': active_menu = 0; break;  // Alt+F
            case 'e': active_menu = 1; break;  // Alt+E
            case 'v': active_menu = 2; break;  // Alt+V
            case 'h': active_menu = 3; break;  // Alt+H
        }
        if (active_menu >= 0) {
            menus[active_menu].selected = 0;
        }
    }
}

int main() {
    char current_path[MAX_PATH];
    getcwd(current_path, sizeof(current_path));

    // Initialize ncurses
    initscr();
    raw();
    noecho();
    keypad(stdscr, TRUE);
    mousemask(ALL_MOUSE_EVENTS | REPORT_MOUSE_POSITION, NULL);
    mouseinterval(0);
    curs_set(1);
    init_colors();

    getmaxyx(stdscr, screen_height, screen_width);

    // Initialize windows with panels
    menu_bar.win = newwin(1, screen_width, 0, 0);
    menu_bar.items[0] = "File";
    menu_bar.items[1] = "Edit";
    menu_bar.items[2] = "View";
    menu_bar.items[3] = "Help";
    menu_bar.count = 4;
    menu_bar.selected = -1;

    status_bar = newwin(1, screen_width, screen_height - 1, 0);
    
    // Adjust main windows for menu bar, tab bar, and status bar
    int main_height = screen_height - 3;  // -1 for menu, -1 for tabs, -1 for status
    file_panel.win = newwin(main_height, screen_width / 2, 2, 0);
    preview_win = newwin(main_height, screen_width / 2, 2, screen_width / 2);

    // Show splash screen
    show_splash_screen();

    // Initialize tab bar
    tab_bar.count = 0;
    tab_bar.active = -1;

    // Initialize menus
    init_menus();

    // Main event loop
    load_directory(&file_panel, current_path);
    
    while (1) {
        // Draw base windows first
        draw_panel(&file_panel, screen_width / 2, main_height, 0, 1);
        if (tab_bar.active >= 0) {
            draw_file_content(preview_win, &tab_bar.tabs[tab_bar.active]);
        }
        
        // Draw overlays last
        draw_tabs();
        draw_menu_bar();
        draw_status_bar(current_path);
        
        // Draw active menu on top of everything
        if (active_menu >= 0) {
            int menu_x = 2;
            for (int i = 0; i < active_menu; i++) {
                menu_x += strlen(menus[i].name) + 3;
            }
            draw_menu_dropdown(&menus[active_menu], menu_x, 1);
            doupdate();  // Ensure all updates are shown
        }

        // Handle input
        int ch = getch();
        
        // Check for Alt key combinations
        if (ch == 27) {  // ESC or Alt key
            nodelay(stdscr, TRUE);  // Don't wait for next char
            int next_ch = getch();
            nodelay(stdscr, FALSE);  // Reset to normal mode
            
            if (next_ch != ERR) {  // Alt + key combination
                handle_menu_input(tolower(next_ch));
                continue;
            }
        }
        
        if (active_menu >= 0) {
            handle_menu_input(ch);
        } else {
            switch (ch) {
                case KEY_MOUSE:
                    handle_mouse();
                    break;
                case 'q':
                    goto cleanup;
                case KEY_UP:
                    if (file_panel.selected > 0)
                        file_panel.selected--;
                    break;
                case KEY_DOWN:
                    if (file_panel.selected < file_panel.count - 1)
                        file_panel.selected++;
                    break;
                case '\t':
                    if (tab_bar.count > 0) {
                        tab_bar.active = (tab_bar.active + 1) % tab_bar.count;
                    }
                    break;
                case '\n':
                    {
                        char *selected = file_panel.items[file_panel.selected];
                        if (selected[strlen(selected) - 1] == '/') {
                            // Handle directory navigation
                            selected[strlen(selected) - 1] = '\0';
                            if (strcmp(selected, "..") == 0) {
                                char *last_slash = strrchr(current_path, '/');
                                if (last_slash && last_slash != current_path)
                                    *last_slash = '\0';
                            } else {
                                size_t current_len = strlen(current_path);
                                size_t selected_len = strlen(selected);
                                
                                if (current_len + selected_len + 2 <= MAX_PATH) {
                                    strcat(current_path, "/");
                                    strcat(current_path, selected);
                                } else {
                                    show_status_message("Error: Path too long", 1);
                                }
                            }
                            load_directory(&file_panel, current_path);
                        } else {
                            // Open file in new tab
                            if (tab_bar.count >= MAX_TABS) {
                                show_status_message("Error: Maximum number of tabs reached", 1);
                            } else {
                                char full_path[MAX_PATH];
                                if (safe_path_join(full_path, MAX_PATH, current_path, selected) >= 0) {
                                    open_file_in_tab(full_path);
                                    show_status_message("File opened successfully", 2);
                                } else {
                                    show_status_message("Error: Path too long", 1);
                                }
                            }
                        }
                    }
                    break;
                case KEY_CTRL('s'):  // Ctrl+S
                    save_current_tab();
                    break;
                case KEY_CTRL('w'):  // Ctrl+W
                    close_current_tab();
                    break;
                case KEY_CTRL('h'):  // Ctrl+H
                    toggle_hidden_files();
                    break;
            }
        }
    }

cleanup:
    // Free allocated memory
    for (int i = 0; i < tab_bar.count; i++) {
        free(tab_bar.tabs[i].content);
    }
    
    // Delete windows
    delwin(menu_bar.win);
    delwin(status_bar);
    delwin(file_panel.win);
    delwin(preview_win);
    endwin();
    return 0;
}
