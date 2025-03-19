#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/wait.h>
#include <ncurses.h>
#include <dirent.h>
#include <sys/stat.h>
#include <errno.h>
#include <panel.h>
#include <ctype.h>  // For isspace()
#include <termios.h> // For serial communication
#include <fcntl.h>   // For file control
#include <signal.h>  // For signal handling
#include "error_console.h"
#include <locale.h>

// Conditionally include libcamera and pigpio only if available
#ifdef __arm__
    #if __has_include(<libcamera/libcamera.h>)
    #include <libcamera/libcamera.h>
    #endif

    #if __has_include(<pigpio.h>)
    #include <pigpio.h>
    #endif
#endif

#define VERSION "0.0.1"
#define MAX_CMD_LENGTH 256
#define MAX_ARGS 32
#define MAX_PATH 4096
#define STATUS_BAR_HEIGHT 1

// Serial communication structure
typedef struct {
    int fd;
    struct termios old_tio;
    int baud_rate;
    char port[256];
    int is_connected;
} SerialPort;

// Function declarations for serial communication
void serial_monitor(void);
static int open_serial_port(const char *port, int baud_rate);
static void close_serial_port(SerialPort *sp);
static void configure_serial_port(int fd, struct termios *tio);
static void handle_serial_input(SerialPort *sp);
static void handle_serial_output(SerialPort *sp);
static void cleanup_serial(void);

// Command function prototypes
void cmd_help(void);
void cmd_version(void);
void cmd_time(void);
void cmd_date(void);
void cmd_path(void);
void cmd_ls(const char *path);
void cmd_cd(const char *path);
void cmd_clear(void);
void cmd_gpio(void);
void launch_explorer(void);
void capture_image(const char *filename);
void test_camera(void);

// Command structure
typedef struct {
    const char *name;
    void (*func)(void);
    const char *help;
} Command;

// Available commands
Command commands[] = {
    {"help", cmd_help, "Display this help message"},
    {"version", cmd_version, "Display MINUX version"},
    {"time", cmd_time, "Display current time"},
    {"date", cmd_date, "Display current date"},
    {"path", cmd_path, "Display or modify system path"},
    {"ls", NULL, "List directory contents"},  // Special handling for args
    {"cd", NULL, "Change directory"},         // Special handling for args
    {"clear", cmd_clear, "Clear screen"},
    {"gpio", cmd_gpio, "Display GPIO status"},
    {"explorer", launch_explorer, "Launch file explorer"},
    {"test camera", test_camera, "Test the Arducam camera"},
    {"serial", serial_monitor, "Open serial monitor for device communication"},
    {NULL, NULL, NULL}
};

// Global variables
char current_path[MAX_PATH];
ErrorConsole *error_console;
WINDOW *status_bar;
int screen_width, screen_height;
SerialPort serial_port = {
    .fd = -1,
    .baud_rate = 115200,
    .is_connected = 0,
    .port = {0}
};

// Function declarations
void init_windows(void);
void cleanup(void);
void draw_status_bar(void);
void handle_command(const char *cmd);
void show_prompt(void);
static int levenshtein_distance(const char *s1, const char *s2);
static inline int min(int a, int b) {
    return (a < b) ? a : b;
}

// Command implementations
void cmd_help(void) {
    clear();
    int y = 1;
    mvprintw(y++, 1, "MINUX Commands:\n");
    y++;
    for (Command *cmd = commands; cmd->name != NULL; cmd++) {
        mvprintw(y++, 2, "%-15s - %s", cmd->name, cmd->help);
    }
    y++;
    refresh();
}

void cmd_version(void) {
    clear();
    mvprintw(1, 1, "MINUX Version %s", VERSION);
    refresh();
}

void cmd_time(void) {
    time_t t = time(NULL);
    struct tm *tm = localtime(&t);
    char time_str[64];
    strftime(time_str, sizeof(time_str), "%H:%M:%S", tm);
    clear();
    mvprintw(1, 1, "Current time: %s", time_str);
    refresh();
}

