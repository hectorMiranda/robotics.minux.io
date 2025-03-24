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
#include <stddef.h>
#include <iostream>
#include <fstream>
#include <string>
#include <cstring>
#include <pwd.h>    // For getpwuid
#include <grp.h>    // For getgrgid
#include <vector>
#include <algorithm>
#include <functional> // For tree command

// Define GPIO constants for systems without pigpio
#define PI_INPUT 0
#define PI_OUTPUT 1
#define PI_ALT0 4

// Add these definitions for command history
#define MAX_HISTORY 100
#define HISTORY_FILE ".minux_history"

// Try to include pigpio only if the compiler flag indicates it's available
#ifndef HAS_PIGPIO
    #define HAS_PIGPIO 0
#endif

#if HAS_PIGPIO
    extern "C" {
        #include <pigpio.h>
    }
#endif

// First check if we're on ARM architecture (like Raspberry Pi)
#ifdef __arm__
    // Then check for libcamera
    #if defined(__has_include) && __has_include(<libcamera/libcamera.h>)
        #include <libcamera/libcamera.h>
    #endif
#endif

#define VERSION "0.0.1"
#define MAX_CMD_LENGTH 256
#define MAX_ARGS 32
#define MAX_PATH 4096
#define STATUS_BAR_HEIGHT 1

// Forward declarations for all functions (add these before they're used)
static inline int min(int a, int b) {
    return (a < b) ? a : b;
}

// Serial communication structure
typedef struct {
    int fd;
    char device[256];
    int baud_rate;
    struct termios old_tio;
    FILE* port;
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
void cmd_tree(void);
void cmd_tree_interactive(void); // Add separate function for interactive mode
void cmd_cat(const char *filepath); // Add cat command prototype
void cmd_history(void);
void cmd_log(const char *message);
void add_to_history(const char *cmd);
void load_history(void);
void save_history(void);
char *get_history_entry(int index);
int history_size();

// Function declarations that were missing
void init_windows(void);
void cleanup(void);
void draw_status_bar(void);
void draw_error_status_bar(const char *error_msg);
void handle_command(const char *cmd);
void show_prompt(void);
void view_file_contents(const char *filepath);
static int levenshtein_distance(const char *s1, const char *s2);

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
    {"tree", cmd_tree, "Display directory structure in a tree-like format"},
    {"cat", NULL, "Display file contents"},   // Special handling for args
    {"history", cmd_history, "Display command history"},  // Add history command
    {"log", NULL, "Add entry to log file"},   // Special handling for args
    {NULL, NULL, NULL}
};

// Global variables
char current_path[MAX_PATH];
ErrorConsole *error_console = NULL;
WINDOW *status_bar;
int screen_width, screen_height;
SerialPort serial_port = {
    .fd = -1,
    .device = {0},
    .baud_rate = 115200,
    .old_tio = {},
    .port = NULL,
    .is_connected = 0
};

// Add these global variables with other globals
char *command_history[MAX_HISTORY];
int history_count = 0;
int history_position = -1;

// Define the GPIO pin information structure
typedef struct {
    int bcm;       // BCM pin number
    int wPi;       // WiringPi pin number
    const char *name;    // Pin name
    int physical; // Physical pin number
} PinInfo;

// GPIO pin information for Raspberry Pi 3B
PinInfo pinInfoTable[] = {
    {2, 8, "SDA.1", 3},
    {3, 9, "SCL.1", 5},
    {4, 7, "GPIO. 7", 7},
    {14, 15, "TxD", 8},
    {15, 16, "RxD", 10},
    {17, 0, "GPIO. 0", 11},
    {18, 1, "GPIO. 1", 12},
    {27, 2, "GPIO. 2", 13},
    {22, 3, "GPIO. 3", 15},
    {23, 4, "GPIO. 4", 16},
    {24, 5, "GPIO. 5", 18},
    {10, 12, "MOSI", 19},
    {9, 13, "MISO", 21},
    {25, 6, "GPIO. 6", 22},
    {11, 14, "SCLK", 23},
    {8, 10, "CE0", 24},
    {7, 11, "CE1", 26},
    {0, 30, "SDA.0", 27},
    {1, 31, "SCL.0", 28},
    {5, 21, "GPIO.21", 29},
    {6, 22, "GPIO.22", 31},
    {12, 26, "GPIO.26", 32},
    {13, 23, "GPIO.23", 33},
    {19, 24, "GPIO.24", 35},
    {16, 27, "GPIO.27", 36},
    {26, 25, "GPIO.25", 37},
    {20, 28, "GPIO.28", 38},
    {21, 29, "GPIO.29", 40},
    {-1, -1, NULL, -1}  // End marker
};

void display_welcome_banner() {
    std::cout << "\n";
    std::cout << "███╗   ███╗██╗███╗   ██╗██╗   ██╗██╗  ██╗\n";
    std::cout << "████╗ ████║██║████╗  ██║██║   ██║╚██╗██╔╝\n";
    std::cout << "██╔████╔██║██║██╔██╗ ██║██║   ██║ ╚███╔╝ \n";
    std::cout << "██║╚██╔╝██║██║██║╚██╗██║██║   ██║ ██╔██╗ \n";
    std::cout << "██║ ╚═╝ ██║██║██║ ╚████║╚██████╔╝██╔╝ ██╗\n";
    std::cout << "╚═╝     ╚═╝╚═╝╚═╝  ╚═══╝ ╚═════╝ ╚═╝  ╚═╝\n";
    std::cout << "  Minimalist Unix-like Shell for Embedded Systems\n";
    std::cout << "  Version 1.0.0\n\n";

    // Display system information
    std::cout << "=== System Information ===\n";

    // OS information
    std::ifstream os_release("/etc/os-release");
    if (os_release.is_open()) {
        std::string line;
        while (std::getline(os_release, line)) {
            if (line.substr(0, 12) == "PRETTY_NAME=") {
                // Remove quotes if present
                std::string name = line.substr(12);
                if (name[0] == '"') {
                    name = name.substr(1, name.find_last_of('"') - 1);
                }
                std::cout << "OS: " << name << std::endl;
                break;
            }
        }
        os_release.close();
    } else {
        // Try uname if os-release doesn't exist
        FILE *uname_cmd = popen("uname -a", "r");
        if (uname_cmd) {
            char uname_output[256];
            if (fgets(uname_output, sizeof(uname_output), uname_cmd)) {
                std::cout << "System: " << uname_output;
            }
            pclose(uname_cmd);
        }
    }

    // CPU information
    std::ifstream cpuinfo("/proc/cpuinfo");
    if (cpuinfo.is_open()) {
        std::string line;
        bool found_model = false;
        while (std::getline(cpuinfo, line) && !found_model) {
            if (line.substr(0, 10) == "model name" || 
                line.substr(0, 8) == "Hardware" || 
                line.substr(0, 5) == "Model") {
                size_t colon_pos = line.find(':');
                if (colon_pos != std::string::npos) {
                    std::cout << "CPU:" << line.substr(colon_pos + 1) << std::endl;
                    found_model = true;
                }
            }
        }
        cpuinfo.close();
    }

    // Memory information
    std::ifstream meminfo("/proc/meminfo");
    if (meminfo.is_open()) {
        std::string line;
        while (std::getline(meminfo, line)) {
            if (line.substr(0, 9) == "MemTotal:") {
                std::cout << "Memory: " << line.substr(9) << std::endl;
                break;
            }
        }
        meminfo.close();
    }

    // Check if running on Raspberry Pi
    std::ifstream model("/sys/firmware/devicetree/base/model");
    if (model.is_open()) {
        std::string model_str;
        std::getline(model, model_str);
        if (model_str.find("Raspberry Pi") != std::string::npos) {
            std::cout << "Platform: Raspberry Pi detected\n";
            std::cout << "GPIO Support: Available\n";
        }
        model.close();
    } else {
        std::cout << "Platform: Not running on Raspberry Pi\n";
        std::cout << "GPIO Support: Not available\n";
    }

    std::cout << std::endl;
}

// Command implementations
void cmd_help(void) {
    // Don't clear screen, just add a blank line for separation
    printw("\n");
    int y = getcury(stdscr);
    move(y, 0);  // Move to start of line
    
    printw("MINUX Commands:\n\n");
    for (Command *cmd = commands; cmd->name != NULL; cmd++) {
        printw("  %-15s - %s\n", cmd->name, cmd->help);
    }
    printw("\n");
    refresh();
}

void cmd_version(void) {
    printw("\nMINUX Version %s\n\n", VERSION);
    refresh();
}

void cmd_time(void) {
    time_t t = time(NULL);
    struct tm *tm = localtime(&t);
    char time_str[64];
    strftime(time_str, sizeof(time_str), "%H:%M:%S", tm);
    printw("\nCurrent time: %s\n\n", time_str);
    refresh();
}

void cmd_date(void) {
    time_t t = time(NULL);
    struct tm *tm = localtime(&t);
    char date_str[64];
    strftime(date_str, sizeof(date_str), "%Y-%m-%d", tm);
    printw("\nCurrent date: %s\n\n", date_str);
    refresh();
}

