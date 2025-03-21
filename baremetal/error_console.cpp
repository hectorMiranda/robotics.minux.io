#include "error_console.h"
#include <sys/stat.h>
#include <unistd.h>
#include <pwd.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdarg.h>
#include <stdio.h>
#include <locale.h>
#include <ncurses.h>
#include <panel.h>
#include <menu.h>
#include <cstring>
#include <cstdlib>

// Helper function declarations
static int min(int a, int b) {
    return (a < b) ? a : b;
}

static int max(int a, int b) {
    return (a > b) ? a : b;
}

static void write_to_log_file(const char *log_path, const char *timestamp, 
                            ErrorLevel level, const char *source, const char *message) {
    FILE *fp = fopen(log_path, "a");
    if (!fp) return;

    const char *level_str = 
        level == ERROR_SUCCESS ? "SUCCESS" :
        level == ERROR_INFO ? "INFO" :
        level == ERROR_WARNING ? "WARNING" :
        level == ERROR_CRITICAL ? "CRITICAL" : "UNKNOWN";

    fprintf(fp, "[%s] <%s> %s: %s\n", 
            timestamp, level_str, source, message);
    fclose(fp);
}

static char* get_log_path(void) {
    const char *home = getenv("HOME");
    if (!home) {
        struct passwd *pw = getpwuid(getuid());
        if (!pw) return NULL;
        home = pw->pw_dir;
    }
    
    // Create .minux directory if it doesn't exist
    char *dir_path = (char *)malloc(PATH_MAX);
    if (!dir_path) return NULL;
    
    snprintf(dir_path, PATH_MAX, "%s/.minux", home);
    mkdir(dir_path, 0755);
    
    // Create full path to log file
    char *log_path = (char *)malloc(PATH_MAX);
    if (!log_path) {
        free(dir_path);
        return NULL;
    }
    
    snprintf(log_path, PATH_MAX, "%s/error.log", dir_path);
    free(dir_path);
    
    return log_path;
}

// Initialize error console
ErrorConsole* error_console_init(void) {
    // Set up UTF-8 support
    setlocale(LC_ALL, "");
    const char *env_var = "NCURSES_NO_UTF8_ACS=1";
    putenv((char *)env_var);

    ErrorConsole *console = (ErrorConsole *)malloc(sizeof(ErrorConsole));
    if (!console) return NULL;

    // Initialize console state
    console->messages = NULL;
    console->is_visible = 0;
    console->scroll_offset = 0;
    console->total_messages = 0;
    console->window_height = LINES * 3/4;
    console->window_width = COLS;
    console->log_path = get_log_path();

    // Create window with a border
    console->window = newwin(console->window_height, console->window_width, 0, 0);
    if (!console->window) {
        free(console->log_path);
        free(console);
        return NULL;
    }

    // Initialize colors for different message types
    start_color();
    use_default_colors();
    init_pair(ERROR_COLOR_BORDER, COLOR_BLUE, -1);
    init_pair(ERROR_COLOR_SUCCESS, COLOR_GREEN, -1);
    init_pair(ERROR_COLOR_INFO, COLOR_WHITE, -1);
    init_pair(ERROR_COLOR_WARNING, COLOR_YELLOW, -1);
    init_pair(ERROR_COLOR_CRITICAL, COLOR_RED, -1);
    init_pair(ERROR_COLOR_TITLE, COLOR_CYAN, -1);

    // Enable keypad and scrolling
    keypad(console->window, TRUE);
    scrollok(console->window, TRUE);

    return console;
}

static void draw_console_border(ErrorConsole *console) {
    int height = console->window_height;
    int width = console->window_width;

    // Set border color
    wattron(console->window, COLOR_PAIR(ERROR_COLOR_BORDER) | A_BOLD);

    // Draw corners with proper Unicode box characters
    mvwaddstr(console->window, 0, 0, CONSOLE_TOP_LEFT);
    mvwaddstr(console->window, 0, width-1, CONSOLE_TOP_RIGHT);
    mvwaddstr(console->window, height-1, 0, CONSOLE_BOTTOM_LEFT);
    mvwaddstr(console->window, height-1, width-1, CONSOLE_BOTTOM_RIGHT);

    // Draw horizontal borders
    for (int x = 1; x < width-1; x++) {
        mvwaddstr(console->window, 0, x, CONSOLE_HORIZONTAL);
        mvwaddstr(console->window, height-1, x, CONSOLE_HORIZONTAL);
    }

    // Draw vertical borders
    for (int y = 1; y < height-1; y++) {
        mvwaddstr(console->window, y, 0, CONSOLE_VERTICAL);
        mvwaddstr(console->window, y, width-1, CONSOLE_VERTICAL);
    }

    // Draw title with fancy borders
    int title_pos = (width - 20) / 2;
    mvwaddstr(console->window, 0, title_pos, "╡ MINUX ERROR LOG ╞");

    wattroff(console->window, COLOR_PAIR(ERROR_COLOR_BORDER) | A_BOLD);
}