void cmd_date(void) {
    time_t t = time(NULL);
    struct tm *tm = localtime(&t);
    char date_str[64];
    strftime(date_str, sizeof(date_str), "%Y-%m-%d", tm);
    clear();
    mvprintw(1, 1, "Current date: %s", date_str);
    refresh();
}

void cmd_path(void) {
    char *path = getenv("PATH");
    clear();
    int y = 1;
    if (path) {
        mvprintw(y++, 1, "System PATH:");
        y++;
        char *token = strtok(path, ":");
        while (token != NULL) {
            mvprintw(y++, 2, "%s", token);
            token = strtok(NULL, ":");
        }
    } else {
        mvprintw(y, 1, "PATH environment variable not found");
    }
    refresh();
}

void cmd_ls(const char *path) {
    DIR *dir;
    struct dirent *entry;
    const char *target_path = path ? path : ".";

    dir = opendir(target_path);
    if (dir == NULL) {
        log_error(error_console, ERROR_WARNING, "MINUX", 
                  "Error opening directory '%s': %s", target_path, strerror(errno));
        return;
    }

    clear();
    int y = 1;
    mvprintw(y++, 1, "Contents of %s:", target_path);
    y++;

    // Initialize color pairs for ls
    init_pair(1, COLOR_BLUE, COLOR_BLACK);   // Directories
    init_pair(2, COLOR_GREEN, COLOR_BLACK);  // Executables
    
    int x = 2;
    while ((entry = readdir(dir)) != NULL) {
        struct stat st;
        char full_path[MAX_PATH];
        snprintf(full_path, sizeof(full_path), "%s/%s", target_path, entry->d_name);
        
        if (stat(full_path, &st) == 0) {
            if (x > screen_width - 25) {
                x = 2;
                y++;
            }
            
            if (S_ISDIR(st.st_mode)) {
                attron(COLOR_PAIR(1) | A_BOLD);
                mvprintw(y, x, "%-20s", entry->d_name);
                attroff(COLOR_PAIR(1) | A_BOLD);
            }
            else if (st.st_mode & S_IXUSR) {
                attron(COLOR_PAIR(2) | A_BOLD);
                mvprintw(y, x, "%-20s", entry->d_name);
                attroff(COLOR_PAIR(2) | A_BOLD);
            }
            else {
                mvprintw(y, x, "%-20s", entry->d_name);
            }
            x += 22;
        }
    }
    mvprintw(y + 2, 1, " ");  // Move cursor to end with a space
    refresh();
    closedir(dir);
}

void cmd_cd(const char *path) {
    if (!path) {
        log_error(error_console, ERROR_WARNING, "MINUX", "Usage: cd <directory>");
        return;
    }
    
    if (chdir(path) != 0) {
        log_error(error_console, ERROR_WARNING, "MINUX", 
                  "Error changing to directory '%s': %s", path, strerror(errno));
    } else {
        getcwd(current_path, sizeof(current_path));
    }
}

void cmd_clear(void) {
    clear();
    refresh();
    show_prompt();
}

void cmd_gpio(void) {
    clear();
    #ifdef __arm__
    // Raspberry Pi specific GPIO handling
    FILE *fp = fopen("/sys/class/gpio/export", "w");
    if (fp == NULL) {
        log_error(error_console, ERROR_WARNING, "MINUX", 
                  "GPIO interface not available (are you running on Raspberry Pi?)");
        return;
    }
    fclose(fp);
    
    // Display GPIO status in a formatted way
    mvprintw(1, 1, "GPIO Status:");
    mvprintw(3, 2, "GPIO Directory: /sys/class/gpio/");
    
    // Try to read GPIO exports
    DIR *dir = opendir("/sys/class/gpio");
    if (dir) {
        struct dirent *entry;
        int y = 5;
        while ((entry = readdir(dir)) != NULL) {
            if (strncmp(entry->d_name, "gpio", 4) == 0) {
                mvprintw(y++, 2, "- %s", entry->d_name);
            }
        }
        closedir(dir);
    }
    refresh();
    #else
    mvprintw(1, 1, "GPIO support only available on Raspberry Pi");
    refresh();
    log_error(error_console, ERROR_INFO, "MINUX", 
              "GPIO support only available on Raspberry Pi");
    #endif
}

