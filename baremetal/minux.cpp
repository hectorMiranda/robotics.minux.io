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
    init_pair(3, COLOR_CYAN, COLOR_BLACK);   // Symlinks
    
    // Print header row
    mvprintw(y++, 1, "%-10s %-8s %-8s %8s %-12s %s", "Permissions", "Owner", "Group", "Size", "Modified", "Name");
    mvprintw(y++, 1, "--------------------------------------------------------------------------");

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
            mvprintw(y++, 1, "%-10s %-8s %-8s %8s %-12s %s", 
                    perm, owner, group, size_str, time_str, entries[i]->d_name);
            
            // Restore normal attributes
            attroff(COLOR_PAIR(1) | COLOR_PAIR(2) | COLOR_PAIR(3) | A_BOLD);
        }
    }
    
    mvprintw(y + 1, 1, " ");  // Move cursor to end with a space
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
    
    // Define y here, outside the conditional blocks
    int y = 1;
    
    // Check if we're on a Raspberry Pi using multiple methods
    bool is_raspberry_pi = false;
    char model[256] = {0};
    
    // Method 1: Check /sys/firmware/devicetree/base/model
    FILE *model_file = fopen("/sys/firmware/devicetree/base/model", "r");
    if (model_file) {
        if (fgets(model, sizeof(model), model_file)) {
            if (strstr(model, "Raspberry Pi") != NULL) {
                is_raspberry_pi = true;
            }
        }
        fclose(model_file);
    }
    
    // Method 2: Check /proc/cpuinfo for Raspberry Pi-specific hardware
    if (!is_raspberry_pi) {
        FILE *cpuinfo = fopen("/proc/cpuinfo", "r");
        if (cpuinfo) {
            char line[256];
            while (fgets(line, sizeof(line), cpuinfo)) {
                if (strstr(line, "BCM") || strstr(line, "Raspberry")) {
                    is_raspberry_pi = true;
                    snprintf(model, sizeof(model), "Raspberry Pi (detected via cpuinfo)");
                    break;
                }
            }
            fclose(cpuinfo);
        }
    }
    
    if (!is_raspberry_pi) {
        mvprintw(y++, 1, "GPIO support only available on Raspberry Pi");
        log_error(error_console, ERROR_INFO, "MINUX", 
                  "GPIO support only available on Raspberry Pi");
        mvprintw(y + 2, 1, "Press any key to continue...");
        refresh();
        getch();
        return;
    }
    
    mvprintw(y++, 1, "Raspberry Pi detected: %s", model);
    
    // For Raspberry Pi 5, check model string
    bool model_5 = strstr(model, "Raspberry Pi 5") != NULL;
    
