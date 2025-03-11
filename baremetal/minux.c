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

#define VERSION "0.0.1"
#define MAX_CMD_LENGTH 1024
#define MAX_ARGS 64
#define MAX_PATH 4096

// Command function prototypes
void cmd_help(int argc, char *argv[]);
void cmd_version(int argc, char *argv[]);
void cmd_time(int argc, char *argv[]);
void cmd_date(int argc, char *argv[]);
void cmd_path(int argc, char *argv[]);
void cmd_ls(int argc, char *argv[]);
void cmd_cd(int argc, char *argv[]);
void cmd_clear(int argc, char *argv[]);
void cmd_gpio(int argc, char *argv[]);
void launch_explorer(int argc, char *argv[]);

// Command structure
typedef struct {
    const char *name;
    void (*func)(int argc, char *argv[]);
    const char *help;
} Command;

// Available commands
Command commands[] = {
    {"help", cmd_help, "Display this help message"},
    {"version", cmd_version, "Display MINUX version"},
    {"time", cmd_time, "Display current time"},
    {"date", cmd_date, "Display current date"},
    {"path", cmd_path, "Display or modify system path"},
    {"ls", cmd_ls, "List directory contents"},
    {"cd", cmd_cd, "Change directory"},
    {"clear", cmd_clear, "Clear screen"},
    {"gpio", cmd_gpio, "Display GPIO status"},
    {"explorer", launch_explorer, "Launch file explorer"},
    {NULL, NULL, NULL}
};

// Global variables
char current_path[MAX_PATH];

// Command implementations
void cmd_help(int argc, char *argv[]) {
    printf("\nMINUX Commands:\n\n");
    for (Command *cmd = commands; cmd->name != NULL; cmd++) {
        printf("%-15s - %s\n", cmd->name, cmd->help);
    }
    printf("\n");
}

void cmd_version(int argc, char *argv[]) {
    printf("MINUX Version %s\n", VERSION);
}

void cmd_time(int argc, char *argv[]) {
    time_t t = time(NULL);
    struct tm *tm = localtime(&t);
    char time_str[64];
    strftime(time_str, sizeof(time_str), "%H:%M:%S", tm);
    printf("Current time: %s\n", time_str);
}

void cmd_date(int argc, char *argv[]) {
    time_t t = time(NULL);
    struct tm *tm = localtime(&t);
    char date_str[64];
    strftime(date_str, sizeof(date_str), "%Y-%m-%d", tm);
    printf("Current date: %s\n", date_str);
}

void cmd_path(int argc, char *argv[]) {
    char *path = getenv("PATH");
    if (path) {
        printf("System PATH:\n");
        char *token = strtok(path, ":");
        while (token != NULL) {
            printf("  %s\n", token);
            token = strtok(NULL, ":");
        }
    } else {
        printf("PATH environment variable not found\n");
    }
}

void cmd_ls(int argc, char *argv[]) {
    DIR *dir;
    struct dirent *entry;
    const char *path = argc > 1 ? argv[1] : ".";

    dir = opendir(path);
    if (dir == NULL) {
        printf("Error opening directory: %s\n", strerror(errno));
        return;
    }

    printf("\nContents of %s:\n\n", path);
    while ((entry = readdir(dir)) != NULL) {
        struct stat st;
        char full_path[MAX_PATH];
        snprintf(full_path, sizeof(full_path), "%s/%s", path, entry->d_name);
        
        if (stat(full_path, &st) == 0) {
            if (S_ISDIR(st.st_mode))
                printf("\033[1;34m%-20s\033[0m  ", entry->d_name);  // Blue for directories
            else if (st.st_mode & S_IXUSR)
                printf("\033[1;32m%-20s\033[0m  ", entry->d_name);  // Green for executables
            else
                printf("%-20s  ", entry->d_name);
        }
    }
    printf("\n\n");
    closedir(dir);
}