void init_windows(void) {
    // Initialize ncurses
    initscr();
    start_color();
    raw();
    noecho();
    keypad(stdscr, TRUE);
    curs_set(1);
    
    // Enable proper line drawing
    use_default_colors();
    
    // Get screen dimensions
    getmaxyx(stdscr, screen_height, screen_width);

    // Initialize status bar
    status_bar = newwin(STATUS_BAR_HEIGHT, screen_width, screen_height - 1, 0);
    keypad(status_bar, TRUE);

    // Initialize error console
    error_console = error_console_init();
    if (!error_console) {
        endwin();
        fprintf(stderr, "Failed to initialize error console\n");
        exit(1);
    }
}

void cleanup(void) {
    error_console_destroy(error_console);
    delwin(status_bar);
    endwin();
}

void draw_status_bar(void) {
    werase(status_bar);
    wattron(status_bar, A_REVERSE);
    
    // Draw base status bar
    mvwhline(status_bar, 0, 0, ' ', screen_width);
    
    // Draw current path
    mvwprintw(status_bar, 0, 1, "Path: %s", current_path);
    
    // Draw error indicator if there are errors
    int critical_count = get_error_count(error_console, ERROR_CRITICAL);
    int warning_count = get_error_count(error_console, ERROR_WARNING);
    
    if (critical_count > 0 || warning_count > 0) {
        const char *last_error = get_last_error_message(error_console);
        if (last_error) {
            int msg_len = strlen(last_error);
            if (msg_len > 40) msg_len = 40;
            mvwprintw(status_bar, 0, screen_width - msg_len - 20, 
                     "Errors: %d/%d | %.*s", critical_count, warning_count, 
                     msg_len, last_error);
        }
    }
    
    wattroff(status_bar, A_REVERSE);
    wrefresh(status_bar);
}

void update_status_bar_error(ErrorConsole *console) {
    (void)console;  // Explicitly mark console as unused
    draw_status_bar();
}

void show_prompt(void) {
    printw("MINUX> ");
    refresh();
}

void launch_explorer(void) {
    // Show splash screen with proper Unicode box drawing
    clear();
    int y = screen_height / 2 - 4;
    const char *banner[] = {
        "┌────────────────────────────────────────┐",
        "│           MINUX File Explorer          │",
        "│                                        │",
        "│              Version %s             │",
        "│                                        │",
        "│          Loading, please wait...       │",
        "└────────────────────────────────────────┘"
    };

    // Calculate center position
    int banner_width = strlen(banner[0]);
    int start_x = (screen_width - banner_width) / 2;

    // Draw the banner
    for (size_t i = 0; i < sizeof(banner) / sizeof(banner[0]); i++) {
        if (i == 3) {
            // Special handling for version string
            mvprintw(y, start_x, banner[i], VERSION);
        } else {
            mvprintw(y, start_x, "%s", banner[i]);
        }
        y++;
    }
    refresh();
    napms(1500);  // Show splash for 1.5 seconds

    // Launch explorer
    endwin();  // Temporarily end ncurses mode
    int result = system("./explorer");
    if (result != 0) {
        initscr();  // Restore ncurses mode
        log_error(error_console, ERROR_CRITICAL, "MINUX", 
                  "Failed to launch explorer (error code: %d)", result);
    } else {
        initscr();  // Restore ncurses mode
        log_error(error_console, ERROR_SUCCESS, "MINUX", 
                  "Explorer session ended successfully");
    }
    refresh();
}

