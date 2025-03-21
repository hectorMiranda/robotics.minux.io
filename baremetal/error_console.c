#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ncurses.h>
#include "error_console.h"
#include <sys/stat.h>
#include <sys/types.h>
#include <limits.h>
#include <stdarg.h>

// Forward declarations
static void draw_error_console(ErrorConsole *console);

// Get log file path
static char* get_log_path() {
    char *dir_path = (char *)malloc(MAX_PATH_LENGTH);
    if (!dir_path) {
        return NULL;
    }

    // Get home directory
    const char *home = getenv("HOME");
    if (!home) {
        free(dir_path);
        return NULL;
    }
    snprintf(dir_path, MAX_PATH_LENGTH, "%s/.minux", home);

    // Create directory if it doesn't exist
    mkdir(dir_path, 0755);

    char *log_path = (char *)malloc(MAX_PATH_LENGTH);
    if (!log_path) {
        free(dir_path);
        return NULL;
    }
    snprintf(log_path, MAX_PATH_LENGTH, "%s/error.log", dir_path);
    
    free(dir_path);
    return log_path;
}

// Initialize error console
ErrorConsole* error_console_init() {
    // Set up proper Unicode display
    const char *env_var = "NCURSES_NO_UTF8_ACS=1";
    putenv((char *)env_var);

    ErrorConsole *console = (ErrorConsole *)malloc(sizeof(ErrorConsole));
    if (!console) {
        return NULL;
    }

    // Initialize console properties
    console->is_visible = 0;
    console->scroll_position = 0;
    console->message_count = 0;
    console->messages = NULL;
    console->critical_count = 0;
    console->warning_count = 0;
    console->info_count = 0;
    console->success_count = 0;

    // Create window (will be sized when displayed)
    console->win = NULL;

    // Initialize color pairs
    init_pair(ERROR_COLOR_BORDER, COLOR_WHITE, COLOR_BLUE);
    init_pair(ERROR_COLOR_SUCCESS, COLOR_GREEN, COLOR_BLACK);
    init_pair(ERROR_COLOR_INFO, COLOR_WHITE, COLOR_BLACK);
    init_pair(ERROR_COLOR_WARNING, COLOR_YELLOW, COLOR_BLACK);
    init_pair(ERROR_COLOR_CRITICAL, COLOR_RED, COLOR_BLACK);
    init_pair(ERROR_COLOR_TITLE, COLOR_BLACK, COLOR_CYAN);

    // Get log path
    console->log_path = get_log_path();

    return console;
}

// Destroy error console
void error_console_destroy(ErrorConsole *console) {
    if (!console) {
        return;
    }

    if (console->win) {
        delwin(console->win);
    }

    // Free all error messages
    ErrorMessage *current = console->messages;
    while (current != NULL) {
        ErrorMessage *next = current->next;
        free(current);
        current = next;
    }

    // Free log path
    if (console->log_path) {
        free(console->log_path);
    }

    free(console);
}

// Toggle error console visibility
void error_console_toggle(ErrorConsole *console) {
    if (!console) {
        return;
    }

    console->is_visible = !console->is_visible;

    if (console->is_visible) {
        // Create window if it doesn't exist
        int max_y, max_x;
        getmaxyx(stdscr, max_y, max_x);
        
        if (console->win) {
            delwin(console->win);
        }
        
        console->win = newwin(max_y - 2, max_x, 0, 0);
        keypad(console->win, TRUE);
        
        console->scroll_position = 0;
        draw_error_console(console);
    } else {
        // Hide console
        if (console->win) {
            delwin(console->win);
            console->win = NULL;
        }
        redrawwin(stdscr);
        refresh();
    }
}