static void refresh_console(ErrorConsole *console) {
    werase(console->window);
    draw_console_border(console);

    // Display messages
    int y = 1;
    ErrorMessage *msg = console->messages;
    int skip = console->scroll_offset;
    
    // Skip messages based on scroll offset
    while (msg && skip > 0) {
        msg = msg->next;
        skip--;
    }

    // Display visible messages
    while (msg && y < console->window_height - 2) {
        // Set color based on error level
        int color_pair;
        const char *level_str;
        
        switch (msg->level) {
            case ERROR_SUCCESS:
                color_pair = ERROR_COLOR_SUCCESS;
                level_str = "SUCCESS";
                break;
            case ERROR_INFO:
                color_pair = ERROR_COLOR_INFO;
                level_str = "INFO";
                break;
            case ERROR_WARNING:
                color_pair = ERROR_COLOR_WARNING;
                level_str = "WARNING";
                break;
            case ERROR_CRITICAL:
                color_pair = ERROR_COLOR_CRITICAL;
                level_str = "CRITICAL";
                break;
            default:
                color_pair = ERROR_COLOR_INFO;
                level_str = "UNKNOWN";
        }
        
        // Draw timestamp and level
        wattron(console->window, COLOR_PAIR(color_pair) | A_BOLD);
        mvwprintw(console->window, y, 2, "[%s]", msg->timestamp);
        mvwprintw(console->window, y, 22, "<%s>", level_str);
        wattroff(console->window, A_BOLD);
        
        // Draw source and message
        mvwprintw(console->window, y, 32, "%s:", msg->source);
        mvwprintw(console->window, y, 32 + strlen(msg->source) + 2, "%s", msg->message);
        wattroff(console->window, COLOR_PAIR(color_pair));
        
        y++;
        msg = msg->next;
    }

    // Show scroll indicator if needed
    if (console->total_messages > console->window_height - 3) {
        wattron(console->window, COLOR_PAIR(ERROR_COLOR_INFO));
        mvwprintw(console->window, console->window_height - 2, 2,
                 "Use UP/DOWN to scroll, ESC to close (Message %d/%d)",
                 min(console->scroll_offset + 1, console->total_messages),
                 console->total_messages);
        wattroff(console->window, COLOR_PAIR(ERROR_COLOR_INFO));
    }

    wrefresh(console->window);
}

void error_console_toggle(ErrorConsole *console) {
    console->is_visible = !console->is_visible;
    if (console->is_visible) {
        refresh_console(console);
    } else {
        werase(console->window);
        wrefresh(console->window);
        refresh();
    }
}

void error_console_handle_input(ErrorConsole *console, int ch) {
    switch (ch) {
        case KEY_UP:
            if (console->scroll_offset > 0) {
                console->scroll_offset--;
                refresh_console(console);
            }
            break;
        case KEY_DOWN:
            if (console->scroll_offset < console->total_messages - 1) {
                console->scroll_offset++;
                refresh_console(console);
            }
            break;
        case KEY_PPAGE:  // Page Up
            console->scroll_offset = max(0, console->scroll_offset - (console->window_height - 3));
            refresh_console(console);
            break;
        case KEY_NPAGE:  // Page Down
            console->scroll_offset = min(console->total_messages - 1,
                                       console->scroll_offset + (console->window_height - 3));
            refresh_console(console);
            break;
        case KEY_HOME:  // Home
            console->scroll_offset = 0;
            refresh_console(console);
            break;
        case KEY_END:   // End
            console->scroll_offset = max(0, console->total_messages - 1);
            refresh_console(console);
            break;
        case 27:  // ESC
            error_console_toggle(console);
            break;
    }
}

void log_error(ErrorConsole *console, ErrorLevel level, const char *source,
               const char *format, ...) {
    ErrorMessage *msg = (ErrorMessage *)malloc(sizeof(ErrorMessage));
    if (!msg) return;

    // Get current timestamp
    time_t now = time(NULL);
    strftime(msg->timestamp, sizeof(msg->timestamp), "%Y-%m-%d %H:%M:%S",
             localtime(&now));

    // Format message
    va_list args;
    va_start(args, format);
    vsnprintf(msg->message, sizeof(msg->message), format, args);
    va_end(args);

    msg->level = level;
    strncpy(msg->source, source, sizeof(msg->source) - 1);
    msg->source[sizeof(msg->source) - 1] = '\0';
    msg->next = NULL;

    // Add to message list
    if (!console->messages) {
        console->messages = msg;
    } else {
        ErrorMessage *current = console->messages;
        while (current->next) {
            current = current->next;
        }
        current->next = msg;
    }
    console->total_messages++;

    // Write to log file if path is available
    if (console->log_path) {
        write_to_log_file(console->log_path, msg->timestamp, level, source, msg->message);
    }

    // Auto-scroll if at bottom
    if (console->scroll_offset == console->total_messages - 2) {
        console->scroll_offset++;
    }

    // Show console automatically for critical errors
    if (level == ERROR_CRITICAL && !console->is_visible) {
        error_console_toggle(console);
    } else if (console->is_visible) {
        refresh_console(console);
    }

    // Update status bar
    update_status_bar_error(console);
}