void capture_image(const char *filename) {
    // Placeholder for actual camera initialization and capture logic
    printf("Capturing image: %s\n", filename);
    // Here you would add the actual code to capture the image using libcamera
    
    // For now, create an empty file to simulate capturing an image
    FILE *fp = fopen(filename, "w");
    if (fp) {
        fprintf(fp, "Simulated camera image\n");
        fclose(fp);
    }
}

void test_camera() {
    // Create a folder for test images
    system("mkdir -p test_images");

    // Display information in the terminal
    clear();
    mvprintw(1, 1, "Testing Arducam Day-Night Vision Camera");
    mvprintw(3, 1, "Capturing Daylight Image...");
    refresh();
    
    // Capture Daylight Image
    capture_image("test_images/daylight.jpg");
    
    mvprintw(4, 1, "Turning on Night Vision...");
    refresh();
    
    // Simulate Night Mode
#if __has_include(<pigpio.h>)
    system("gpio -g mode 4 out");
    system("gpio -g write 4 0"); // Disable IR-Cut Filter (Activate IR mode)
#else
    mvprintw(5, 1, "Note: GPIO control not available (pigpio not found)");
    refresh();
#endif
    sleep(2); // Allow time for adjustment

    mvprintw(6, 1, "Capturing Night Vision Image...");
    refresh();
    
    // Capture Night Vision Image
    capture_image("test_images/night_vision.jpg");
    
    mvprintw(7, 1, "Restoring Day Vision...");
    refresh();
    
    // Restore Day Mode
#if __has_include(<pigpio.h>)
    system("gpio -g write 4 1"); // Enable IR-Cut Filter (Day Mode)
#endif
    sleep(2); // Allow time for adjustment

    mvprintw(8, 1, "Capturing Restored Daylight Image...");
    refresh();
    
    // Capture restored image
    capture_image("test_images/restored_daylight.jpg");

    mvprintw(10, 1, "Test images saved in 'test_images/' directory.");
    mvprintw(11, 1, "Press any key to continue...");
    refresh();
    getch();
}

void handle_command(const char *cmd) {
    char *args[MAX_ARGS];
    char cmd_copy[MAX_CMD_LENGTH];
    strncpy(cmd_copy, cmd, MAX_CMD_LENGTH - 1);
    cmd_copy[MAX_CMD_LENGTH - 1] = '\0';
    
    // Trim leading/trailing whitespace
    char *start = cmd_copy;
    while (*start && isspace(*start)) start++;
    char *end = start + strlen(start) - 1;
    while (end > start && isspace(*end)) *end-- = '\0';
    
    if (!*start) return;  // Empty command
    
    // Handle "test camera" as a special case
    if (strcmp(start, "test camera") == 0) {
        test_camera();
        return;
    }
    
    // Split command into arguments
    int argc = 0;
    char *token = strtok(start, " ");
    while (token && argc < MAX_ARGS) {
        args[argc++] = token;
        token = strtok(NULL, " ");
    }

    // Handle commands
    if (strcmp(args[0], "exit") == 0) {
        log_error(error_console, ERROR_INFO, "MINUX", "Exiting MINUX...");
        cleanup();
        exit(0);
    }
    else if (strcmp(args[0], "ls") == 0) {
        cmd_ls(argc > 1 ? args[1] : NULL);
    }
    else if (strcmp(args[0], "cd") == 0) {
        cmd_cd(argc > 1 ? args[1] : NULL);
    }
    else {
        // Look for command in command table
        Command *best_match = NULL;
        int min_distance = MAX_CMD_LENGTH;
        
        for (Command *cmd_entry = commands; cmd_entry->name != NULL; cmd_entry++) {
            if (strcmp(args[0], cmd_entry->name) == 0) {
                if (cmd_entry->func) {
                    cmd_entry->func();
                    return;
                }
            }
            // Find closest match for typos
            int dist = levenshtein_distance(args[0], cmd_entry->name);
            if (dist < min_distance && dist <= 2) {  // Allow 2 character difference
                min_distance = dist;
                best_match = cmd_entry;
            }
        }
        
        if (best_match) {
            log_error(error_console, ERROR_WARNING, "MINUX", 
                     "Unknown command '%s'. Did you mean '%s'?", args[0], best_match->name);
        } else {
            log_error(error_console, ERROR_WARNING, "MINUX", 
                     "Unknown command: %s", args[0]);
        }
    }
}