#if HAS_PIGPIO
    // Only declare this variable when HAS_PIGPIO is defined to avoid unused variable warning
    bool using_direct_access = false;
    
    // Try to initialize pigpio
    int pi_init = -1;
    
    // Additional check specifically for Pi 5
    if (model_5) {
        mvprintw(y++, 1, "Raspberry Pi 5 detected - pigpio might need updating");
        mvprintw(y++, 1, "Will attempt to use gpiochip interface if pigpio fails");
    }
    
    // Try initializing pigpio
    pi_init = gpioInitialise();
    
    if (pi_init < 0) {
        mvprintw(y++, 1, "Failed to initialize pigpio interface. Error code: %d", pi_init);
        
        if (model_5) {
            mvprintw(y++, 1, "This is expected since pigpio hasn't been updated for Pi 5 yet.");
            mvprintw(y++, 1, "Using direct GPIO access via modern gpiod interface");
            using_direct_access = true;
        } else {
            mvprintw(y++, 1, "Make sure pigpio daemon is running with: sudo pigpiod");
            mvprintw(y++, 1, "Or run MINUX with sudo privileges");
            refresh();
            log_error(error_console, ERROR_WARNING, "MINUX", 
                      "Failed to initialize GPIO interface. Try running: sudo pigpiod");
            mvprintw(y + 2, 1, "Press any key to continue...");
            refresh();
            getch();
            return;
        }
    }
    
    // If we need to use direct access or if pigpio initialization failed
    if (using_direct_access) {
        // Display table header
        y = 5;  // Reset y to leave space for messages above
        mvprintw(y++, 1, "+-----+-----+---------+------+---+---Pi %s--+---+------+---------+-----+-----+", 
                model_5 ? "5" : "3B");
        mvprintw(y++, 1, "| BCM | wPi |   Name  | Mode | V | Physical | V | Mode | Name    | wPi | BCM |");
        mvprintw(y++, 1, "+-----+-----+---------+------+---+----++----+---+------+---------+-----+-----+");
        
        // Print 3.3V, 5V, and GND (0V) pins
        mvprintw(y++, 1, "|     |     |    3.3v |      |   |  1 || 2  |   |      | 5v      |     |     |");
        
        // Try to use the gpiod utility if available (newer Raspberry Pi OS)
        FILE *gpio_cmd = popen("which gpioinfo >/dev/null 2>&1 && gpioinfo", "r");
        bool used_gpiod = false;
        
        if (gpio_cmd) {
            // Read all GPIO info into a buffer for parsing
            char buffer[10240] = {0};
            size_t total_read = 0;
            size_t bytes_read;
            
            while ((bytes_read = fread(buffer + total_read, 1, sizeof(buffer) - total_read - 1, gpio_cmd)) > 0) {
                total_read += bytes_read;
                if (total_read >= sizeof(buffer) - 1) break;
            }
            buffer[total_read] = '\0';
            
            if (total_read > 0) {
                used_gpiod = true;
                
                // Loop through pins by physical order
                for (int physical = 3; physical <= 40; physical += 2) {
                    char leftPin[100] = "|     |     |      0v |      |   |";
                    char rightPin[100] = "|   |      | 0v      |     |     |";
                    
                    // Find info for left pin (odd numbers)
                    for (int i = 0; pinInfoTable[i].name != NULL; i++) {
                        if (pinInfoTable[i].physical == physical) {
                            int bcm_pin = pinInfoTable[i].bcm;
                            
                            // Try to find this pin in the gpioinfo output
                            char search_str[20];
                            sprintf(search_str, "GPIO %d:", bcm_pin);
                            
                            char *pin_info = strstr(buffer, search_str);
                            char mode[5] = "---";
                            int value = -1;
                            
                            if (pin_info) {
                                // Parse line to extract mode and value
                                if (strstr(pin_info, "input")) {
                                    strcpy(mode, "IN");
                                } else if (strstr(pin_info, "output")) {
                                    strcpy(mode, "OUT");
                                }
                                
                                // Extract value
                                if (strstr(pin_info, "active-high")) {
                                    value = strstr(pin_info, "val:1") ? 1 : 0;
                                } else {
                                    value = strstr(pin_info, "val:1") ? 0 : 1; // Inverted logic for active-low
                                }
                            }
                            
                            sprintf(leftPin, "| %3d | %3d | %7s | %4s | %d |", 
                                    pinInfoTable[i].bcm, 
                                    pinInfoTable[i].wPi, 
                                    pinInfoTable[i].name, 
                                    mode,
                                    value);
                            break;
                        }
                    }
                    
                    // Find info for right pin (even numbers)
                    for (int i = 0; pinInfoTable[i].name != NULL; i++) {
                        if (pinInfoTable[i].physical == physical + 1) {
                            int bcm_pin = pinInfoTable[i].bcm;
                            
                            // Try to find this pin in the gpioinfo output
                            char search_str[20];
                            sprintf(search_str, "GPIO %d:", bcm_pin);
                            
                            char *pin_info = strstr(buffer, search_str);
                            char mode[5] = "---";
                            int value = -1;
                            
                            if (pin_info) {
                                // Parse line to extract mode and value
                                if (strstr(pin_info, "input")) {
                                    strcpy(mode, "IN");
                                } else if (strstr(pin_info, "output")) {
                                    strcpy(mode, "OUT");
                                }
                                
                                // Extract value
                                if (strstr(pin_info, "active-high")) {
                                    value = strstr(pin_info, "val:1") ? 1 : 0;
                                } else {
                                    value = strstr(pin_info, "val:1") ? 0 : 1; // Inverted logic for active-low
                                }
                            }
                            
                            sprintf(rightPin, "| %d | %4s | %-8s | %3d | %3d |", 
                                    value,
                                    mode,
                                    pinInfoTable[i].name, 
                                    pinInfoTable[i].wPi, 
                                    pinInfoTable[i].bcm);
                            break;
                        }
                    }
                    
                    // Print the line with both pins
                    mvprintw(y++, 1, "%s %2d || %2d %s", leftPin, physical, physical + 1, rightPin);
                }
            }
            
            pclose(gpio_cmd);
        }
        
        // If gpiod didn't work, fall back to sysfs
        if (!used_gpiod) {
            // Loop through pins by physical order
            for (int physical = 3; physical <= 40; physical += 2) {
                char leftPin[100] = "|     |     |      0v |      |   |";
                char rightPin[100] = "|   |      | 0v      |     |     |";
                
                // Find info for left pin (odd numbers)
                for (int i = 0; pinInfoTable[i].name != NULL; i++) {
                    if (pinInfoTable[i].physical == physical) {
                        // Check if the GPIO exists in sysfs
                        char path[128];
                        snprintf(path, sizeof(path), "/sys/class/gpio/gpio%d/value", pinInfoTable[i].bcm);
                        
                        char mode[5] = "---";
                        int value = -1;
                        
                        // Try to read direction
                        char dir_path[128];
                        snprintf(dir_path, sizeof(dir_path), "/sys/class/gpio/gpio%d/direction", pinInfoTable[i].bcm);
                        FILE *dir_file = fopen(dir_path, "r");
                        if (dir_file) {
                            char dir[10] = {0};
                            if (fgets(dir, sizeof(dir), dir_file)) {
                                if (strstr(dir, "in")) {
                                    strcpy(mode, "IN");
                                } else if (strstr(dir, "out")) {
                                    strcpy(mode, "OUT");
                                }
                            }
                            fclose(dir_file);
                        }
                        
                        // Try to read value
                        FILE *val_file = fopen(path, "r");
                        if (val_file) {
                            char val[2] = {0};
                            if (fgets(val, sizeof(val), val_file)) {
                                value = atoi(val);
                            }
                            fclose(val_file);
                        }
                        
                        sprintf(leftPin, "| %3d | %3d | %7s | %4s | %d |", 
                                pinInfoTable[i].bcm, 
                                pinInfoTable[i].wPi, 
                                pinInfoTable[i].name, 
                                mode,
                                value);
                        break;
                    }
                }
                
                // Find info for right pin (even numbers)
                for (int i = 0; pinInfoTable[i].name != NULL; i++) {
                    if (pinInfoTable[i].physical == physical + 1) {
                        // Check if the GPIO exists in sysfs
                        char path[128];
                        snprintf(path, sizeof(path), "/sys/class/gpio/gpio%d/value", pinInfoTable[i].bcm);
                        
                        char mode[5] = "---";
                        int value = -1;
                        
                        // Try to read direction
                        char dir_path[128];
                        snprintf(dir_path, sizeof(dir_path), "/sys/class/gpio/gpio%d/direction", pinInfoTable[i].bcm);
                        FILE *dir_file = fopen(dir_path, "r");
                        if (dir_file) {
                            char dir[10] = {0};
                            if (fgets(dir, sizeof(dir), dir_file)) {
                                if (strstr(dir, "in")) {
                                    strcpy(mode, "IN");
                                } else if (strstr(dir, "out")) {
                                    strcpy(mode, "OUT");
                                }
                            }
                            fclose(dir_file);
                        }
                        
                        // Try to read value
                        FILE *val_file = fopen(path, "r");
                        if (val_file) {
                            char val[2] = {0};
                            if (fgets(val, sizeof(val), val_file)) {
                                value = atoi(val);
                            }
                            fclose(val_file);
                        }
                        
                        sprintf(rightPin, "| %d | %4s | %-8s | %3d | %3d |", 
                                value,
                                mode,
                                pinInfoTable[i].name, 
                                pinInfoTable[i].wPi, 
                                pinInfoTable[i].bcm);
                        break;
                    }
                }
                
                // Print the line with both pins
                mvprintw(y++, 1, "%s %2d || %2d %s", leftPin, physical, physical + 1, rightPin);
            }
        }
        
        // Print table footer
        mvprintw(y++, 1, "+-----+-----+---------+------+---+----++----+---+------+---------+-----+-----+");
        mvprintw(y++, 1, "| BCM | wPi |   Name  | Mode | V | Physical | V | Mode | Name    | wPi | BCM |");
        mvprintw(y++, 1, "+-----+-----+---------+------+---+---Pi %s--+---+------+---------+-----+-----+", 
                 model_5 ? "5" : "3B");
        
        mvprintw(y++, 1, "NOTE: Using direct GPIO access for Raspberry Pi 5 (pigpio not compatible)");
        if (!used_gpiod) {
            mvprintw(y++, 1, "Some GPIO pins may not show correct status without sudo access");
        }
    } else {
        // Using pigpio successfully - display GPIO status in a formatted table
        y = 5;  // Reset y here to leave space for the model info
        mvprintw(y++, 1, "+-----+-----+---------+------+---+---Pi %s--+---+------+---------+-----+-----+", 
                model_5 ? "5" : "3B");
        mvprintw(y++, 1, "| BCM | wPi |   Name  | Mode | V | Physical | V | Mode | Name    | wPi | BCM |");
        mvprintw(y++, 1, "+-----+-----+---------+------+---+----++----+---+------+---------+-----+-----+");
        
        // Print 3.3V, 5V, and GND (0V) pins
        mvprintw(y++, 1, "|     |     |    3.3v |      |   |  1 || 2  |   |      | 5v      |     |     |");
        
        // Loop through pins by physical order
        for (int physical = 3; physical <= 40; physical += 2) {
            char leftPin[100] = "|     |     |      0v |      |   |";
            char rightPin[100] = "|   |      | 0v      |     |     |";
            
            // Find info for left pin (odd numbers)
            for (int i = 0; pinInfoTable[i].name != NULL; i++) {
                if (pinInfoTable[i].physical == physical) {
                    int mode = gpioGetMode(pinInfoTable[i].bcm);
                    int value = gpioRead(pinInfoTable[i].bcm);
                    sprintf(leftPin, "| %3d | %3d | %7s | %4s | %d |", 
                            pinInfoTable[i].bcm, 
                            pinInfoTable[i].wPi, 
                            pinInfoTable[i].name, 
                            mode == PI_INPUT ? "IN" : (mode == PI_OUTPUT ? "OUT" : "ALT"),
                            value);
                    break;
                }
            }
            
            // Find info for right pin (even numbers)
            for (int i = 0; pinInfoTable[i].name != NULL; i++) {
                if (pinInfoTable[i].physical == physical + 1) {
                    int mode = gpioGetMode(pinInfoTable[i].bcm);
                    int value = gpioRead(pinInfoTable[i].bcm);
                    sprintf(rightPin, "| %d | %4s | %-8s | %3d | %3d |", 
                            value,
                            mode == PI_INPUT ? "IN" : (mode == PI_OUTPUT ? "OUT" : "ALT"),
                            pinInfoTable[i].name, 
                            pinInfoTable[i].wPi, 
                            pinInfoTable[i].bcm);
                    break;
                }
            }
            
            // Print the line with both pins
            mvprintw(y++, 1, "%s %2d || %2d %s", leftPin, physical, physical + 1, rightPin);
        }
        
        mvprintw(y++, 1, "+-----+-----+---------+------+---+----++----+---+------+---------+-----+-----+");
        mvprintw(y++, 1, "| BCM | wPi |   Name  | Mode | V | Physical | V | Mode | Name    | wPi | BCM |");
        mvprintw(y++, 1, "+-----+-----+---------+------+---+---Pi %s--+---+------+---------+-----+-----+", 
                 model_5 ? "5" : "3B");
        
        // Cleanup 
        gpioTerminate();
    }
