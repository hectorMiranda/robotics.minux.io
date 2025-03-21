#ifndef ERROR_CONSOLE_H
#define ERROR_CONSOLE_H

#include <ncurses.h>
#include <time.h>

// Constants
#define MAX_ERROR_MESSAGES 100
#define MAX_ERROR_LENGTH 256
#define MAX_PATH_LENGTH 4096
#define MAX_ERROR_HISTORY 100

// Box drawing characters
#define CONSOLE_TOP_LEFT "┌"
#define CONSOLE_TOP_RIGHT "┐"
#define CONSOLE_BOTTOM_LEFT "└"
#define CONSOLE_BOTTOM_RIGHT "┘"
#define CONSOLE_HORIZONTAL "─"
#define CONSOLE_VERTICAL "│"

// Color pairs
#define ERROR_COLOR_BORDER 1
#define ERROR_COLOR_SUCCESS 2
#define ERROR_COLOR_INFO 3
#define ERROR_COLOR_WARNING 4
#define ERROR_COLOR_CRITICAL 5
#define ERROR_COLOR_TITLE 6

// Error levels
typedef enum {
    ERROR_CRITICAL = 0,
    ERROR_WARNING = 1,
    ERROR_INFO = 2,
    ERROR_SUCCESS = 3
} ErrorLevel;

// Forward declaration of ErrorConsole
typedef struct _ErrorConsole ErrorConsole;

// Error message structure
typedef struct ErrorMessage {
    ErrorLevel level;
    char source[64];
    char message[MAX_ERROR_LENGTH];
    char timestamp[32];
    struct ErrorMessage *next;
} ErrorMessage;

// Error console structure
struct _ErrorConsole {
    WINDOW *win;
    int is_visible;
    int message_count;
    int scroll_position;
    ErrorMessage *messages;
    int critical_count;
    int warning_count;
    int info_count;
    int success_count;
    char *log_path;
};

// Function prototypes
ErrorConsole* error_console_init();
void error_console_destroy(ErrorConsole *console);
void error_console_toggle(ErrorConsole *console);
void error_console_handle_input(ErrorConsole *console, int ch);
void log_error(ErrorConsole *console, ErrorLevel level, const char *source, const char *format, ...);
int get_error_count(ErrorConsole *console, ErrorLevel level);
const char* get_last_error_message(ErrorConsole *console);
void update_status_bar_error(ErrorConsole *console);

#endif // ERROR_CONSOLE_H 