// Helper function to calculate Levenshtein distance between two strings
static int levenshtein_distance(const char *s1, const char *s2) {
    int len1 = strlen(s1);
    int len2 = strlen(s2);
    int matrix[len1 + 1][len2 + 1];
    
    for (int i = 0; i <= len1; i++)
        matrix[i][0] = i;
    for (int j = 0; j <= len2; j++)
        matrix[0][j] = j;
        
    for (int i = 1; i <= len1; i++) {
        for (int j = 1; j <= len2; j++) {
            int cost = (s1[i-1] == s2[j-1]) ? 0 : 1;
            int deletion = matrix[i-1][j] + 1;
            int insertion = matrix[i][j-1] + 1;
            int substitution = matrix[i-1][j-1] + cost;
            matrix[i][j] = min(min(deletion, insertion), substitution);
        }
    }
    
    return matrix[len1][len2];
}

// Serial monitor implementation
static int open_serial_port(const char *port, int baud_rate) {
    (void)baud_rate;  // Mark as intentionally unused
    int fd = open(port, O_RDWR | O_NOCTTY | O_NONBLOCK);
    if (fd < 0) {
        log_error(error_console, ERROR_WARNING, "SERIAL", 
                  "Error opening port %s: %s", port, strerror(errno));
        return -1;
    }

    struct termios tio;
    if (tcgetattr(fd, &tio) < 0) {
        close(fd);
        log_error(error_console, ERROR_WARNING, "SERIAL", 
                  "Error getting port attributes: %s", strerror(errno));
        return -1;
    }

    // Save old settings
    memcpy(&serial_port.old_tio, &tio, sizeof(struct termios));

    // Configure port
    configure_serial_port(fd, &tio);

    // Apply settings
    if (tcsetattr(fd, TCSANOW, &tio) < 0) {
        close(fd);
        log_error(error_console, ERROR_WARNING, "SERIAL", 
                  "Error setting port attributes: %s", strerror(errno));
        return -1;
    }

    // Set up non-blocking I/O
    int flags = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);

    return fd;
}

static void configure_serial_port(int fd, struct termios *tio) {
    (void)fd;  // Mark as intentionally unused
    // Set baud rate
    speed_t baud;
    switch (serial_port.baud_rate) {
        case 9600: baud = B9600; break;
        case 19200: baud = B19200; break;
        case 38400: baud = B38400; break;
        case 57600: baud = B57600; break;
        case 115200: baud = B115200; break;
        case 230400: baud = B230400; break;
        default: baud = B115200; break;
    }

    cfsetispeed(tio, baud);
    cfsetospeed(tio, baud);

    // Configure other settings
    tio->c_cflag &= ~PARENB;  // No parity
    tio->c_cflag &= ~CSTOPB;  // 1 stop bit
    tio->c_cflag &= ~CSIZE;
    tio->c_cflag |= CS8;      // 8 data bits
    tio->c_cflag |= CREAD;    // Enable receiver
    tio->c_cflag |= CLOCAL;   // Ignore modem control lines
    tio->c_cflag |= CRTSCTS;  // Enable hardware flow control

    // Raw input
    tio->c_lflag &= ~ICANON;
    tio->c_lflag &= ~ECHO;
    tio->c_lflag &= ~ECHOE;
    tio->c_lflag &= ~ECHONL;
    tio->c_lflag &= ~ISIG;

    // Raw output
    tio->c_oflag &= ~OPOST;
    tio->c_oflag &= ~ONLCR;
}

static void close_serial_port(SerialPort *sp) {
    if (sp->fd >= 0) {
        // Restore old settings
        tcsetattr(sp->fd, TCSANOW, &sp->old_tio);
        close(sp->fd);
        sp->fd = -1;
        sp->is_connected = 0;
    }
}