void cmd_path(void) {
    char *path = getenv("PATH");
    printw("\n");
    if (path) {
        printw("System PATH:\n\n");
        char path_copy[4096];
        strncpy(path_copy, path, sizeof(path_copy) - 1);
        path_copy[sizeof(path_copy) - 1] = '\0';
        
        char *token = strtok(path_copy, ":");
        while (token != NULL) {
            printw("  %s\n", token);
            token = strtok(NULL, ":");
        }
    } else {
        printw("PATH environment variable not found\n");
    }
    printw("\n");
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

    // Add a blank line for separation
    printw("\nContents of %s:\n\n", target_path);

    // Initialize color pairs for ls
    init_pair(1, COLOR_BLUE, COLOR_BLACK);   // Directories
    init_pair(2, COLOR_GREEN, COLOR_BLACK);  // Executables
    init_pair(3, COLOR_CYAN, COLOR_BLACK);   // Symlinks
    
    // Print header row
    printw("%-10s %-8s %-8s %8s %-12s %s\n", "Permissions", "Owner", "Group", "Size", "Modified", "Name");
    printw("--------------------------------------------------------------------------\n");

    // Sort entries - we'll use a simple array for now
    struct dirent *entries[1024]; // Assuming no more than 1024 entries
    int entry_count = 0;
    
    while ((entry = readdir(dir)) != NULL && entry_count < 1024) {
        entries[entry_count++] = entry;
    }
    
    // Simple bubble sort to order entries (directories first, then files)
    for (int i = 0; i < entry_count - 1; i++) {
        for (int j = 0; j < entry_count - i - 1; j++) {
            struct stat st_j, st_j1;
            char full_path_j[MAX_PATH], full_path_j1[MAX_PATH];
            
            snprintf(full_path_j, sizeof(full_path_j), "%s/%s", target_path, entries[j]->d_name);
            snprintf(full_path_j1, sizeof(full_path_j1), "%s/%s", target_path, entries[j+1]->d_name);
            
            stat(full_path_j, &st_j);
            stat(full_path_j1, &st_j1);
            
            // Directories come first, then sort alphabetically
            bool j_is_dir = S_ISDIR(st_j.st_mode);
            bool j1_is_dir = S_ISDIR(st_j1.st_mode);
            
            if ((j_is_dir && !j1_is_dir) || 
                (j_is_dir == j1_is_dir && strcasecmp(entries[j]->d_name, entries[j+1]->d_name) > 0)) {
                // Swap
                struct dirent *temp = entries[j];
                entries[j] = entries[j+1];
                entries[j+1] = temp;
            }
        }
    }
    
    // Display entries
    for (int i = 0; i < entry_count; i++) {
        struct stat st;
        char full_path[MAX_PATH];
        snprintf(full_path, sizeof(full_path), "%s/%s", target_path, entries[i]->d_name);
        
        if (stat(full_path, &st) == 0) {
            // Format permissions
            char perm[11];
            perm[0] = S_ISDIR(st.st_mode) ? 'd' : (S_ISLNK(st.st_mode) ? 'l' : '-');
            perm[1] = (st.st_mode & S_IRUSR) ? 'r' : '-';
            perm[2] = (st.st_mode & S_IWUSR) ? 'w' : '-';
            perm[3] = (st.st_mode & S_IXUSR) ? 'x' : '-';
            perm[4] = (st.st_mode & S_IRGRP) ? 'r' : '-';
            perm[5] = (st.st_mode & S_IWGRP) ? 'w' : '-';
            perm[6] = (st.st_mode & S_IXGRP) ? 'x' : '-';
            perm[7] = (st.st_mode & S_IROTH) ? 'r' : '-';
            perm[8] = (st.st_mode & S_IWOTH) ? 'w' : '-';
            perm[9] = (st.st_mode & S_IXOTH) ? 'x' : '-';
            perm[10] = '\0';
            
            // Get owner and group
            char owner[32] = "unknown";
            char group[32] = "unknown";
            
            // Try to get username and group name - handle gracefully if functions aren't available
            struct passwd *pw = getpwuid(st.st_uid);
            struct group *gr = getgrgid(st.st_gid);
            
            if (pw != NULL) {
                strncpy(owner, pw->pw_name, sizeof(owner) - 1);
                owner[sizeof(owner) - 1] = '\0'; // Ensure null termination
            } else {
                // Just use the numeric UID if lookup fails
                snprintf(owner, sizeof(owner), "%d", st.st_uid);
            }
            
            if (gr != NULL) {
                strncpy(group, gr->gr_name, sizeof(group) - 1);
                group[sizeof(group) - 1] = '\0'; // Ensure null termination
            } else {
                // Just use the numeric GID if lookup fails
                snprintf(group, sizeof(group), "%d", st.st_gid);
            }
            
            // Format size
            char size_str[32];
            if (st.st_size < 1024) {
                snprintf(size_str, sizeof(size_str), "%d", (int)st.st_size);
            } else if (st.st_size < 1024 * 1024) {
                snprintf(size_str, sizeof(size_str), "%.1fK", st.st_size / 1024.0);
            } else if (st.st_size < 1024 * 1024 * 1024) {
                snprintf(size_str, sizeof(size_str), "%.1fM", st.st_size / (1024.0 * 1024.0));
            } else {
                snprintf(size_str, sizeof(size_str), "%.1fG", st.st_size / (1024.0 * 1024.0 * 1024.0));
            }
            
            // Format time
            char time_str[32];
            struct tm *tm = localtime(&st.st_mtime);
            strftime(time_str, sizeof(time_str), "%b %d %H:%M", tm);
            
            // Apply appropriate color based on file type
            if (S_ISDIR(st.st_mode)) {
                attron(COLOR_PAIR(1) | A_BOLD);
            } else if (st.st_mode & S_IXUSR) {
                attron(COLOR_PAIR(2) | A_BOLD);
            } else if (S_ISLNK(st.st_mode)) {
                attron(COLOR_PAIR(3) | A_BOLD);
            }
            
            // Print the entry details
            printw("%-10s %-8s %-8s %8s %-12s %s\n", 
                    perm, owner, group, size_str, time_str, entries[i]->d_name);
            
            // Restore normal attributes
            attroff(COLOR_PAIR(1) | COLOR_PAIR(2) | COLOR_PAIR(3) | A_BOLD);
        }
    }
    
    printw("\n");  // Add a blank line after the listing
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
    // This command is meant to clear the screen, so keep the clear() call
    clear();
    refresh();
    show_prompt();
}

void cmd_gpio(void) {
    // Keep clear for this command as it's a full-screen display
    clear();
    
    // Rest of the function remains unchanged
    // ... existing code ...
}

// These commands still clear the screen because they are more interactive or full-screen:
// launch_explorer
// test_camera
// serial_monitor
// cmd_tree and cmd_tree_interactive

