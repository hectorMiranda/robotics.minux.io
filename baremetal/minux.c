#include <stddef.h>
#include <ncurses.h>
#include "error_console.h"

// Initialize the error console globally
ErrorConsole *error_console = NULL;

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

void cmd_gpio(void) {
    clear();
    
    // Define y here, outside the conditional blocks
    int y = 1;
    
#if defined(__has_include)
#if __has_include(<pigpio.h>)
    // Initialize pigpio if needed
    if (gpioInitialise() < 0) {
        mvprintw(1, 1, "Failed to initialize GPIO interface");
        refresh();
        log_error(error_console, ERROR_WARNING, "MINUX", 
                  "Failed to initialize GPIO interface");
        return;
    }
    
    // Display GPIO status in a formatted table
    y = 1;  // Reset y here
    mvprintw(y++, 1, "+-----+-----+---------+------+---+---Pi 3B--+---+------+---------+-----+-----+");
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
    mvprintw(y++, 1, "+-----+-----+---------+------+---+---Pi 3B--+---+------+---------+-----+-----+");
    
    // Cleanup 
    gpioTerminate();
#else
    // If pigpio is not available
    mvprintw(y++, 1, "GPIO support not available (pigpio library not found)");
    log_error(error_console, ERROR_INFO, "MINUX", 
              "GPIO support not available (pigpio library not found)");
#endif
#else
    // If __has_include is not supported
    mvprintw(y++, 1, "GPIO support not available (compiler does not support feature detection)");
    log_error(error_console, ERROR_INFO, "MINUX", 
              "GPIO support not available (compiler does not support feature detection)");
#endif

    mvprintw(y + 2, 1, "Press any key to continue...");
    refresh();
    getch();
} 