void error_console_destroy(ErrorConsole *console) {
    if (!console) return;

    // Free message list
    ErrorMessage *current = console->messages;
    while (current) {
        ErrorMessage *next = current->next;
        free(current);
        current = next;
    }

    // Clean up resources
    if (console->log_path) free(console->log_path);
    delwin(console->window);
    free(console);
}

int get_error_count(ErrorConsole *console, ErrorLevel level) {
    int count = 0;
    ErrorMessage *current = console->messages;
    while (current) {
        if (current->level == level) count++;
        current = current->next;
    }
    return count;
}

const char* get_last_error_message(ErrorConsole *console) {
    if (!console->messages) return NULL;
    ErrorMessage *current = console->messages;
    while (current->next) {
        current = current->next;
    }
    return current->message;
}

// For backward compatibility
ErrorConsole *create_error_console(void) {
    return error_console_init();
}

void destroy_error_console(ErrorConsole *console) {
    error_console_destroy(console);
}

void update_error_console(ErrorConsole *console) {
    if (!console || !console->is_visible) {
        return;
    }
    refresh_console(console);
}

void show_error_console(ErrorConsole *console) {
    if (!console || console->is_visible) {
        return;
    }
    console->is_visible = 1;
    refresh_console(console);
}

void hide_error_console(ErrorConsole *console) {
    if (!console || !console->is_visible) {
        return;
    }
    console->is_visible = 0;
    werase(console->window);
    wrefresh(console->window);
    refresh();
}

void toggle_error_console(ErrorConsole *console) {
    error_console_toggle(console);
}

int handle_error_console_input(ErrorConsole *console, int ch) {
    if (!console || !console->is_visible) {
        return 0;
    }
    error_console_handle_input(console, ch);
    return 1;
}

int count_error_messages(ErrorConsole *console) {
    return console->total_messages;
}

// Update the status bar with error information
void update_status_bar_error(WINDOW *status_bar, ErrorConsole *console) {
    if (!status_bar || !console) {
        return;
    }

    int errors = 0;
    int warnings = 0;
    int infos = 0;
    ErrorMessage *current = console->messages;
    
    while (current) {
        switch (current->level) {
            case ERROR_ERROR:
                errors++;
                break;
            case ERROR_WARNING:
                warnings++;
                break;
            case ERROR_INFO:
                infos++;
                break;
            default:
                break;
        }
        current = current->next;
    }

    // Display the error counts in the status bar
    wattron(status_bar, A_BOLD);
    mvwprintw(status_bar, 0, COLS - 40, "Errors: ");
    if (errors > 0) {
        wattron(status_bar, COLOR_PAIR(COLOR_PAIR_ERROR));
    }
    wprintw(status_bar, "%d ", errors);
    if (errors > 0) {
        wattroff(status_bar, COLOR_PAIR(COLOR_PAIR_ERROR));
    }
    
    wprintw(status_bar, "Warnings: ");
    if (warnings > 0) {
        wattron(status_bar, COLOR_PAIR(COLOR_PAIR_WARNING));
    }
    wprintw(status_bar, "%d ", warnings);
    if (warnings > 0) {
        wattroff(status_bar, COLOR_PAIR(COLOR_PAIR_WARNING));
    }
    
    wprintw(status_bar, "Info: ");
    if (infos > 0) {
        wattron(status_bar, COLOR_PAIR(COLOR_PAIR_INFO));
    }
    wprintw(status_bar, "%d", infos);
    if (infos > 0) {
        wattroff(status_bar, COLOR_PAIR(COLOR_PAIR_INFO));
    }
    wattroff(status_bar, A_BOLD);
    
    wrefresh(status_bar);
}

// Declaration with GCC weak attribute
#ifdef __GNUC__
__attribute__((weak))
#endif
void update_status_bar_error(ErrorConsole *console) {
    // This is a stub implementation that will be replaced by the app-specific version
    // when linked with either minux.o or explorer.o
    // The weak attribute allows it to be overridden without causing multiple definition errors
    (void)console;  // Prevent unused parameter warning
}