// Handle input for error console
void error_console_handle_input(ErrorConsole *console, int ch) {
    if (!console || !console->is_visible) {
        return;
    }

    switch (ch) {
        case KEY_UP: {
            if (console->scroll_position > 0) {
                console->scroll_position--;
                draw_error_console(console);
            }
            break;
        }
            
        case KEY_DOWN: {
            int msg_count = 0;
            ErrorMessage *msg = console->messages;
            while (msg != NULL) {
                msg_count++;
                msg = msg->next;
            }
            
            if (console->scroll_position < msg_count - 1) {
                console->scroll_position++;
                draw_error_console(console);
            }
            break;
        }
            
        case KEY_HOME: {
            console->scroll_position = 0;
            draw_error_console(console);
            break;
        }
            
        case KEY_END: {
            int msg_count = 0;
            ErrorMessage *msg = console->messages;
            while (msg != NULL) {
                msg_count++;
                msg = msg->next;
            }
            
            console->scroll_position = msg_count > 0 ? msg_count - 1 : 0;
            draw_error_console(console);
            break;
        }
            
        case 'q':
        case 'Q':
        case 27:  // ESC key
        case '`':
        case '~': {
            error_console_toggle(console);
            break;
        }
            
        case 'c':
        case 'C': {
            // Clear all errors
            ErrorMessage *current = console->messages;
            while (current != NULL) {
                ErrorMessage *next = current->next;
                free(current);
                current = next;
            }
            
            console->messages = NULL;
            console->scroll_position = 0;
            console->critical_count = 0;
            console->warning_count = 0;
            console->info_count = 0;
            console->success_count = 0;
            console->message_count = 0;
            draw_error_console(console);
            break;
        }
    }
}

// Draw error console
static void draw_error_console(ErrorConsole *console) {
    if (!console || !console->win) {
        return;
    }

    werase(console->win);
    
    // Draw border with colors
    wattron(console->win, COLOR_PAIR(ERROR_COLOR_BORDER) | A_BOLD);
    
    // Top border
    mvwprintw(console->win, 0, 0, "%s", CONSOLE_TOP_LEFT);
    for (int x = 1; x < getmaxx(console->win) - 1; x++) {
        mvwprintw(console->win, 0, x, "%s", CONSOLE_HORIZONTAL);
    }
    mvwprintw(console->win, 0, getmaxx(console->win) - 1, "%s", CONSOLE_TOP_RIGHT);
    
    // Side borders
    for (int y = 1; y < getmaxy(console->win) - 1; y++) {
        mvwprintw(console->win, y, 0, "%s", CONSOLE_VERTICAL);
        mvwprintw(console->win, y, getmaxx(console->win) - 1, "%s", CONSOLE_VERTICAL);
    }
    
    // Bottom border
    mvwprintw(console->win, getmaxy(console->win) - 1, 0, "%s", CONSOLE_BOTTOM_LEFT);
    for (int x = 1; x < getmaxx(console->win) - 1; x++) {
        mvwprintw(console->win, getmaxy(console->win) - 1, x, "%s", CONSOLE_HORIZONTAL);
    }
    mvwprintw(console->win, getmaxy(console->win) - 1, getmaxx(console->win) - 1, "%s", CONSOLE_BOTTOM_RIGHT);
    
    // Draw title
    wattron(console->win, COLOR_PAIR(ERROR_COLOR_TITLE));
    mvwprintw(console->win, 0, 2, " ERROR CONSOLE ");
    wattroff(console->win, COLOR_PAIR(ERROR_COLOR_TITLE));
    
    // Draw help text
    mvwprintw(console->win, getmaxy(console->win) - 1, 2, 
              " [ESC]: Close | [UP/DOWN]: Scroll | [C]: Clear ");
    
    wattroff(console->win, COLOR_PAIR(ERROR_COLOR_BORDER) | A_BOLD);
    
    // Display error counts
    wattron(console->win, COLOR_PAIR(ERROR_COLOR_BORDER));
    mvwprintw(console->win, 0, getmaxx(console->win) - 30, 
             " [%d Critical, %d Warnings] ", console->critical_count, console->warning_count);
    wattroff(console->win, COLOR_PAIR(ERROR_COLOR_BORDER));
    
    // Display messages
    int y = 1;
    ErrorMessage *msg = console->messages;
    
    // Skip messages based on scroll position
    for (int i = 0; i < console->scroll_position && msg != NULL; i++) {
        msg = msg->next;
    }
    
    // Display visible messages
    while (msg != NULL && y < getmaxy(console->win) - 1) {
        switch (msg->level) {
            case ERROR_CRITICAL:
                wattron(console->win, COLOR_PAIR(ERROR_COLOR_CRITICAL) | A_BOLD);
                break;
                
            case ERROR_WARNING:
                wattron(console->win, COLOR_PAIR(ERROR_COLOR_WARNING) | A_BOLD);
                break;
                
            case ERROR_INFO:
                wattron(console->win, COLOR_PAIR(ERROR_COLOR_INFO));
                break;
                
            case ERROR_SUCCESS:
                wattron(console->win, COLOR_PAIR(ERROR_COLOR_SUCCESS) | A_BOLD);
                break;
        }
        
        mvwprintw(console->win, y, 2, "[%s] <%s> [%s] %s", 
                 msg->timestamp, 
                 msg->level == ERROR_CRITICAL ? "CRITICAL" :
                 msg->level == ERROR_WARNING ? "WARNING" :
                 msg->level == ERROR_INFO ? "INFO" : "SUCCESS",
                 msg->source, 
                 msg->message);
                 
        switch (msg->level) {
            case ERROR_CRITICAL:
                wattroff(console->win, COLOR_PAIR(ERROR_COLOR_CRITICAL) | A_BOLD);
                break;
                
            case ERROR_WARNING:
                wattroff(console->win, COLOR_PAIR(ERROR_COLOR_WARNING) | A_BOLD);
                break;
                
            case ERROR_INFO:
                wattroff(console->win, COLOR_PAIR(ERROR_COLOR_INFO));
                break;
                
            case ERROR_SUCCESS:
                wattroff(console->win, COLOR_PAIR(ERROR_COLOR_SUCCESS) | A_BOLD);
                break;
        }
        
        y++;
        msg = msg->next;
    }
    
    wrefresh(console->win);
}