#else
    // pigpio library not available or not compiled in
    mvprintw(y++, 1, "GPIO library (pigpio) is not enabled in this build.");
    
    // Try to use gpioinfo as a fallback
    FILE *gpio_cmd = popen("which gpioinfo >/dev/null 2>&1 && gpioinfo gpiochip0", "r");
    if (gpio_cmd) {
        mvprintw(y++, 1, "Using Linux libgpiod utilities as fallback:");
        y++;
        
        char line[1024];
        while (fgets(line, sizeof(line), gpio_cmd)) {
            mvprintw(y++, 1, "%s", line);
            // Limit to prevent overflow
            if (y > screen_height - 5) break;
        }
        pclose(gpio_cmd);
    } else {
        mvprintw(y++, 1, "Please install GPIO libraries for full support:");
        mvprintw(y++, 1, "    sudo apt-get update");
        mvprintw(y++, 1, "    sudo apt-get install libpigpio-dev libgpiod-dev gpiod");
        mvprintw(y++, 1, "Then recompile the application with: make clean && make");
    }
    
    // For Pi 5, add additional information
    if (model_5) {
        mvprintw(y++, 1, "");
        mvprintw(y++, 1, "Note: Raspberry Pi 5 requires updated GPIO libraries.");
        mvprintw(y++, 1, "The current version of pigpio might not recognize Pi 5 hardware.");
        mvprintw(y++, 1, "Consider using gpiod library which has better Pi 5 support.");
    }
    
    log_error(error_console, ERROR_WARNING, "MINUX", 
              "GPIO functionality requires GPIO libraries to be installed and enabled.");