void cmd_cd(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: cd <directory>\n");
        return;
    }
    
    if (chdir(argv[1]) != 0) {
        printf("Error changing directory: %s\n", strerror(errno));
    } else {
        getcwd(current_path, sizeof(current_path));
    }
}

void cmd_clear(int argc, char *argv[]) {
    printf("\033[2J\033[H");  // ANSI escape sequence to clear screen
}

void cmd_gpio(int argc, char *argv[]) {
    #ifdef __arm__
    // Raspberry Pi specific GPIO handling
    FILE *fp = fopen("/sys/class/gpio/export", "w");
    if (fp == NULL) {
        printf("GPIO interface not available (are you running on Raspberry Pi?)\n");
        return;
    }
    fclose(fp);
    
    printf("GPIO Status:\n");
    system("ls -l /sys/class/gpio/");
    #else
    printf("GPIO support only available on Raspberry Pi\n");
    #endif
}

void launch_explorer(int argc, char *argv[]) {
    // Show splash screen
    printf("\033[2J\033[H");  // Clear screen
    printf("\n\n");
    printf("    ███████╗██╗  ██╗██████╗ ██╗      ██████╗ ██████╗ ███████╗██████╗\n");
    printf("    ██╔════╝╚██╗██╔╝██╔══██╗██║     ██╔═══██╗██╔══██╗██╔════╝██╔══██╗\n");
    printf("    █████╗   ╚███╔╝ ██████╔╝██║     ██║   ██║██████╔╝█████╗  ██████╔╝\n");
    printf("    ██╔══╝   ██╔██╗ ██╔═══╝ ██║     ██║   ██║██╔══██╗██╔══╝  ██╔══██╗\n");
    printf("    ███████╗██╔╝ ██╗██║     ███████╗╚██████╔╝██║  ██║███████╗██║  ██║\n");
    printf("    ╚══════╝╚═╝  ╚═╝╚═╝     ╚══════╝ ╚═════╝ ╚═╝  ╚═╝╚══════╝╚═╝  ╚═╝\n");
    printf("\n                           Version %s\n", VERSION);
    printf("\n                    Press any key to continue...\n");
    getchar();

    // Launch explorer
    pid_t pid = fork();
    if (pid == 0) {
        // Child process
        execl("./explorer", "explorer", NULL);
        exit(1);  // Only reached if execl fails
    } else if (pid > 0) {
        // Parent process
        wait(NULL);  // Wait for explorer to finish
    } else {
        printf("Error launching explorer\n");
    }
}

// Command parsing and execution
void execute_command(char *cmd_str) {
    char *argv[MAX_ARGS];
    int argc = 0;
    
    // Tokenize command
    char *token = strtok(cmd_str, " \t\n");
    while (token != NULL && argc < MAX_ARGS - 1) {
        argv[argc++] = token;
        token = strtok(NULL, " \t\n");
    }
    argv[argc] = NULL;
    
    if (argc == 0) return;
    
    // Find and execute command
    for (Command *cmd = commands; cmd->name != NULL; cmd++) {
        if (strcmp(cmd->name, argv[0]) == 0) {
            cmd->func(argc, argv);
            return;
        }
    }
    
    printf("Unknown command: %s\n", argv[0]);
}

int main() {
    char cmd[MAX_CMD_LENGTH];
    
    // Initialize
    getcwd(current_path, sizeof(current_path));
    
    // Welcome message
    printf("\033[2J\033[H");  // Clear screen
    printf("MINUX %s - Minimal Unix-like Shell\n", VERSION);
    printf("Type 'help' for available commands\n\n");
    
    // Main loop
    while (1) {
        printf("MINUX> ");
        fflush(stdout);
        
        if (fgets(cmd, sizeof(cmd), stdin) == NULL) break;
        
        // Remove trailing newline
        cmd[strcspn(cmd, "\n")] = 0;
        
        // Check for exit
        if (strcmp(cmd, "exit") == 0) break;
        
        execute_command(cmd);
    }
    
    printf("\nGoodbye!\n");
    return 0;
} 