#ifndef ERROR_CONSOLE_H
#define ERROR_CONSOLE_H

#include <ncurses.h>
#include <panel.h>

// Error console settings
#define ERROR_LOG_DIR "/var/log/minux"
#define ERROR_LOG_FILE "/var/log/minux/error.log"
#define MAX_ERROR_LENGTH 256
#define MAX_ERROR_SOURCE 32
#define MAX_ERROR_TIMESTAMP 32

// Color pairs for error levels
#define COLOR_PAIR_ERROR 1
#define COLOR_PAIR_WARNING 2
#define COLOR_PAIR_INFO 3
#define COLOR_PAIR_DEBUG 4

// Modern console color definitions
#define ERROR_COLOR_BORDER 1
#define ERROR_COLOR_SUCCESS 2
#define ERROR_COLOR_INFO 3
#define ERROR_COLOR_WARNING 4
#define ERROR_COLOR_CRITICAL 5
#define ERROR_COLOR_TITLE 6

// Replace Unicode box drawing characters with ASCII equivalents
#define CONSOLE_TOP_LEFT "+"
#define CONSOLE_TOP_RIGHT "+"
#define CONSOLE_BOTTOM_LEFT "+"
#define CONSOLE_BOTTOM_RIGHT "+"
#define CONSOLE_HORIZONTAL "-"
#define CONSOLE_VERTICAL "|"

// Error levels
typedef enum {
    ERROR_SUCCESS = 0,
    ERROR_INFO,
    ERROR_WARNING,
    ERROR_CRITICAL,
    ERROR_ERROR = ERROR_CRITICAL,  // alias for backward compatibility
    ERROR_DEBUG   // For backward compatibility
} ErrorLevel;

// Error message structure
typedef struct ErrorMessage {
    ErrorLevel level;
    char timestamp[MAX_ERROR_TIMESTAMP];
    char source[MAX_ERROR_SOURCE];
    char message[MAX_ERROR_LENGTH];
    struct ErrorMessage *next;
} ErrorMessage;

// Error console structure
typedef struct ErrorConsole {
    WINDOW *window;
    PANEL *panel;
    ErrorMessage *messages;  // Newer format
    ErrorMessage *first_message; // Legacy for backward compatibility
    ErrorMessage *last_message;  // Legacy for backward compatibility
    char *log_path;
    int is_visible;
    int scroll_offset;
    int total_messages;
    int window_height;
    int window_width;
    int visible;            // Legacy for backward compatibility
    int message_count;      // Legacy for backward compatibility
} ErrorConsole;

// Console functions - modern API
ErrorConsole* error_console_init(void);
void error_console_destroy(ErrorConsole *console);
void error_console_toggle(ErrorConsole *console);
void error_console_handle_input(ErrorConsole *console, int ch);
void log_error(ErrorConsole *console, ErrorLevel level, const char *source, const char *format, ...);
int get_error_count(ErrorConsole *console, ErrorLevel level);
const char* get_last_error_message(ErrorConsole *console);

// Legacy API for backward compatibility
ErrorConsole *create_error_console(void);
void destroy_error_console(ErrorConsole *console);
void show_error_console(ErrorConsole *console);
void hide_error_console(ErrorConsole *console);
void toggle_error_console(ErrorConsole *console);
void update_error_console(ErrorConsole *console);
int handle_error_console_input(ErrorConsole *console, int ch);
int count_error_messages(ErrorConsole *console);

// Status bar update functions
void update_status_bar_error(WINDOW *status_bar, ErrorConsole *console);
void update_status_bar_error(ErrorConsole *console);  // Overloaded version

#endif /* ERROR_CONSOLE_H */ 