#endif
    
    mvprintw(y + 2, 1, "Press any key to continue...");
    refresh();
    getch();
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
    // Move to a new line first to ensure we don't get double prompts
    printw("\n");
    
    // Display a more informative prompt with current path
    printw("minux > %s > ", current_path);
    refresh();
}

void launch_explorer(void) {
    // Check if explorer executable exists in various possible locations
    const char* explorer_paths[] = {
        "./explorer",                // Current directory
        "../explorer",               // Parent directory
        "/usr/local/bin/explorer",   // System location
        NULL
    };
    
    const char* explorer_path = NULL;
    for (int i = 0; explorer_paths[i] != NULL; i++) {
        if (access(explorer_paths[i], X_OK) == 0) {
            explorer_path = explorer_paths[i];
            break;
        }
    }
    
    // Launch explorer if found
    if (explorer_path != NULL) {
        endwin();  // Temporarily end ncurses mode
        int result = system(explorer_path);
        if (result != 0) {
            initscr();  // Restore ncurses mode
            log_error(error_console, ERROR_CRITICAL, "MINUX", 
                      "Explorer process exited with error code: %d", result);
        } else {
            initscr();  // Restore ncurses mode
            log_error(error_console, ERROR_SUCCESS, "MINUX", 
                      "Explorer session ended successfully");
        }
    } else {
        // Explorer not found, show error
        log_error(error_console, ERROR_CRITICAL, "MINUX", 
                  "Explorer executable not found. Please ensure 'explorer' is built and in the path.");
        
        // Display error message on screen
        clear();
        mvprintw(screen_height/2 - 2, (screen_width - 40)/2, "Explorer executable not found!");
        mvprintw(screen_height/2 - 1, (screen_width - 40)/2, "Please ensure 'explorer' is built.");
        mvprintw(screen_height/2, (screen_width - 40)/2, "Press any key to continue...");
        refresh();
        getch();
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
    else if (strcmp(args[0], "tree") == 0) {
        // Special handling for tree command with arguments
        clear();
        
        // Default settings
        const char *path = ".";  // Default to current directory
        bool show_hidden = false;
        int max_depth = -1;  // -1 means unlimited
        bool interactive = false;
        
        // Parse any provided arguments
        for (int i = 1; i < argc; i++) {
            if (strcmp(args[i], "-i") == 0) {
                interactive = true;
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
            mvprintw(1, 1, "%s", resolved_path);
            attroff(COLOR_PAIR(1) | A_BOLD);
            
            // Reset position for tree display
            int y = 3;
            
            // Use recursive display for Unix-like functionality
            int dir_count = 0;
            int file_count = 0;
            
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
            closedir(dir);
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
    for (size_t i = 0; i < indices.size(); i++) {
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
        } else if (st.st_mode & S_IXUSR) {
            attron(COLOR_PAIR(2) | A_BOLD); // Green for executables
            mvprintw(y, 5, "%s", name.c_str());
            attroff(COLOR_PAIR(2) | A_BOLD);
            file_count++;
        } else {
            mvprintw(y, 5, "%s", name.c_str());
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

int main(void) {
    // Set up locale and UTF-8 support BEFORE ncurses init
    setlocale(LC_ALL, "");
    const char *env_var = "NCURSES_NO_UTF8_ACS=1";
    putenv((char *)env_var);

    init_windows();
    getcwd(current_path, sizeof(current_path));

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
                // No need to print a newline here, show_prompt() will do it
                handle_command(cmd);
                cmd_pos = 0;
                // Don't call show_prompt() here, it's called in handle_command()
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