static void handle_serial_input(SerialPort *sp) {
    char buffer[1024];
    int n = read(sp->fd, buffer, sizeof(buffer) - 1);
    if (n > 0) {
        buffer[n] = '\0';
        printw("%s", buffer);
        refresh();
    }
}

static void handle_serial_output(SerialPort *sp) {
    char buffer[1024];
    int n = read(stdin->_file, buffer, sizeof(buffer) - 1);
    if (n > 0) {
        buffer[n] = '\0';
        write(sp->fd, buffer, n);
    }
}

static void cleanup_serial(void) {
    close_serial_port(&serial_port);
}

void serial_monitor(void) {
    // Create a new window for serial monitor
    WINDOW *serial_win = newwin(screen_height - 2, screen_width, 0, 0);
    keypad(serial_win, TRUE);
    scrollok(serial_win, TRUE);

    // Show connection dialog
    clear();
    mvprintw(1, 1, "Serial Monitor Configuration");
    mvprintw(3, 1, "Enter port (e.g., /dev/ttyUSB0): ");
    refresh();

    char port[256];
    int pos = 0;
    int ch;
    while ((ch = getch()) != '\n' && pos < 255) {
        if (ch >= 32 && ch <= 126) {
            port[pos++] = ch;
            printw("%c", ch);
            refresh();
        }
    }
    port[pos] = '\0';

    // Default baud rate
    serial_port.baud_rate = 115200;

    // Try to open the port
    serial_port.fd = open_serial_port(port, serial_port.baud_rate);
    if (serial_port.fd < 0) {
        mvprintw(5, 1, "Failed to open port. Press any key to continue...");
        refresh();
        getch();
        delwin(serial_win);
        return;
    }

    strncpy(serial_port.port, port, sizeof(serial_port.port) - 1);
    serial_port.is_connected = 1;

    // Set up signal handler for cleanup
    signal(SIGINT, (void (*)(int))cleanup_serial);

    // Main serial monitor loop
    clear();
    mvprintw(0, 1, "Serial Monitor - %s @ %d baud", port, serial_port.baud_rate);
    mvprintw(1, 1, "Press Ctrl+C to exit");
    refresh();

    while (serial_port.is_connected) {
        handle_serial_input(&serial_port);
        handle_serial_output(&serial_port);
        usleep(10000);  // 10ms delay to prevent CPU hogging
    }

    delwin(serial_win);
}

int main(void) {
    // Set up locale and UTF-8 support BEFORE ncurses init
    setlocale(LC_ALL, "");
    const char *env_var = "NCURSES_NO_UTF8_ACS=1";
    putenv((char *)env_var);

    init_windows();
    getcwd(current_path, sizeof(current_path));

    // Show welcome message
    printw("MINUX %s\n", VERSION);
    printw("Type 'help' for available commands\n\n");

    char cmd[MAX_CMD_LENGTH];
    int cmd_pos = 0;
    int ch;

    show_prompt();

    while (1) {
        ch = getch();

        if (ch == '`' || ch == '~') {  // Toggle error console
            error_console_toggle(error_console);
            continue;
        }

        if (error_console->is_visible) {
            error_console_handle_input(error_console, ch);
            continue;
        }

        switch (ch) {
            case '\n':
                cmd[cmd_pos] = '\0';
                printw("\n");
                handle_command(cmd);
                cmd_pos = 0;
                show_prompt();
                break;

            case KEY_BACKSPACE:
            case 127:  // Also handle DEL key
                if (cmd_pos > 0) {
                    cmd_pos--;
                    printw("\b \b");
                    refresh();
                }
                break;

            default:
                if (cmd_pos < MAX_CMD_LENGTH - 1 && ch >= 32 && ch <= 126) {
                    cmd[cmd_pos++] = ch;
                    printw("%c", ch);
                    refresh();
                }
                break;
        }
    }

    cleanup();
    return 0;
} 