// Log error message
void log_error(ErrorConsole *console, ErrorLevel level, const char *source, const char *format, ...) {
    if (!console) {
        return;
    }
    
    // Format the error message
    char message[MAX_ERROR_LENGTH];
    va_list args;
    va_start(args, format);
    vsnprintf(message, sizeof(message), format, args);
    va_end(args);
    
    // Create new error message
    ErrorMessage *msg = (ErrorMessage *)malloc(sizeof(ErrorMessage));
    if (!msg) {
        return;
    }
    
    // Fill message data
    msg->level = level;
    strncpy(msg->source, source, sizeof(msg->source) - 1);
    msg->source[sizeof(msg->source) - 1] = '\0';
    strncpy(msg->message, message, sizeof(msg->message) - 1);
    msg->message[sizeof(msg->message) - 1] = '\0';
    
    // Get current time
    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    strftime(msg->timestamp, sizeof(msg->timestamp), "%Y-%m-%d %H:%M:%S", tm_info);
    
    // Initialize next pointer
    msg->next = NULL;
    
    // Add to head of list or append to list
    if (console->messages == NULL) {
        console->messages = msg;
    } else {
        // Find last message
        ErrorMessage *last = console->messages;
        while (last->next != NULL) {
            last = last->next;
        }
        last->next = msg;
    }
    
    // Update error counts
    switch (level) {
        case ERROR_CRITICAL:
            console->critical_count++;
            break;
            
        case ERROR_WARNING:
            console->warning_count++;
            break;
            
        case ERROR_INFO:
            console->info_count++;
            break;
            
        case ERROR_SUCCESS:
            console->success_count++;
            break;
    }
    
    console->message_count++;
    
    // Limit number of messages
    if (console->message_count > MAX_ERROR_MESSAGES) {
        ErrorMessage *old = console->messages;
        console->messages = old->next;
        free(old);
        console->message_count--;
    }
    
    // Write to log file
    if (console->log_path) {
        FILE *log_file = fopen(console->log_path, "a");
        if (log_file) {
            fprintf(log_file, "[%s] <%s> [%s] %s\n", 
                   msg->timestamp, 
                   level == ERROR_CRITICAL ? "CRITICAL" :
                   level == ERROR_WARNING ? "WARNING" :
                   level == ERROR_INFO ? "INFO" : "SUCCESS",
                   source, 
                   message);
                   
            fclose(log_file);
        }
    }
    
    // Refresh console if visible
    if (console->is_visible) {
        draw_error_console(console);
    }
    
    // Update status bar
    update_status_bar_error(console);
}

// Public status bar update function
void update_status_bar_error(ErrorConsole *console) {
    // This is a stub - implement with actual status bar update logic
    (void)console; // Suppress unused parameter warning
}

// Get the count of errors of a specific level
int get_error_count(ErrorConsole *console, ErrorLevel level) {
    if (!console) {
        return 0;
    }
    
    int count = 0;
    ErrorMessage *msg = console->messages;
    
    while (msg != NULL) {
        if (msg->level == level) {
            count++;
        }
        msg = msg->next;
    }
    
    return count;
}

// Get the last error message
const char* get_last_error_message(ErrorConsole *console) {
    if (!console || console->messages == NULL) {
        return NULL;
    }
    
    // Find last message (at the end of the linked list)
    ErrorMessage *last = console->messages;
    while (last->next != NULL) {
        last = last->next;
    }
    
    return last->message;
}