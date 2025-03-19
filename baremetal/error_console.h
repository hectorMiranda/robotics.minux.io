#ifndef ERROR_CONSOLE_H
#define ERROR_CONSOLE_H

#include <ncurses.h>
#include <panel.h>
#include <time.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>

#define MAX_ERROR_LENGTH 1024
#define MAX_ERROR_HISTORY 1000
#define ERROR_LOG_PATH "~/.minux/error.log"

// Error levels
typedef enum {
    ERROR_SUCCESS,
    ERROR_INFO,
    ERROR_WARNING,
    ERROR_CRITICAL
} ErrorLevel;

// Error message structure
typedef struct ErrorMessage {
    char timestamp[32];
    ErrorLevel level;
    char source[32];
    char message[256];
    struct ErrorMessage *next;
} ErrorMessage;

// Error console structure
typedef struct {
    WINDOW *window;
    ErrorMessage *messages;
    int is_visible;
    int scroll_offset;
    int total_messages;
    int window_height;
    int window_width;
    char *log_path;  // Path to error log file
} ErrorConsole;

// Box drawing characters
#define CONSOLE_TOP_LEFT     "╔"
#define CONSOLE_TOP_RIGHT    "╗"
#define CONSOLE_BOTTOM_LEFT  "╚"
#define CONSOLE_BOTTOM_RIGHT "╝"
#define CONSOLE_HORIZONTAL   "═"
#define CONSOLE_VERTICAL     "║"

// Color pairs
#define ERROR_COLOR_BORDER   1
#define ERROR_COLOR_SUCCESS  2
#define ERROR_COLOR_INFO     3
#define ERROR_COLOR_WARNING  4
#define ERROR_COLOR_CRITICAL 5
#define ERROR_COLOR_TITLE    6

// Core functions
ErrorConsole* error_console_init(void);
void error_console_destroy(ErrorConsole *console);
void error_console_toggle(ErrorConsole *console);
void error_console_handle_input(ErrorConsole *console, int ch);
void log_error(ErrorConsole *console, ErrorLevel level, const char *source, const char *format, ...);
int get_error_count(ErrorConsole *console, ErrorLevel level);
const char* get_last_error_message(ErrorConsole *console);

// Status bar integration
extern void update_status_bar_error(ErrorConsole *console);

#endif // ERROR_CONSOLE_H 