// Update handle_command to not clear before running the tree command
void handle_command(const char *cmd) {
    // Add to history if not empty
    if (cmd && *cmd) {
        add_to_history(cmd);
    }
    
    char *args[MAX_ARGS];
    char cmd_copy[MAX_CMD_LENGTH];
    strncpy(cmd_copy, cmd, MAX_CMD_LENGTH - 1);
    cmd_copy[MAX_CMD_LENGTH - 1] = '\0';
    
    // Trim leading/trailing whitespace
    char *start = cmd_copy;
    while (*start && isspace(*start)) start++;
    char *end = start + strlen(start) - 1;
    while (end > start && isspace(*end)) *end-- = '\0';
    
    if (!*start) {
        // Empty command, just show the prompt
        show_prompt();
        return;  
    }
    
    // Handle "test camera" as a special case
    if (strcmp(start, "test camera") == 0) {
        test_camera();
        show_prompt();
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
        show_prompt();
    }
    else if (strcmp(args[0], "cd") == 0) {
        cmd_cd(argc > 1 ? args[1] : NULL);
        show_prompt();
    }
    else if (strcmp(args[0], "cat") == 0) {
        cmd_cat(argc > 1 ? args[1] : NULL);
        show_prompt();
    }
    else if (strcmp(args[0], "tree") == 0) {
        // Special handling for tree command with arguments - add printw here instead of clear()
        printw("\n");
        
        // Default settings
        const char *path = ".";  // Default to current directory
        bool show_hidden = false;
        int max_depth = -1;  // -1 means unlimited
        bool interactive = false;
        
        // Parse any provided arguments
        for (int i = 1; i < argc; i++) {
            if (strcmp(args[i], "-i") == 0) {
                interactive = true;
                // Interactive mode does need a clear screen
                clear();
                break;
            } else if (strcmp(args[i], "-a") == 0 || strcmp(args[i], "--all") == 0) {
                show_hidden = true;
            } else if (strcmp(args[i], "-L") == 0 && i + 1 < argc) {
                max_depth = atoi(args[i + 1]);
                i++;  // Skip the next argument which is the depth number
            } else if (args[i][0] != '-') {
                path = args[i];
            }
        }
        
        if (interactive) {
            cmd_tree_interactive();
        } else {
            // Open the directory
            DIR *dir = opendir(path);
            if (!dir) {
                log_error(error_console, ERROR_WARNING, "MINUX", 
                        "Error opening directory '%s': %s", path, strerror(errno));
                show_prompt();
                return;
            }
            
            // Print header
            char resolved_path[PATH_MAX];
            if (realpath(path, resolved_path) == NULL) {
                strncpy(resolved_path, path, PATH_MAX);
            }
            
            // Display path
            attron(COLOR_PAIR(1) | A_BOLD);
            printw("%s\n", resolved_path);
            attroff(COLOR_PAIR(1) | A_BOLD);
            
            // Use recursive display for Unix-like functionality
            int dir_count = 0;
            int file_count = 0;
            
            // Adjust the display_tree_recursive function to use printw instead of mvprintw
            std::function<void(const char*, int, const char*, bool)> display_tree_recursive = 
                [&](const char* dir_path, int depth, const char* prefix, bool is_last) {
                    if (max_depth > 0 && depth > max_depth) {
                        return;
                    }
                    
                    DIR *d = opendir(dir_path);
                    if (!d) return;
                    
                    // Collect and sort entries
                    std::vector<std::string> entries;
                    std::vector<bool> is_dirs;
                    struct dirent *entry;
                    
                    while ((entry = readdir(d)) != NULL) {
                        // Skip . and .. entries
                        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
                            continue;
                        }
                        
                        // Skip hidden files if not showing them
                        if (!show_hidden && entry->d_name[0] == '.') {
                            continue;
                        }
                        
                        char full_path[MAX_PATH];
                        snprintf(full_path, sizeof(full_path), "%s/%s", dir_path, entry->d_name);
                        
                        struct stat st;
                        if (stat(full_path, &st) != 0) continue;
                        
                        entries.push_back(entry->d_name);
                        is_dirs.push_back(S_ISDIR(st.st_mode));
                    }
                    
                    closedir(d);
                    
                    // Sort entries
                    std::vector<size_t> indices(entries.size());
                    for (size_t i = 0; i < indices.size(); i++) {
                        indices[i] = i;
                    }
                    
                    std::sort(indices.begin(), indices.end(), [&](size_t a, size_t b) {
                        if (is_dirs[a] != is_dirs[b]) {
                            return is_dirs[a] > is_dirs[b]; // Directories first
                        }
                        return entries[a] < entries[b]; // Then alphabetically
                    });
                    
                    // Process entries
                    for (size_t i = 0; i < indices.size(); i++) {
                        // No need to check for screen overflow in sequential mode
                        
                        size_t idx = indices[i];
                        const std::string& name = entries[idx];
                        bool is_directory = is_dirs[idx];
                        bool is_entry_last = (i == indices.size() - 1);
                        
                        char full_path[MAX_PATH];
                        snprintf(full_path, sizeof(full_path), "%s/%s", dir_path, name.c_str());
                        
                        struct stat st;
                        if (stat(full_path, &st) != 0) continue;
                        
                        // Create new prefix for children
                        std::string new_prefix = prefix;
                        new_prefix += is_last ? "    " : "|   ";
                        
                        // Display current entry
                        printw("%s%s", prefix, is_entry_last ? "`-- " : "|-- ");
                        
                        if (is_directory) {
                            attron(COLOR_PAIR(1) | A_BOLD);
                            printw("%s\n", name.c_str());
                            attroff(COLOR_PAIR(1) | A_BOLD);
                            dir_count++;
                            
                            // Recurse into directory
                            display_tree_recursive(full_path, depth + 1, new_prefix.c_str(), is_entry_last);
                        } else {
                            if (st.st_mode & S_IXUSR) {
                                attron(COLOR_PAIR(2) | A_BOLD);
                                printw("%s\n", name.c_str());
                                attroff(COLOR_PAIR(2) | A_BOLD);
                            } else if (S_ISLNK(st.st_mode)) {
                                attron(COLOR_PAIR(3) | A_BOLD);
                                printw("%s\n", name.c_str());
                                attroff(COLOR_PAIR(3) | A_BOLD);
                            } else {
                                printw("%s\n", name.c_str());
                            }
                            file_count++;
                        }
                    }
                };
            
            // Start with the root entry
            printw(".\n");
            
            // Start recursive display
            display_tree_recursive(path, 1, "", true);
            
            // Print summary
            printw("\n%d directories, %d files\n", dir_count, file_count);
            printw("\n");  // Extra line for spacing
            refresh();
            closedir(dir);
        }
        show_prompt();
    }
    else if (strcmp(args[0], "history") == 0) {
        cmd_history();
        show_prompt();
    }
    else if (strcmp(args[0], "log") == 0) {
        // If there are arguments, combine them into a single message
        if (argc > 1) {
            // Get the original command string and extract everything after "log "
            const char *message = cmd + 4;
            while (isspace(*message)) message++;
            cmd_log(message);
        } else {
            cmd_log(NULL);  // No arguments
        }
        show_prompt();
    }
    else {
        // Look for command in command table
        Command *best_match = NULL;
        int min_distance = MAX_CMD_LENGTH;
        
        for (Command *cmd_entry = commands; cmd_entry->name != NULL; cmd_entry++) {
            if (strcmp(args[0], cmd_entry->name) == 0) {
                if (cmd_entry->func) {
                    cmd_entry->func();
                    show_prompt();
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
        show_prompt();
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
            matrix[i][j] = std::min(std::min(deletion, insertion), substitution);
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
    int n = read(fileno(stdin), buffer, sizeof(buffer) - 1);
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

    strncpy(serial_port.device, port, sizeof(serial_port.device) - 1);
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

// Implement the tree command
void cmd_tree(void) {
    // Check if arguments include -i for interactive mode
    char cmd_copy[MAX_CMD_LENGTH];
    strcpy(cmd_copy, "tree");  // Default command name
    
    char args[MAX_CMD_LENGTH] = {0};
    int pos = 0;
    int ch;
    int y = 1;
    
    // Get command line arguments (if any)
    mvprintw(y, 1, "Arguments (optional): ");
    refresh();
    
    while ((ch = getch()) != '\n' && pos < MAX_CMD_LENGTH - 1) {
        if (ch >= 32 && ch <= 126) { // Printable characters
            args[pos++] = ch;
            printw("%c", ch);
        } else if (ch == KEY_BACKSPACE || ch == 127) { // Backspace
            if (pos > 0) {
                pos--;
                printw("\b \b"); // Erase character
            }
        }
        refresh();
    }
    args[pos] = '\0';
    
    // Check if user wants interactive mode
    if (strstr(args, "-i") != NULL) {
        cmd_tree_interactive();
        return;
    }
    
    // Non-interactive mode with default or provided args
    clear();
    
    // Parse any arguments
    char *argv[MAX_ARGS];
    int argc = 1; // Start with the command itself
    argv[0] = cmd_copy;
    
    char *token = strtok(args, " ");
    while (token != NULL && argc < MAX_ARGS) {
        argv[argc++] = token;
        token = strtok(NULL, " ");
    }
    
    // Default settings
    const char *path = ".";  // Default to current directory
    bool show_hidden = false;
    int max_depth = -1;  // -1 means unlimited
    
    // Parse any provided arguments
    for (int i = 0; i < argc; i++) {
        if (i < argc && (strcmp(argv[i], "-a") == 0 || strcmp(argv[i], "--all") == 0)) {
            show_hidden = true;
        } else if (i < argc-1 && strcmp(argv[i], "-L") == 0) {
            max_depth = atoi(argv[i + 1]);
            i++;  // Skip the next argument which is the depth number
        } else if (i > 0 && argv[i][0] != '-') {
            path = argv[i];
        }
    }
    
    // Open the directory
    DIR *dir = opendir(path);
    if (!dir) {
        mvprintw(1, 1, "Error: Cannot open directory '%s': %s", path, strerror(errno));
        refresh();
        getch();
        return;
    }
    
    // Print header
    char resolved_path[PATH_MAX];
    if (realpath(path, resolved_path) == NULL) {
        strncpy(resolved_path, path, PATH_MAX);
    }
    
    // Display path
    attron(COLOR_PAIR(1) | A_BOLD);
    mvprintw(1, 1, "%s", resolved_path);
    attroff(COLOR_PAIR(1) | A_BOLD);
    
    // Reset position for tree display
    y = 3;
    
    // Use recursive display for Unix-like functionality
    int dir_count = 0;
    int file_count = 0;
    
    // Display the directory tree
    mvprintw(y++, 1, ".");
    
    // Display tree recursively using a dedicated function
    std::function<void(const char*, int, const char*, bool)> display_tree_recursive = 
    [&](const char* dir_path, int depth, const char* prefix, bool is_last) {
        if (max_depth > 0 && depth > max_depth) {
            return;
        }
        
        DIR *d = opendir(dir_path);
        if (!d) return;
        
        // Collect and sort entries
        std::vector<std::string> entries;
        std::vector<bool> is_dirs;
        struct dirent *entry;
        
        while ((entry = readdir(d)) != NULL) {
            // Skip . and .. entries
            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
                continue;
            }
            
            // Skip hidden files if not showing them
            if (!show_hidden && entry->d_name[0] == '.') {
                continue;
            }
            
            char full_path[MAX_PATH];
            snprintf(full_path, sizeof(full_path), "%s/%s", dir_path, entry->d_name);
            
            struct stat st;
            if (stat(full_path, &st) != 0) continue;
            
            entries.push_back(entry->d_name);
            is_dirs.push_back(S_ISDIR(st.st_mode));
        }
        
        closedir(d);
        
        // Sort entries
        std::vector<size_t> indices(entries.size());
        for (size_t i = 0; i < entries.size(); i++) {
            indices[i] = i;
        }
        
        std::sort(indices.begin(), indices.end(), [&](size_t a, size_t b) {
            if (is_dirs[a] != is_dirs[b]) {
                return is_dirs[a] > is_dirs[b]; // Directories first
            }
            return entries[a] < entries[b]; // Then alphabetically
        });
        
        // Process entries
        for (size_t i = 0; i < indices.size(); i++) {
            if (y >= LINES - 5) {
                mvprintw(y++, 1, "... (more items not shown)");
                return;
            }
            
            size_t idx = indices[i];
            const std::string& name = entries[idx];
            bool is_directory = is_dirs[idx];
            bool is_entry_last = (i == indices.size() - 1);
            
            char full_path[MAX_PATH];
            snprintf(full_path, sizeof(full_path), "%s/%s", dir_path, name.c_str());
            
            struct stat st;
            if (stat(full_path, &st) != 0) continue;
            
            // Create new prefix for children
            std::string new_prefix = prefix;
            new_prefix += is_last ? "    " : "|   ";
            
            // Display current entry
            mvprintw(y, 1, "%s%s", prefix, is_entry_last ? "`-- " : "|-- ");
            
            if (is_directory) {
                attron(COLOR_PAIR(1) | A_BOLD);
                mvprintw(y, 1 + strlen(prefix) + 4, "%s", name.c_str());
                attroff(COLOR_PAIR(1) | A_BOLD);
                dir_count++;
                y++;
                
                // Recurse into directory
                display_tree_recursive(full_path, depth + 1, new_prefix.c_str(), is_entry_last);
            } else {
                if (st.st_mode & S_IXUSR) {
                    attron(COLOR_PAIR(2) | A_BOLD);
                    mvprintw(y, 1 + strlen(prefix) + 4, "%s", name.c_str());
                    attroff(COLOR_PAIR(2) | A_BOLD);
                } else if (S_ISLNK(st.st_mode)) {
                    attron(COLOR_PAIR(3) | A_BOLD);
                    mvprintw(y, 1 + strlen(prefix) + 4, "%s", name.c_str());
                    attroff(COLOR_PAIR(3) | A_BOLD);
                } else {
                    mvprintw(y, 1 + strlen(prefix) + 4, "%s", name.c_str());
                }
                file_count++;
                y++;
            }
        }
    };
    
    // Start recursive display
    display_tree_recursive(path, 1, "", true);
    
    // Print summary
    mvprintw(y + 1, 1, "\n%d directories, %d files", dir_count, file_count);
    
    // Instructions to continue
    mvprintw(y + 3, 1, "Press any key to continue...");
    refresh();
    getch();
}

// Interactive version of the tree command
void cmd_tree_interactive(void) {
    clear();
    
    // Get arguments from user
    mvprintw(1, 1, "Tree Command - Directory Structure Viewer");
    mvprintw(3, 1, "Usage: tree [options] [directory]");
    mvprintw(4, 1, "Options:");
    mvprintw(5, 1, "  -a, --all    Show hidden files");
    mvprintw(6, 1, "  -L LEVEL     Limit display to LEVEL directories deep");
    mvprintw(7, 1, "  -i           Interactive mode (this screen)");
    mvprintw(9, 1, "Examples:");
    mvprintw(10, 1, "  tree            - Show current directory structure");
    mvprintw(11, 1, "  tree /etc       - Show structure of /etc");
    mvprintw(12, 1, "  tree -a         - Show all files including hidden ones");
    mvprintw(13, 1, "  tree -L 2       - Limit depth to 2 levels");
    mvprintw(15, 1, "Enter path (default is current directory): ");
    refresh();
    
    // Get user input for path
    char path_input[MAX_PATH] = {0};
    int ch;
    int pos = 0;
    
    // Allow user to input path or just press Enter for default
    while ((ch = getch()) != '\n' && pos < MAX_PATH - 1) {
        if (ch >= 32 && ch <= 126) { // Printable characters
            path_input[pos++] = ch;
            printw("%c", ch);
        } else if (ch == KEY_BACKSPACE || ch == 127) { // Backspace
            if (pos > 0) {
                pos--;
                printw("\b \b"); // Erase character
            }
        }
        refresh();
    }
    path_input[pos] = '\0';
    
    // Use current directory if no path entered
    const char *path = (path_input[0] == '\0') ? "." : path_input;
    
    // Collect options (simplified for this implementation)
    bool show_hidden = false;
    int max_depth = -1; // -1 means unlimited
    
    // Ask about hidden files
    mvprintw(17, 1, "Show hidden files? (y/n): ");
    refresh();
    ch = getch();
    show_hidden = (ch == 'y' || ch == 'Y');
    printw("%c", ch);
    
    // Ask about depth limit
    mvprintw(19, 1, "Limit depth? (Enter number or 0 for unlimited): ");
    refresh();
    char depth_input[10] = {0};
    pos = 0;
    
    while ((ch = getch()) != '\n' && pos < 9) {
        if (ch >= '0' && ch <= '9') {
            depth_input[pos++] = ch;
            printw("%c", ch);
        } else if (ch == KEY_BACKSPACE || ch == 127) {
            if (pos > 0) {
                pos--;
                printw("\b \b");
            }
        }
        refresh();
    }
    depth_input[pos] = '\0';
    
    if (depth_input[0] != '\0') {
        max_depth = atoi(depth_input);
        if (max_depth == 0) max_depth = -1; // Convert 0 to unlimited
    }
    
    // Clear screen and start displaying tree
    clear();
    
    DIR *dir = opendir(path);
    if (!dir) {
        mvprintw(1, 1, "Error: Cannot open directory '%s': %s", path, strerror(errno));
        refresh();
        getch();
        return;
    }
    
    // Print header
    char resolved_path[PATH_MAX];
    if (realpath(path, resolved_path) == NULL) {
        strncpy(resolved_path, path, PATH_MAX);
    }
    
    attron(COLOR_PAIR(1) | A_BOLD);
    mvprintw(1, 1, "%s", resolved_path);
    attroff(COLOR_PAIR(1) | A_BOLD);
    
    // Counters for summary
    int dir_count = 0;
    int file_count = 0;
    
    // Current display position
    int y = 3;
    
    // Helper function to display tree (simple version without recursion)
    // Instead, we'll just display the top level for simplicity
    struct dirent *entry;
    
    // Collect all entries for sorting
    std::vector<std::string> entries;
    std::vector<bool> is_dir;
    
    while ((entry = readdir(dir)) != NULL) {
        // Skip . and .. entries
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        
        // Skip hidden files if not showing them
        if (!show_hidden && entry->d_name[0] == '.') {
            continue;
        }
        
        char full_path[MAX_PATH];
        snprintf(full_path, sizeof(full_path), "%s/%s", path, entry->d_name);
        
        struct stat st;
        if (stat(full_path, &st) != 0) continue;
        
        entries.push_back(entry->d_name);
        is_dir.push_back(S_ISDIR(st.st_mode));
    }
    
    // Sort entries (directories first, then alphabetically)
    std::vector<size_t> indices(entries.size());
    for (size_t i = 0; i < entries.size(); i++) {
        indices[i] = i;
    }
    
    std::sort(indices.begin(), indices.end(), [&](size_t a, size_t b) {
        if (is_dir[a] != is_dir[b]) {
            return is_dir[a] > is_dir[b]; // Directories first
        }
        return entries[a] < entries[b]; // Then alphabetically
    });
    
    // Display entries with tree-like structure
    for (size_t i = 0; i < indices.size(); i++) {
        size_t idx = indices[i];
        const std::string& name = entries[idx];
        bool is_directory = is_dir[idx];
        
        char full_path[MAX_PATH];
        snprintf(full_path, sizeof(full_path), "%s/%s", path, name.c_str());
        
        struct stat st;
        if (stat(full_path, &st) != 0) continue;
        
        // Display with appropriate prefix and color
        bool is_last = (i == indices.size() - 1);
        
        if (is_last) {
            mvprintw(y, 1, "`-- ");
        } else {
            mvprintw(y, 1, "|-- ");
        }
        
        if (is_directory) {
            attron(COLOR_PAIR(1) | A_BOLD); // Blue for directories
            mvprintw(y, 5, "%s/", name.c_str());
            attroff(COLOR_PAIR(1) | A_BOLD);
            dir_count++;
        } else {
            // Check if it's executable
            if (st.st_mode & S_IXUSR) {
                attron(COLOR_PAIR(2) | A_BOLD);
                mvprintw(y, 5, "%s", name.c_str());
                attroff(COLOR_PAIR(2) | A_BOLD);
            } else {
                mvprintw(y, 5, "%s", name.c_str());
            }
            file_count++;
        }
        
        y++;
        
        // Limit display to fit on screen
        if (y >= LINES - 5) {
            mvprintw(y++, 1, "... (more items not shown)");
            break;
        }
    }
    
    closedir(dir);
    
    // Print summary
    mvprintw(y + 1, 1, "\n%d directories, %d files", dir_count, file_count);
    
    // Instructions to continue
    mvprintw(y + 3, 1, "Press any key to continue...");
    refresh();
    getch();
}

// Function implementations for previously undefined functions
void init_windows(void) {
    // Initialize ncurses
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    
    // Get screen dimensions
    getmaxyx(stdscr, screen_height, screen_width);
    
    // Enable scrolling for the main window
    scrollok(stdscr, TRUE);
    
    // Initialize colors if terminal supports it
    if (has_colors()) {
        start_color();
        use_default_colors();
        
        // Initialize color pairs for different file types
        init_pair(1, COLOR_BLUE, COLOR_BLACK);    // Directories
        init_pair(2, COLOR_GREEN, COLOR_BLACK);   // Executables
        init_pair(3, COLOR_CYAN, COLOR_BLACK);    // Symlinks
        init_pair(4, COLOR_YELLOW, COLOR_BLACK);  // For status bar/highlights
        init_pair(5, COLOR_RED, COLOR_BLACK);     // For errors/warnings
    }
    
    // Create status bar at the bottom of the screen
    status_bar = newwin(STATUS_BAR_HEIGHT, screen_width, screen_height - STATUS_BAR_HEIGHT, 0);
    
    // Initialize error console - using the correct API
    error_console = error_console_init();
    if (!error_console) {
        endwin();
        fprintf(stderr, "Error: Failed to create error console\n");
        exit(1);
    }
    
    // Refresh windows
    refresh();
    draw_status_bar();
}

void cleanup(void) {
    // Free command history
    for (int i = 0; i < history_count; i++) {
        if (command_history[i]) {
            free(command_history[i]);
            command_history[i] = NULL;
        }
    }
    
    // Destroy error console
    error_console_destroy(error_console);
    endwin();
    
    // Cleanup serial port if it was opened
    cleanup_serial();
    
    printf("Thank you for using MINUX!\n");
}

void draw_status_bar(void) {
    // Get current time for status bar
    time_t t = time(NULL);
    struct tm *tm = localtime(&t);
    char time_str[32];
    strftime(time_str, sizeof(time_str), "%H:%M:%S", tm);
    
    // Clear status bar
    werase(status_bar);
    
    // Draw status bar content
    wattron(status_bar, A_REVERSE);
    for (int i = 0; i < screen_width; i++) {
        mvwaddch(status_bar, 0, i, ' ');
    }
    
    // Show current path and time on the status bar
    mvwprintw(status_bar, 0, 1, "MINUX v%s | Path: %s", VERSION, current_path);
    mvwprintw(status_bar, 0, screen_width - 10, "%s", time_str);
    
    wattroff(status_bar, A_REVERSE);
    wrefresh(status_bar);
}

void show_prompt(void) {
    // Draw updated status bar
    draw_status_bar();
    
    // Show command prompt
    printw("\nminux:%s$ ", current_path);
    refresh();
}

// Placeholder for test_camera
void test_camera(void) {
    clear();
    printw("\nCamera testing functionality is not implemented on this platform.\n");
    printw("Press any key to continue...\n");
    refresh();
    getch();
}

// Function to view file contents
void view_file_contents(const char *filepath) {
    clear();
    
    // Create a new window for file viewing with proper code editor appearance
    int viewer_height = LINES - 6;  // Leave room for header and footer
    int viewer_width = COLS;
    
    WINDOW *viewer_win = newwin(viewer_height, viewer_width, 3, 0);
    keypad(viewer_win, TRUE);
    scrollok(viewer_win, TRUE);
    
    // Use a more code-editor-like title bar with modern styling
    attron(A_REVERSE | COLOR_PAIR(4));
    for (int i = 0; i < COLS; i++) {
        mvaddch(0, i, ' ');
    }
    mvprintw(0, 1, "File Viewer - %s", filepath);
    mvprintw(0, COLS - 27, "Press q to quit, e to edit");
    attroff(A_REVERSE | COLOR_PAIR(4));
    
    // Better looking path bar
    attron(COLOR_PAIR(1) | A_BOLD);
    mvprintw(1, 0, "Path: ");
    attroff(COLOR_PAIR(1) | A_BOLD);
    printw("%s", filepath);
    
    // Draw a double-lined box around the viewer window for a more professional look
    wborder(viewer_win, '|', '|', '-', '-', '+', '+', '+', '+');
    
    // Try to open the file
    FILE *file = fopen(filepath, "r");
    if (!file) {
        mvprintw(2, 1, "Error: Cannot open file: %s", strerror(errno));
        draw_error_status_bar("Cannot open file");
        wrefresh(viewer_win);
        refresh();
        getch();
        delwin(viewer_win);
        return;
    }
    
    // Read the file into a vector of lines
    std::vector<std::string> lines;
    char line_buffer[4096];
    while (fgets(line_buffer, sizeof(line_buffer), file)) {
        // Remove trailing newline
        size_t len = strlen(line_buffer);
        if (len > 0 && line_buffer[len-1] == '\n') {
            line_buffer[len-1] = '\0';
        }
        lines.push_back(line_buffer);
    }
    fclose(file);
    
    // Display parameters
    int current_line = 0;
    int max_display_lines = viewer_height - 2;  // Account for border
    int left_margin = 6;  // Space for line numbers
    
    // Main viewing loop
    bool running = true;
    bool edit_mode = false;
    int edit_line = 0;
    int edit_col = 0;
    bool modified = false;
    
    while (running) {
        // Clear viewer window content but preserve the border
        for (int i = 1; i < viewer_height - 1; i++) {
            wmove(viewer_win, i, 1);
            for (int j = 1; j < viewer_width - 1; j++) {
                waddch(viewer_win, ' ');
            }
        }
        
        if (!edit_mode) {
            // VIEW MODE
            // Display lines with syntax highlighting-like coloring for common programming elements
            for (int i = 0; i < max_display_lines && current_line + i < (int)lines.size(); i++) {
                // Ensure we don't go beyond window boundaries
                if (i + 1 >= viewer_height - 1) break;
                
                // Display line number with right alignment and highlighted background
                wattron(viewer_win, COLOR_PAIR(4) | A_BOLD);
                mvwprintw(viewer_win, i + 1, 1, "%4d ", current_line + i + 1);
                wattroff(viewer_win, COLOR_PAIR(4) | A_BOLD);
                
                // Add a separator between line numbers and text
                wattron(viewer_win, COLOR_PAIR(4));
                mvwaddch(viewer_win, i + 1, left_margin - 1, '|');
                wattroff(viewer_win, COLOR_PAIR(4));
                
                // Get the line to display
                std::string &display_line = lines[current_line + i];
                
                // Apply simple syntax highlighting (just for visual effect)
                std::string current_word;
                int cursor_pos = left_margin;
                
                for (size_t j = 0; j < display_line.length() && cursor_pos < viewer_width - 2; j++) {
                    char ch = display_line[j];
                    
                    // Handle tab characters properly
                    if (ch == '\t') {
                        int spaces = 4 - ((cursor_pos - left_margin) % 4);
                        for (int k = 0; k < spaces && cursor_pos < viewer_width - 2; k++) {
                            mvwaddch(viewer_win, i + 1, cursor_pos++, ' ');
                        }
                        continue;
                    }
                    
                    // Check for comments (C/C++ style)
                    if (j < display_line.length() - 1 && ch == '/' && display_line[j+1] == '/') {
                        wattron(viewer_win, COLOR_PAIR(3));  // Use cyan for comments
                        for (size_t k = j; k < display_line.length() && cursor_pos < viewer_width - 2; k++) {
                            mvwaddch(viewer_win, i + 1, cursor_pos++, display_line[k]);
                        }
                        wattroff(viewer_win, COLOR_PAIR(3));
                        break;
                    }
                    
                    // Check for string literals
                    if (ch == '"' || ch == '\'') {
                        char quote = ch;
                        wattron(viewer_win, COLOR_PAIR(2));  // Use green for strings
                        mvwaddch(viewer_win, i + 1, cursor_pos++, ch);
                        for (j++; j < display_line.length() && cursor_pos < viewer_width - 2; j++) {
                            if (display_line[j] == '\\' && j+1 < display_line.length()) {
                                // Handle escape sequence
                                mvwaddch(viewer_win, i + 1, cursor_pos++, '\\');
                                j++;
                                if (cursor_pos < viewer_width - 2) {
                                    mvwaddch(viewer_win, i + 1, cursor_pos++, display_line[j]);
                                }
                            } else if (display_line[j] == quote) {
                                mvwaddch(viewer_win, i + 1, cursor_pos++, quote);
                                break;
                            } else {
                                mvwaddch(viewer_win, i + 1, cursor_pos++, display_line[j]);
                            }
                        }
                        wattroff(viewer_win, COLOR_PAIR(2));
                        continue;
                    }
                    
                    // Just display the character normally
                    mvwaddch(viewer_win, i + 1, cursor_pos++, ch);
                }
            }
            
            // Show position info in a cleaner format
            int total_lines = lines.size();
            attron(A_BOLD);
            mvprintw(2, 0, "Line: ");
            attroff(A_BOLD);
            printw("%d of %d (%.0f%%)", 
                  current_line + 1, total_lines, 
                  total_lines > 0 ? (current_line + 1) * 100.0 / total_lines : 0);
            
            // Show scrollbar using block characters for a more modern look
            if (total_lines > max_display_lines) {
                int scrollbar_height = (max_display_lines * max_display_lines) / total_lines;
                if (scrollbar_height < 1) scrollbar_height = 1;
                
                int scrollbar_pos = (max_display_lines * current_line) / total_lines;
                
                wattron(viewer_win, COLOR_PAIR(4));
                for (int i = 0; i < max_display_lines; i++) {
                    mvwaddch(viewer_win, i + 1, viewer_width - 2, 
                           (i >= scrollbar_pos && i < scrollbar_pos + scrollbar_height) ? '#' : '|');
                }
                wattroff(viewer_win, COLOR_PAIR(4));
            }
            
            // Update instructions with a cleaner layout
            mvprintw(LINES - 2, 0, "Arrow keys: scroll | Page Up/Down: page scroll | Home/End: start/end");
            mvprintw(LINES - 1, 0, "e: edit file | q: quit");
        }
        else {
            // EDIT MODE
            // Update title bar to show we're in edit mode
            attron(A_REVERSE | COLOR_PAIR(5));
            for (int i = 0; i < COLS; i++) {
                mvaddch(0, i, ' ');
            }
            mvprintw(0, 1, "EDITOR - %s", filepath);
            mvprintw(0, COLS - 35, "Press F2 to save, Esc to exit without saving");
            attroff(A_REVERSE | COLOR_PAIR(5));
            
            // Show edit position
            attron(A_BOLD);
            mvprintw(2, 0, "Line: ");
            attroff(A_BOLD);
            printw("%d of %d, Col: %d  ", 
                  edit_line + 1, (int)lines.size(), edit_col + 1);
            
            if (modified) {
                attron(COLOR_PAIR(5) | A_BOLD);
                printw("[Modified]");
                attroff(COLOR_PAIR(5) | A_BOLD);
            }
            
            // Display lines for editing
            for (int i = 0; i < max_display_lines && current_line + i < (int)lines.size(); i++) {
                // Ensure we don't go beyond window boundaries
                if (i + 1 >= viewer_height - 1) break;
                
                // Display line number with right alignment and highlighted background
                wattron(viewer_win, COLOR_PAIR(4) | A_BOLD);
                mvwprintw(viewer_win, i + 1, 1, "%4d ", current_line + i + 1);
                wattroff(viewer_win, COLOR_PAIR(4) | A_BOLD);
                
                // Add a separator between line numbers and text
                wattron(viewer_win, COLOR_PAIR(4));
                mvwaddch(viewer_win, i + 1, left_margin - 1, '|');
                wattroff(viewer_win, COLOR_PAIR(4));
                
                // Get the line to display
                std::string &display_line = lines[current_line + i];
                
                // Display the text
                for (size_t j = 0; j < display_line.length() && left_margin + j < (size_t)viewer_width - 2; j++) {
                    // If this is the current edit line and column, position the cursor here
                    if (current_line + i == edit_line && (int)j == edit_col) {
                        wmove(viewer_win, i + 1, left_margin + j);
                    }
                    
                    // Handle tabs for display
                    if (display_line[j] == '\t') {
                        int spaces = 4 - ((j) % 4);
                        for (int k = 0; k < spaces && left_margin + j + k < (size_t)viewer_width - 2; k++) {
                            mvwaddch(viewer_win, i + 1, left_margin + j + k, ' ');
                        }
                    } else {
                        mvwaddch(viewer_win, i + 1, left_margin + j, display_line[j]);
                    }
                }
                
                // If we're at the edit line but at the end of text, position cursor at the end
                if (current_line + i == edit_line && edit_col >= (int)display_line.length()) {
                    wmove(viewer_win, i + 1, left_margin + display_line.length());
                }
            }
            
            // Update instructions for edit mode
            mvprintw(LINES - 2, 0, "Arrow keys: move cursor | Backspace/Delete: delete char | Enter: new line");
            mvprintw(LINES - 1, 0, "F2: save | Esc: exit without saving");
        }
        
        // Refresh display
        wrefresh(viewer_win);
        refresh();
        
        // Handle input
        int ch = wgetch(viewer_win);
        
        if (!edit_mode) {
            // VIEW MODE CONTROLS
            switch (ch) {
                case KEY_UP:
                    if (current_line > 0) {
                        current_line--;
                    }
                    break;
                
                case KEY_DOWN:
                    if (current_line < (int)lines.size() - max_display_lines) {
                        current_line++;
                    }
                    break;
                
                case KEY_PPAGE:  // Page Up
                    current_line -= max_display_lines;
                    if (current_line < 0) {
                        current_line = 0;
                    }
                    break;
                
                case KEY_NPAGE:  // Page Down
                    current_line += max_display_lines;
                    if (current_line > (int)lines.size() - max_display_lines) {
                        current_line = lines.size() - max_display_lines;
                    }
                    if (current_line < 0) {
                        current_line = 0;
                    }
                    break;
                
                case KEY_HOME:
                    current_line = 0;
                    break;
                
                case KEY_END:
                    current_line = lines.size() - max_display_lines;
                    if (current_line < 0) {
                        current_line = 0;
                    }
                    break;
                
                case 'e':  // Switch to edit mode
                    edit_mode = true;
                    edit_line = current_line;
                    edit_col = 0;
                    break;
                    
                case 'q':
                    running = false;
                    break;
            }
        }
        else {
            // EDIT MODE CONTROLS
            switch (ch) {
                case KEY_UP:
                    if (edit_line > 0) {
                        edit_line--;
                        if (edit_col > (int)lines[edit_line].length()) {
                            edit_col = lines[edit_line].length();
                        }
                        if (edit_line < current_line) {
                            current_line = edit_line;
                        }
                    }
                    break;
                
                case KEY_DOWN:
                    if (edit_line < (int)lines.size() - 1) {
                        edit_line++;
                        if (edit_col > (int)lines[edit_line].length()) {
                            edit_col = lines[edit_line].length();
                        }
                        if (edit_line >= current_line + max_display_lines) {
                            current_line = edit_line - max_display_lines + 1;
                        }
                    }
                    break;
                
                case KEY_LEFT:
                    if (edit_col > 0) {
                        edit_col--;
                    } else if (edit_line > 0) {
                        edit_line--;
                        edit_col = lines[edit_line].length();
                        if (edit_line < current_line) {
                            current_line = edit_line;
                        }
                    }
                    break;
                
                case KEY_RIGHT:
                    if (edit_col < (int)lines[edit_line].length()) {
                        edit_col++;
                    } else if (edit_line < (int)lines.size() - 1) {
                        edit_line++;
                        edit_col = 0;
                        if (edit_line >= current_line + max_display_lines) {
                            current_line = edit_line - max_display_lines + 1;
                        }
                    }
                    break;
                
                case KEY_HOME:
                    edit_col = 0;
                    break;
                
                case KEY_END:
                    edit_col = lines[edit_line].length();
                    break;
                
                case KEY_BACKSPACE:
                case 127:  // DEL key
                    if (edit_col > 0) {
                        // Delete character before cursor
                        lines[edit_line].erase(edit_col - 1, 1);
                        edit_col--;
                        modified = true;
                    } else if (edit_line > 0) {
                        // Merge with previous line
                        edit_col = lines[edit_line - 1].length();
                        lines[edit_line - 1] += lines[edit_line];
                        lines.erase(lines.begin() + edit_line);
                        edit_line--;
                        modified = true;
                        if (edit_line < current_line) {
                            current_line = edit_line;
                        }
                    }
                    break;
                
                case KEY_DC:  // Delete key
                    if (edit_col < (int)lines[edit_line].length()) {
                        // Delete character at cursor
                        lines[edit_line].erase(edit_col, 1);
                        modified = true;
                    } else if (edit_line < (int)lines.size() - 1) {
                        // Merge with next line
                        lines[edit_line] += lines[edit_line + 1];
                        lines.erase(lines.begin() + edit_line + 1);
                        modified = true;
                    }
                    break;
                
                case '\n':
                case KEY_ENTER:
                    // Split line and insert a new one
                    {
                        std::string new_line = lines[edit_line].substr(edit_col);
                        lines[edit_line].erase(edit_col);
                        lines.insert(lines.begin() + edit_line + 1, new_line);
                        edit_line++;
                        edit_col = 0;
                        modified = true;
                        if (edit_line >= current_line + max_display_lines) {
                            current_line = edit_line - max_display_lines + 1;
                        }
                    }
                    break;
                
                case '\t':
                    // Insert tab
                    lines[edit_line].insert(edit_col, "\t");
                    edit_col++;
                    modified = true;
                    break;
                
                case KEY_F(2):  // Save
                    {
                        FILE *save_file = fopen(filepath, "w");
                        bool success = false;
                        
                        if (save_file) {
                            for (size_t i = 0; i < lines.size(); i++) {
                                fprintf(save_file, "%s\n", lines[i].c_str());
                            }
                            fclose(save_file);
                            success = true;
                        }
                        
                        if (success) {
                            modified = false;
                            mvprintw(2, 30, "File saved successfully!         ");
                        } else {
                            mvprintw(2, 30, "Error saving file!               ");
                        }
                        refresh();
                    }
                    break;
                
                case 27:  // ESC key
                    if (modified) {
                        // Confirm before exiting
                        mvprintw(LINES - 1, 0, "File modified! Press 'y' to exit without saving, any other key to continue editing");
                        refresh();
                        int confirm = wgetch(viewer_win);
                        if (confirm == 'y' || confirm == 'Y') {
                            edit_mode = false;
                        }
                    } else {
                        edit_mode = false;
                    }
                    break;
                
                default:
                    // Insert regular characters
                    if (ch >= 32 && ch <= 126) {  // Printable ASCII
                        lines[edit_line].insert(edit_col, 1, (char)ch);
                        edit_col++;
                        modified = true;
                    }
                    break;
            }
        }
    }
    
    // Clean up
    delwin(viewer_win);
    clear();
    refresh();
}

// Add forward declarations right before the view_file_contents function
void edit_file_contents(const char *filepath);
bool save_file(const std::vector<std::string> &lines, const char *filepath);

// New function to edit files
void edit_file_contents(const char *filepath) {
    clear();
    
    // Create a new window for file editing with code editor appearance
    int editor_height = LINES - 6;  // Leave room for header and footer
    int editor_width = COLS;
    
    WINDOW *editor_win = newwin(editor_height, editor_width, 3, 0);
    keypad(editor_win, TRUE);
    scrollok(editor_win, TRUE);
    
    // Modern editor title bar
    attron(A_REVERSE | COLOR_PAIR(5));  // Use red to indicate editing mode
    for (int i = 0; i < COLS; i++) {
        mvaddch(0, i, ' ');
    }
    mvprintw(0, 1, "EDITOR - %s", filepath);
    mvprintw(0, COLS - 35, "Press F2 to save, Esc to exit without saving");
    attroff(A_REVERSE | COLOR_PAIR(5));
    
    // Better looking path bar with edit mode indicator
    attron(COLOR_PAIR(5) | A_BOLD);
    mvprintw(1, 0, "EDITING: ");
    attroff(COLOR_PAIR(5) | A_BOLD);
    printw("%s", filepath);
    
    // Draw a double-lined box around the editor window
    wborder(editor_win, '|', '|', '-', '-', '+', '+', '+', '+');
    
    // Try to open the file
    FILE *file = fopen(filepath, "r");
    if (!file) {
        // If file doesn't exist, create a new empty file
        mvprintw(2, 1, "Creating new file: %s", filepath);
    }
    
    // Read the file into a vector of lines or start with one empty line for new files
    std::vector<std::string> lines;
    if (file) {
        char line_buffer[4096];
        while (fgets(line_buffer, sizeof(line_buffer), file)) {
            // Remove trailing newline
            size_t len = strlen(line_buffer);
            if (len > 0 && line_buffer[len-1] == '\n') {
                line_buffer[len-1] = '\0';
            }
            lines.push_back(line_buffer);
        }
        fclose(file);
    }
    
    // Ensure there's at least one line to edit
    if (lines.empty()) {
        lines.push_back("");
    }
    
    // Editor state variables
    int current_line = 0;         // Current line in the file
    int current_col = 0;          // Current column in the line
    int screen_line = 0;          // Top line displayed on screen
    int left_margin = 6;          // Space for line numbers
    int max_display_lines = editor_height - 2;  // Account for border
    bool modified = false;        // Whether the file has been modified
    
    // Draw status message about key functions
    attron(A_BOLD);
    mvprintw(2, 0, "Status: ");
    attroff(A_BOLD);
    if (file) {
        printw("File loaded successfully.");
    } else {
        printw("Creating new file.");
    }
    
    // Main editing loop
    bool running = true;
    while (running) {
        // Clear editor window content but preserve the border
        for (int i = 1; i < editor_height - 1; i++) {
            wmove(editor_win, i, 1);
            for (int j = 1; j < editor_width - 1; j++) {
                waddch(editor_win, ' ');
            }
        }
        
        // Display lines with line numbers
        for (int i = 0; i < max_display_lines && screen_line + i < (int)lines.size(); i++) {
            // Ensure we don't go beyond window boundaries
            if (i + 1 >= editor_height - 1) break;
            
            // Display line number with right alignment and highlighted background
            wattron(editor_win, COLOR_PAIR(4) | A_BOLD);
            mvwprintw(editor_win, i + 1, 1, "%4d ", screen_line + i + 1);
            wattroff(editor_win, COLOR_PAIR(4) | A_BOLD);
            
            // Add a separator between line numbers and text
            wattron(editor_win, COLOR_PAIR(4));
            mvwaddch(editor_win, i + 1, left_margin - 1, '|');
            wattroff(editor_win, COLOR_PAIR(4));
            
            // Get the line to display
            std::string &display_line = lines[screen_line + i];
            
            // Display the line with basic syntax coloring
            std::string current_word;
            int display_col = 0;  // Column position in the displayed text
            
            for (size_t j = 0; j < display_line.length() && left_margin + display_col < editor_width - 2; j++) {
                char ch = display_line[j];
                
                // Handle tab characters
                if (ch == '\t') {
                    int spaces = 4 - (display_col % 4);
                    for (int k = 0; k < spaces && left_margin + display_col < editor_width - 2; k++) {
                        mvwaddch(editor_win, i + 1, left_margin + display_col, ' ');
                        display_col++;
                    }
                    continue;
                }
                
                // Check for comments (C/C++ style)
                if (j < display_line.length() - 1 && ch == '/' && display_line[j+1] == '/') {
                    wattron(editor_win, COLOR_PAIR(3));  // Use cyan for comments
                    for (size_t k = j; k < display_line.length() && left_margin + display_col < editor_width - 2; k++) {
                        mvwaddch(editor_win, i + 1, left_margin + display_col, display_line[k]);
                        display_col++;
                    }
                    wattroff(editor_win, COLOR_PAIR(3));
                    break;
                }
                
                // Check for string literals
                if (ch == '"' || ch == '\'') {
                    char quote = ch;
                    wattron(editor_win, COLOR_PAIR(2));  // Use green for strings
                    mvwaddch(editor_win, i + 1, left_margin + display_col, ch);
                    display_col++;
                    
                    for (j++; j < display_line.length() && left_margin + display_col < editor_width - 2; j++) {
                        if (display_line[j] == '\\' && j+1 < display_line.length()) {
                            // Handle escape sequence
                            mvwaddch(editor_win, i + 1, left_margin + display_col, '\\');
                            display_col++;
                            j++;
                            if (left_margin + display_col < editor_width - 2) {
                                mvwaddch(editor_win, i + 1, left_margin + display_col, display_line[j]);
                                display_col++;
                            }
                        } else if (display_line[j] == quote) {
                            mvwaddch(editor_win, i + 1, left_margin + display_col, quote);
                            display_col++;
                            break;
                        } else {
                            mvwaddch(editor_win, i + 1, left_margin + display_col, display_line[j]);
                            display_col++;
                        }
                    }
                    wattroff(editor_win, COLOR_PAIR(2));
                    continue;
                }
                
                // Regular characters
                mvwaddch(editor_win, i + 1, left_margin + display_col, ch);
                display_col++;
            }
            
            // If this is the current line, highlight it
            if (screen_line + i == current_line) {
                // Position the cursor
                int cursor_x = left_margin;
                for (int j = 0; j < current_col; j++) {
                    if (j < (int)display_line.length()) {
                        if (display_line[j] == '\t') {
                            // Each tab is 4 spaces
                            cursor_x += 4 - ((cursor_x - left_margin) % 4);
                        } else {
                            cursor_x++;
                        }
                    } else {
                        cursor_x++;
                    }
                }
                wmove(editor_win, i + 1, cursor_x);
            }
        }
        
        // Update position information
        mvprintw(2, 30, "Line: %d, Col: %d  ", current_line + 1, current_col + 1);
        if (modified) {
            attron(COLOR_PAIR(5) | A_BOLD);
            printw("[Modified]");
            attroff(COLOR_PAIR(5) | A_BOLD);
        }
        
        // Show scrollbar
        if (lines.size() > (size_t)max_display_lines) {
            int scrollbar_height = (max_display_lines * max_display_lines) / lines.size();
            if (scrollbar_height < 1) scrollbar_height = 1;
            
            int scrollbar_pos = (max_display_lines * screen_line) / lines.size();
            
            wattron(editor_win, COLOR_PAIR(4));
            for (int i = 0; i < max_display_lines; i++) {
                mvwaddch(editor_win, i + 1, editor_width - 2, 
                       (i >= scrollbar_pos && i < scrollbar_pos + scrollbar_height) ? '#' : '|');
            }
            wattroff(editor_win, COLOR_PAIR(4));
        }
        
        // Update instructions
        mvprintw(LINES - 2, 0, "Arrow keys: move cursor | Backspace/Delete: delete | Enter: new line");
        mvprintw(LINES - 1, 0, "F2: save | Esc: quit without saving");
        
        // Refresh display and position cursor
        wrefresh(editor_win);
        refresh();
        
        // Get user input
        int ch = wgetch(editor_win);
        
        switch (ch) {
            case KEY_UP:
                if (current_line > 0) {
                    current_line--;
                    // Make sure cursor column is valid for the new line
                    if (current_col > (int)lines[current_line].length()) {
                        current_col = lines[current_line].length();
                    }
                    // Scroll if needed
                    if (current_line < screen_line) {
                        screen_line = current_line;
                    }
                }
                break;
                
            case KEY_DOWN:
                if (current_line < (int)lines.size() - 1) {
                    current_line++;
                    // Make sure cursor column is valid for the new line
                    if (current_col > (int)lines[current_line].length()) {
                        current_col = lines[current_line].length();
                    }
                    // Scroll if needed
                    if (current_line >= screen_line + max_display_lines) {
                        screen_line = current_line - max_display_lines + 1;
                    }
                }
                break;
                
            case KEY_LEFT:
                if (current_col > 0) {
                    current_col--;
                } else if (current_line > 0) {
                    // Move to the end of the previous line
                    current_line--;
                    current_col = lines[current_line].length();
                    // Scroll if needed
                    if (current_line < screen_line) {
                        screen_line = current_line;
                    }
                }
                break;
                
            case KEY_RIGHT:
                if (current_col < (int)lines[current_line].length()) {
                    current_col++;
                } else if (current_line < (int)lines.size() - 1) {
                    // Move to the beginning of the next line
                    current_line++;
                    current_col = 0;
                    // Scroll if needed
                    if (current_line >= screen_line + max_display_lines) {
                        screen_line = current_line - max_display_lines + 1;
                    }
                }
                break;
                
            case KEY_HOME:
                current_col = 0;
                break;
                
            case KEY_END:
                current_col = lines[current_line].length();
                break;
                
            case KEY_BACKSPACE:
            case 127:  // DEL key
                if (current_col > 0) {
                    // Delete character before cursor
                    lines[current_line].erase(current_col - 1, 1);
                    current_col--;
                    modified = true;
                } else if (current_line > 0) {
                    // Merge with previous line
                    current_col = lines[current_line - 1].length();
                    lines[current_line - 1] += lines[current_line];
                    lines.erase(lines.begin() + current_line);
                    current_line--;
                    modified = true;
                    // Adjust screen_line if needed
                    if (current_line < screen_line) {
                        screen_line = current_line;
                    }
                }
                break;
                
            case KEY_DC:  // Delete key
                if (current_col < (int)lines[current_line].length()) {
                    // Delete character at cursor
                    lines[current_line].erase(current_col, 1);
                    modified = true;
                } else if (current_line < (int)lines.size() - 1) {
                    // Merge with next line
                    lines[current_line] += lines[current_line + 1];
                    lines.erase(lines.begin() + current_line + 1);
                    modified = true;
                }
                break;
                
            case '\n':
            case KEY_ENTER:
                // Split line and insert a new one
                {
                    std::string new_line = lines[current_line].substr(current_col);
                    lines[current_line].erase(current_col);
                    lines.insert(lines.begin() + current_line + 1, new_line);
                    current_line++;
                    current_col = 0;
                    modified = true;
                    // Scroll if needed
                    if (current_line >= screen_line + max_display_lines) {
                        screen_line = current_line - max_display_lines + 1;
                    }
                }
                break;
                
            case '\t':
                // Insert tab
                lines[current_line].insert(current_col, "\t");
                current_col++;
                modified = true;
                break;
                
            case KEY_F(2):  // Save
                {
                    FILE *file = fopen(filepath, "w");
                    bool success = false;
                    if (file) {
                        for (size_t i = 0; i < lines.size(); i++) {
                            fprintf(file, "%s\n", lines[i].c_str());
                        }
                        fclose(file);
                        success = true;
                    }
                    
                    if (success) {
                        modified = false;
                        mvprintw(2, 30, "File saved successfully!         ");
                    } else {
                        mvprintw(2, 30, "Error saving file!               ");
                    }
                }
                refresh();
                break;
                
            case 27:  // ESC key
                if (modified) {
                    // Confirm before exiting
                    mvprintw(LINES - 1, 0, "File modified! Press 'y' to exit without saving, any other key to continue editing");
                    refresh();
                    int confirm = wgetch(editor_win);
                    if (confirm == 'y' || confirm == 'Y') {
                        running = false;
                    }
                } else {
                    running = false;
                }
                break;
                
            default:
                // Insert regular characters
                if (ch >= 32 && ch <= 126) {  // Printable ASCII
                    lines[current_line].insert(current_col, 1, (char)ch);
                    current_col++;
                    modified = true;
                }
                break;
        }
    }
    
    // Clean up
    delwin(editor_win);
    clear();
    refresh();
}

// Function to save file
bool save_file(const std::vector<std::string> &lines, const char *filepath) {
    FILE *file = fopen(filepath, "w");
    if (!file) {
        return false;
    }
    
    for (size_t i = 0; i < lines.size(); i++) {
        fprintf(file, "%s\n", lines[i].c_str());
    }
    
    fclose(file);
    return true;
}

// Improved file explorer implementation
void launch_explorer(void) {
    clear();
    int explorer_height = LINES - 4;
    int explorer_width = COLS;
    
    WINDOW *explorer_win = newwin(explorer_height, explorer_width, 2, 0);
    keypad(explorer_win, TRUE);
    
    // Box drawing characters (ASCII for better compatibility)
    attron(A_REVERSE);
    for (int i = 0; i < COLS; i++) {
        mvaddch(0, i, ' ');
    }
    mvprintw(0, 1, "File Explorer - Use arrow keys to navigate, Enter to select, v to view, q to quit");
    attroff(A_REVERSE);
    
    mvprintw(1, 0, "Path: %s", current_path);
    
    // Variables for directory listing
    DIR *dir;
    struct dirent *entry;
    std::vector<std::string> entries;
    std::vector<bool> is_dir;
    std::vector<off_t> file_sizes;
    
    // Current selection and scroll position
    int current_item = 0;
    int scroll_pos = 0;
    
    // Main exploration loop
    bool running = true;
    char current_explorer_path[MAX_PATH];
    strncpy(current_explorer_path, current_path, MAX_PATH - 1);
    current_explorer_path[MAX_PATH - 1] = '\0'; // Ensure null termination
    
    // Define column widths and positions
    const int name_col_width = explorer_width - 32; // Allow enough space for size and type columns
    const int size_col_pos = name_col_width + 2;
    const int type_col_pos = size_col_pos + 12;
    
    while (running) {
        // Clear entries from previous directory
        entries.clear();
        is_dir.clear();
        file_sizes.clear();
        
        // Get directory listing
        dir = opendir(current_explorer_path);
        if (dir) {
            while ((entry = readdir(dir)) != NULL) {
                entries.push_back(entry->d_name);
                
                // Check if it's a directory and get file size
                char full_path[MAX_PATH];
                
                // Safe path construction to avoid buffer overflow
                size_t path_len = strlen(current_explorer_path);
                size_t name_len = strlen(entry->d_name);
                
                // Check if the combined length would exceed buffer size (accounting for '/' and null terminator)
                if (path_len + name_len + 2 <= MAX_PATH) {
                    strcpy(full_path, current_explorer_path);
                    strcat(full_path, "/");
                    strcat(full_path, entry->d_name);
                } else {
                    // Path would be too long - log error and use truncated path
                    log_error(error_console, ERROR_WARNING, "EXPLORER", 
                              "Path too long: %s/%s", current_explorer_path, entry->d_name);
                    draw_error_status_bar("Path too long");
                    strncpy(full_path, current_explorer_path, MAX_PATH - 1);
                    full_path[MAX_PATH - 1] = '\0';
                }
                
                struct stat st;
                bool is_directory = false;
                off_t size = 0;
                
                if (stat(full_path, &st) == 0) {
                    is_directory = S_ISDIR(st.st_mode);
                    size = st.st_size;
                }
                
                is_dir.push_back(is_directory);
                file_sizes.push_back(size);
            }
            closedir(dir);
            
            // Sort entries (directories first, then alphabetically)
            std::vector<size_t> indices(entries.size());
            for (size_t i = 0; i < entries.size(); i++) {
                indices[i] = i;
            }
            
            std::sort(indices.begin(), indices.end(), [&](size_t a, size_t b) {
                if (is_dir[a] != is_dir[b]) {
                    return is_dir[a] > is_dir[b]; // Directories first
                }
                return entries[a] < entries[b]; // Then alphabetically
            });
            
            // Apply the sort
            std::vector<std::string> sorted_entries;
            std::vector<bool> sorted_is_dir;
            std::vector<off_t> sorted_file_sizes;
            
            for (size_t idx : indices) {
                sorted_entries.push_back(entries[idx]);
                sorted_is_dir.push_back(is_dir[idx]);
                sorted_file_sizes.push_back(file_sizes[idx]);
            }
            
            entries = sorted_entries;
            is_dir = sorted_is_dir;
            file_sizes = sorted_file_sizes;
            
            // Reset selection if needed
            if (current_item >= (int)entries.size()) {
                current_item = entries.size() - 1;
                if (current_item < 0) current_item = 0;
            }
            
            // Update display path
            mvprintw(1, 0, "Path: %s ", current_explorer_path); // Space at end to clear any leftover text
            clrtoeol(); // Clear to the end of line
            
            // Clear explorer window
            werase(explorer_win);
            box(explorer_win, 0, 0);
            
            // Add column headers
            wattron(explorer_win, A_BOLD);
            mvwprintw(explorer_win, 0, 2, " Name");
            mvwprintw(explorer_win, 0, size_col_pos, "Size");
            mvwprintw(explorer_win, 0, type_col_pos, "Type");
            wattroff(explorer_win, A_BOLD);
            
            // Display entries
            int display_height = explorer_height - 2; // Account for borders
            
            // Adjust scroll position if necessary
            if (current_item < scroll_pos) {
                scroll_pos = current_item;
            } else if (current_item >= scroll_pos + display_height) {
                scroll_pos = current_item - display_height + 1;
            }
            
            for (int i = 0; i < display_height && i + scroll_pos < (int)entries.size(); i++) {
                int entry_idx = i + scroll_pos;
                std::string name = entries[entry_idx];
                
                // Truncate name if too long for the column
                if (name.length() > (size_t)name_col_width - 5) {
                    name = name.substr(0, name_col_width - 8) + "...";
                }
                
                // Highlight selected item
                if (entry_idx == current_item) {
                    wattron(explorer_win, A_REVERSE);
                }
                
                // Clear the entire line first to avoid artifacts
                for (int x = 1; x < explorer_width - 1; x++) {
                    mvwaddch(explorer_win, i + 1, x, ' ');
                }
                
                // Display file size
                char size_str[32] = "";
                if (!is_dir[entry_idx]) {
                    off_t size = file_sizes[entry_idx];
                    if (size < 1024) {
                        snprintf(size_str, sizeof(size_str), "%ld B", (long)size);
                    } else if (size < 1024 * 1024) {
                        snprintf(size_str, sizeof(size_str), "%.1f KB", size / 1024.0);
                    } else if (size < 1024 * 1024 * 1024) {
                        snprintf(size_str, sizeof(size_str), "%.1f MB", size / (1024.0 * 1024.0));
                    } else {
                        snprintf(size_str, sizeof(size_str), "%.1f GB", size / (1024.0 * 1024.0 * 1024.0));
                    }
                }
                
                // Display file type
                const char *type_str = is_dir[entry_idx] ? "Directory" : "File";
                
                // Display entry with appropriate color
                if (is_dir[entry_idx]) {
                    wattron(explorer_win, COLOR_PAIR(1) | A_BOLD);
                    mvwprintw(explorer_win, i + 1, 2, " %s/", name.c_str());
                    wattroff(explorer_win, COLOR_PAIR(1) | A_BOLD);
                    
                    // Display file type (right-aligned)
                    mvwprintw(explorer_win, i + 1, type_col_pos, "%s", type_str);
                } else {
                    // Check if it's executable
                    char full_path[MAX_PATH];
                    
                    // Safe path construction
                    size_t path_len = strlen(current_explorer_path);
                    size_t name_len = entries[entry_idx].length();
                    
                    // Check if the combined length would exceed buffer size
                    if (path_len + name_len + 2 <= MAX_PATH) {
                        strcpy(full_path, current_explorer_path);
                        strcat(full_path, "/");
                        strcat(full_path, entries[entry_idx].c_str());
                    } else {
                        // Path would be too long - log error and use truncated path
                        log_error(error_console, ERROR_WARNING, "EXPLORER", 
                                "Path too long: %s/%s", current_explorer_path, entries[entry_idx].c_str());
                        draw_error_status_bar("Path too long");
                        strncpy(full_path, current_explorer_path, MAX_PATH - 1);
                        full_path[MAX_PATH - 1] = '\0';
                    }
                    
                    struct stat st;
                    
                    if (stat(full_path, &st) == 0 && (st.st_mode & S_IXUSR)) {
                        wattron(explorer_win, COLOR_PAIR(2) | A_BOLD);
                        mvwprintw(explorer_win, i + 1, 2, " %s", name.c_str());
                        wattroff(explorer_win, COLOR_PAIR(2) | A_BOLD);
                    } else {
                        mvwprintw(explorer_win, i + 1, 2, " %s", name.c_str());
                    }
                    
                    // Display file size (right-aligned)
                    mvwprintw(explorer_win, i + 1, size_col_pos, "%s", size_str);
                    
                    // Display file type (right-aligned)
                    mvwprintw(explorer_win, i + 1, type_col_pos, "%s", type_str);
                }
                
                // End highlight
                if (entry_idx == current_item) {
                    wattroff(explorer_win, A_REVERSE);
                }
            }
            
            // Show scrollbar if needed
            if (entries.size() > (size_t)display_height) {
                int scrollbar_height = (display_height * display_height) / entries.size();
                if (scrollbar_height < 1) scrollbar_height = 1;
                
                int scrollbar_pos = (display_height * current_item) / entries.size();
                
                for (int i = 0; i < display_height; i++) {
                    mvwaddch(explorer_win, i + 1, explorer_width - 2, 
                            (i >= scrollbar_pos && i < scrollbar_pos + scrollbar_height) ? '#' : '|');
                }
            }
            
            // Add instructions at the bottom
            mvprintw(LINES - 1, 0, "Up/Down: navigate | Enter: open dir | v: view file | Backspace: go up | Home/End: first/last | q: quit");
            
            // Refresh windows
            wrefresh(explorer_win);
            refresh();
            
            // Handle input
            int ch = wgetch(explorer_win);
            
            switch (ch) {
                case KEY_UP:
                    if (current_item > 0) {
                        current_item--;
                    }
                    break;
                    
                case KEY_DOWN:
                    if (current_item < (int)entries.size() - 1) {
                        current_item++;
                    }
                    break;
                
                case KEY_PPAGE:  // Page Up
                    current_item -= display_height;
                    if (current_item < 0) {
                        current_item = 0;
                    }
                    break;
                    
                case KEY_NPAGE:  // Page Down
                    current_item += display_height;
                    if (current_item >= (int)entries.size()) {
                        current_item = entries.size() - 1;
                    }
                    break;
                    
                case KEY_HOME:
                    current_item = 0;
                    break;
                    
                case KEY_END:
                    current_item = entries.size() - 1;
                    break;
                    
                case '\n': // Enter
                    if (entries.size() > 0) {
                        if (is_dir[current_item]) {
                            // Change directory
                            if (entries[current_item] == "..") {
                                // Go up one level
                                char *last_slash = strrchr(current_explorer_path, '/');
                                if (last_slash != NULL && last_slash != current_explorer_path) {
                                    *last_slash = '\0';
                                }
                            } else if (entries[current_item] != ".") {
                                // Go into subdirectory - check buffer space first
                                size_t current_len = strlen(current_explorer_path);
                                size_t entry_len = entries[current_item].length();
                                
                                // Check if adding "/entry_name" would exceed buffer
                                if (current_len + entry_len + 2 <= MAX_PATH) {
                                    strcat(current_explorer_path, "/");
                                    strcat(current_explorer_path, entries[current_item].c_str());
                                } else {
                                    // Path would be too long
                                    log_error(error_console, ERROR_WARNING, "EXPLORER", 
                                            "Path too long: cannot enter %s", entries[current_item].c_str());
                                    draw_error_status_bar("Path too long: cannot enter directory");
                                }
                            }
                            
                            // Reset selection
                            current_item = 0;
                            scroll_pos = 0;
                        }
                    }
                    break;
                
                case 'v': // View file contents
                    if (entries.size() > 0 && !is_dir[current_item]) {
                        // Build full path for the file
                        char full_path[MAX_PATH];
                        
                        size_t path_len = strlen(current_explorer_path);
                        size_t entry_len = entries[current_item].length();
                        
                        if (path_len + entry_len + 2 <= MAX_PATH) {
                            strcpy(full_path, current_explorer_path);
                            strcat(full_path, "/");
                            strcat(full_path, entries[current_item].c_str());
                            
                            // View the file with our enhanced viewer
                            view_file_contents(full_path);
                            
                            // Redraw explorer after returning
                            clear();
                            attron(A_REVERSE);
                            for (int i = 0; i < COLS; i++) {
                                mvaddch(0, i, ' ');
                            }
                            mvprintw(0, 1, "File Explorer - Use arrow keys to navigate, Enter to select, v to view/edit, q to quit");
                            attroff(A_REVERSE);
                            mvprintw(1, 0, "Path: %s", current_explorer_path);
                        } else {
                            // Path would be too long
                            log_error(error_console, ERROR_WARNING, "EXPLORER", 
                                    "Path too long: cannot view %s", entries[current_item].c_str());
                            draw_error_status_bar("Path too long: cannot view file");
                        }
                    }
                    break;
                    
                case KEY_BACKSPACE:
                case 127: // DEL key
                    // Go up one level
                    {
                        char *last_slash = strrchr(current_explorer_path, '/');
                        if (last_slash != NULL && last_slash != current_explorer_path) {
                            *last_slash = '\0';
                        }
                        current_item = 0;
                        scroll_pos = 0;
                    }
                    break;
                    
                case 'q':
                    running = false;
                    break;
            }
        } else {
            // Failed to open directory
            log_error(error_console, ERROR_WARNING, "EXPLORER", 
                      "Error opening directory '%s': %s", current_explorer_path, strerror(errno));
            draw_error_status_bar("Cannot open directory");
            
            // Wait for user acknowledgment
            mvprintw(LINES - 1, 0, "Press any key to return to the shell...");
            refresh();
            getch();
            running = false;
        }
    }
    
    // Clean up
    delwin(explorer_win);
    clear();
    refresh();
}

// Function to display status bar with error message
void draw_error_status_bar(const char *error_msg) {
    // Clear status bar
    werase(status_bar);
    
    // Draw status bar with error color
    wattron(status_bar, COLOR_PAIR(5) | A_REVERSE | A_BOLD);
    for (int i = 0; i < screen_width; i++) {
        mvwaddch(status_bar, 0, i, ' ');
    }
    
    // Show error message
    mvwprintw(status_bar, 0, 1, "ERROR: %s", error_msg);
    mvwprintw(status_bar, 0, screen_width - 26, "Press ~ to view error log");
    
    wattroff(status_bar, COLOR_PAIR(5) | A_REVERSE | A_BOLD);
    wrefresh(status_bar);
}

// Update the log_error function to draw the error status bar for important errors
#define log_error_with_status(console, level, subsystem, fmt, ...) do { \
    log_error(console, level, subsystem, fmt, ##__VA_ARGS__); \
    if (level <= ERROR_ERROR) { \
        char error_buffer[256]; \
        snprintf(error_buffer, sizeof(error_buffer), "%s: " fmt, subsystem, ##__VA_ARGS__); \
        draw_error_status_bar(error_buffer); \
    } \
} while(0)

// Implement the cat command function - add this near the other command implementations
void cmd_cat(const char *filepath) {
    if (!filepath) {
        // Use the same approach as the other command functions
        printw("\nUsage: cat <filename>\n\n");
        refresh();
        return;
    }
    
    FILE *file = fopen(filepath, "r");
    if (!file) {
        printw("\nError: Cannot open file '%s': %s\n\n", filepath, strerror(errno));
        refresh();
        return;
    }
    
    // Print a header with the filename
    printw("\nFile: %s\n", filepath);
    printw("-------------------------------------------------\n");
    
    // Read and display file contents
    char line_buffer[4096];
    while (fgets(line_buffer, sizeof(line_buffer), file)) {
        // Print the line directly without modifications
        printw("%s", line_buffer);
    }
    
    // Print a footer
    printw("-------------------------------------------------\n\n");
    
    fclose(file);
    refresh();
}

// Add these implementations for history and log commands
void cmd_history(void) {
    printw("\nCommand History:\n\n");
    
    for (int i = 0; i < history_count; i++) {
        printw("  %3d  %s\n", i + 1, command_history[i]);
    }
    
    printw("\n");
    refresh();
}

void cmd_log(const char *message) {
    if (!message) {
        log_error_with_status(error_console, ERROR_WARNING, "MINUX", "Usage: log <message>");
        return;
    }
    
    // Get path to log file in user's home directory
    const char *home = getenv("HOME");
    if (!home) {
        struct passwd *pw = getpwuid(getuid());
        if (!pw) {
            log_error_with_status(error_console, ERROR_CRITICAL, "MINUX", "Could not determine home directory");
            return;
        }
        home = pw->pw_dir;
    }
    
    char log_file[MAX_PATH];
    snprintf(log_file, sizeof(log_file), "%s/.minux/log.txt", home);
    
    // Make sure .minux directory exists
    char dir_path[MAX_PATH];
    snprintf(dir_path, sizeof(dir_path), "%s/.minux", home);
    mkdir(dir_path, 0755);
    
    // Get current timestamp
    time_t t = time(NULL);
    struct tm *tm = localtime(&t);
    char timestamp[64];
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", tm);
    
    // Append to log file
    FILE *fp = fopen(log_file, "a");
    if (!fp) {
        log_error_with_status(error_console, ERROR_CRITICAL, "MINUX", 
                 "Could not open log file: %s", strerror(errno));
        return;
    }
    
    fprintf(fp, "[%s] %s\n", timestamp, message);
    fclose(fp);
    
    printw("\nLog entry added: %s\n\n", message);
    refresh();
}

// Add history management functions
void add_to_history(const char *cmd) {
    // Don't add empty commands or duplicates of the last command
    if (!cmd || !*cmd || (history_count > 0 && strcmp(cmd, command_history[history_count - 1]) == 0)) {
        return;
    }
    
    // Add new command to history
    if (history_count < MAX_HISTORY) {
        command_history[history_count++] = strdup(cmd);
    } else {
        // If history is full, remove oldest entry
        free(command_history[0]);
        for (int i = 0; i < MAX_HISTORY - 1; i++) {
            command_history[i] = command_history[i + 1];
        }
        command_history[MAX_HISTORY - 1] = strdup(cmd);
    }
    
    // Reset position to point to the end of history
    history_position = history_count;
    
    // Save history to file
    save_history();
}

void load_history(void) {
    // Get path to history file in user's home directory
    const char *home = getenv("HOME");
    if (!home) {
        struct passwd *pw = getpwuid(getuid());
        if (!pw) return;
        home = pw->pw_dir;
    }
    
    // Make sure .minux directory exists
    char dir_path[MAX_PATH];
    snprintf(dir_path, sizeof(dir_path), "%s/.minux", home);
    mkdir(dir_path, 0755);
    
    char history_path[MAX_PATH];
    snprintf(history_path, sizeof(history_path), "%s/.minux/%s", home, HISTORY_FILE);
    
    FILE *fp = fopen(history_path, "r");
    if (!fp) return;
    
    char line[MAX_CMD_LENGTH];
    history_count = 0;
    
    while (fgets(line, sizeof(line), fp) && history_count < MAX_HISTORY) {
        // Remove newline character
        int len = strlen(line);
        if (len > 0 && line[len - 1] == '\n') {
            line[len - 1] = '\0';
        }
        
        if (len > 1) {  // Only add non-empty lines
            command_history[history_count++] = strdup(line);
        }
    }
    
    fclose(fp);
    
    // Initialize history position to point to the end of history
    history_position = history_count;
}

void save_history(void) {
    // Get path to history file in user's home directory
    const char *home = getenv("HOME");
    if (!home) {
        struct passwd *pw = getpwuid(getuid());
        if (!pw) return;
        home = pw->pw_dir;
    }
    
    // Make sure .minux directory exists
    char dir_path[MAX_PATH];
    snprintf(dir_path, sizeof(dir_path), "%s/.minux", home);
    mkdir(dir_path, 0755);
    
    char history_path[MAX_PATH];
    snprintf(history_path, sizeof(history_path), "%s/.minux/%s", home, HISTORY_FILE);
    
    FILE *fp = fopen(history_path, "w");
    if (!fp) return;
    
    for (int i = 0; i < history_count; i++) {
        fprintf(fp, "%s\n", command_history[i]);
    }
    
    fclose(fp);
}

char *get_history_entry(int index) {
    if (index < 0 || index >= history_count) {
        return NULL;
    }
    return command_history[index];
}

int history_size() {
    return history_count;
}

int main(void) {
    // Set up locale and UTF-8 support BEFORE ncurses init
    setlocale(LC_ALL, "");
    const char *env_var = "NCURSES_NO_UTF8_ACS=1";
    putenv((char *)env_var);

    init_windows();
    getcwd(current_path, sizeof(current_path));

    display_welcome_banner();

    // Show welcome message with version
    clear();
    int y = 1;
    mvprintw(y++, 1, "MINUX %s", VERSION);
    
    // Display platform information in ncurses
    bool is_raspberry_pi = false;
    FILE *model_file = fopen("/sys/firmware/devicetree/base/model", "r");
    if (model_file) {
        char model[256] = {0};
        if (fgets(model, sizeof(model), model_file)) {
            // Remove trailing newline if present
            size_t len = strlen(model);
            if (len > 0 && model[len-1] == '\n') {
                model[len-1] = '\0';
            }
            mvprintw(y++, 1, "Platform: %s", model);
            if (strstr(model, "Raspberry Pi") != NULL) {
                is_raspberry_pi = true;
            }
        }
        fclose(model_file);
    } else {
        // Try to get system information in other ways
        FILE *uname_cmd = popen("uname -sr", "r");
        if (uname_cmd) {
            char uname_output[256] = {0};
            if (fgets(uname_output, sizeof(uname_output), uname_cmd)) {
                // Remove trailing newline if present
                size_t len = strlen(uname_output);
                if (len > 0 && uname_output[len-1] == '\n') {
                    uname_output[len-1] = '\0';
                }
                mvprintw(y++, 1, "Platform: %s", uname_output);
            }
            pclose(uname_cmd);
        } else {
            mvprintw(y++, 1, "Platform: Unknown");
        }
    }
    
    // Display GPIO support status
    if (is_raspberry_pi) {
        mvprintw(y++, 1, "GPIO Support: Available (run 'gpio' command to view)");
    } else {
        mvprintw(y++, 1, "GPIO Support: Not available (requires Raspberry Pi)");
    }
    
    // Empty line and help message
    y++;
    mvprintw(y++, 1, "Type 'help' for available commands");
    
    // Add another empty line before prompt
    y++;
    move(y, 0);
    refresh();

    // Initialize the command history
    load_history();

    // Show the initial prompt
    show_prompt();

    char cmd[MAX_CMD_LENGTH];
    int cmd_pos = 0;
    int ch;

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
                printw("\n");  // Add a newline before executing the command
                handle_command(cmd);
                cmd_pos = 0;
                history_position = history_count;  // Reset history position
                break;

            case KEY_BACKSPACE:
            case 127:  // Also handle DEL key
                if (cmd_pos > 0) {
                    cmd_pos--;
                    // Erase character on screen
                    int y, x;
                    getyx(stdscr, y, x);
                    move(y, x - 1);
                    delch();
                    refresh();
                }
                break;
                
            case KEY_UP:  // Previous command in history
                if (history_position > 0) {
                    history_position--;
                    // Clear current command
                    int y, x, start_x;
                    getyx(stdscr, y, x);
                    start_x = x - cmd_pos;
                    move(y, start_x);
                    for (int i = 0; i < cmd_pos; i++) {
                        delch();
                    }
                    
                    // Display the history entry
                    char *hist_cmd = get_history_entry(history_position);
                    if (hist_cmd) {
                        strncpy(cmd, hist_cmd, MAX_CMD_LENGTH - 1);
                        cmd[MAX_CMD_LENGTH - 1] = '\0';
                        cmd_pos = strlen(cmd);
                        printw("%s", cmd);
                    }
                }
                break;
                
            case KEY_DOWN:  // Next command in history
                if (history_position < history_count) {
                    history_position++;
                    
                    // Clear current command
                    int y, x, start_x;
                    getyx(stdscr, y, x);
                    start_x = x - cmd_pos;
                    move(y, start_x);
                    for (int i = 0; i < cmd_pos; i++) {
                        delch();
                    }
                    
                    // If at the end of history, show an empty command
                    if (history_position == history_count) {
                        cmd[0] = '\0';
                        cmd_pos = 0;
                    } else {
                        // Display the history entry
                        char *hist_cmd = get_history_entry(history_position);
                        if (hist_cmd) {
                            strncpy(cmd, hist_cmd, MAX_CMD_LENGTH - 1);
                            cmd[MAX_CMD_LENGTH - 1] = '\0';
                            cmd_pos = strlen(cmd);
                            printw("%s", cmd);
                        }
                    }
                }
                break;
                
            case KEY_LEFT:  // Move cursor left
                if (cmd_pos > 0) {
                    cmd_pos--;
                    // Move cursor left
                    int y, x;
                    getyx(stdscr, y, x);
                    move(y, x - 1);
                    refresh();
                }
                break;
                
            case KEY_RIGHT:  // Move cursor right
                if (cmd_pos < (int)strlen(cmd)) {
                    cmd_pos++;
                    // Move cursor right
                    int y, x;
                    getyx(stdscr, y, x);
                    move(y, x + 1);
                    refresh();
                }
                break;
                
            case KEY_HOME:  // Move to beginning of line
                {
                    int y, x, start_x;
                    getyx(stdscr, y, x);
                    start_x = x - cmd_pos;
                    move(y, start_x);
                    cmd_pos = 0;
                    refresh();
                }
                break;
                
            case KEY_END:  // Move to end of line
                {
                    int y, x, start_x;
                    getyx(stdscr, y, x);
                    start_x = x - cmd_pos;
                    cmd_pos = (int)strlen(cmd);
                    move(y, start_x + cmd_pos);
                    refresh();
                }
                break;
                
            case KEY_DC:  // Delete key (delete under cursor)
                if (cmd_pos < (int)strlen(cmd)) {
                    // Shift all characters left
                    memmove(&cmd[cmd_pos], &cmd[cmd_pos + 1], strlen(cmd) - cmd_pos);
                    
                    // Redraw the command line
                    int y, x, start_x;
                    getyx(stdscr, y, x);
                    start_x = x - cmd_pos;
                    move(y, start_x);
                    for (int i = 0; i < (int)strlen(cmd); i++) {
                        delch();
                    }
                    move(y, start_x);
                    printw("%s", cmd);
                    move(y, start_x + cmd_pos);
                    refresh();
                }
                break;

            default:
                if (cmd_pos < MAX_CMD_LENGTH - 1 && ch >= 32 && ch <= 126) {
                    // If cursor is in the middle of the command, make room for the new character
                    if (cmd_pos < (int)strlen(cmd)) {
                        memmove(&cmd[cmd_pos + 1], &cmd[cmd_pos], strlen(cmd) - cmd_pos + 1);
                        
                        // Insert the character
                        cmd[cmd_pos] = ch;
                        
                        // Redraw the command line from the cursor position
                        int y, x;
                        getyx(stdscr, y, x);
                        insch(ch);  // Insert the character at cursor position
                        
                        // Move cursor forward
                        move(y, x + 1);
                    } else {
                        // Simply append to the end
                        cmd[cmd_pos] = ch;
                        cmd[cmd_pos + 1] = '\0';
                        printw("%c", ch);
                    }
                    cmd_pos++;
                    refresh();
                }
                break;
        }
    }

    cleanup();
    return 0;
} 