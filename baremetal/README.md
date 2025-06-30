# MINUX Shell - Baremetal Terminal System

MINUX is a feature-rich terminal shell designed for robotics and embedded systems. It provides a comprehensive set of commands for system management, GPIO control, multimedia processing, cryptography, and more.

## Features

- **System Commands**: File system navigation, directory listing, path management
- **GPIO Control**: Hardware GPIO pin status and control (Raspberry Pi)
- **Serial Communication**: Serial monitor for device communication
- **Multimedia**: Audio playback, camera testing, image processing
- **Cryptography**: Wallet operations, hashing, encryption/decryption
- **File Management**: Built-in file explorer with multi-tab support
- **Task Management**: Todo list functionality
- **Development Tools**: Command history, logging, debugging console

## Prerequisites

### System Requirements

- **Operating System**: Linux (Ubuntu/Debian recommended), macOS support
- **Architecture**: x86_64, ARM (Raspberry Pi optimized)
- **Memory**: Minimum 256MB RAM
- **Storage**: 50MB free space

### Required Dependencies

#### Core Dependencies (All Platforms)
```bash
# Ubuntu/Debian
sudo apt-get update
sudo apt-get install build-essential g++ libncurses5-dev libmenu-dev libpanel-dev

# macOS (requires Homebrew)
brew install ncurses

# Install OpenSSL development libraries
# Ubuntu/Debian
sudo apt-get install libssl-dev

# macOS
brew install openssl@3

# Install secp256k1 for cryptocurrency operations
# Ubuntu/Debian
sudo apt-get install libsecp256k1-dev

# macOS
brew install secp256k1
```

#### Raspberry Pi Specific Dependencies (Optional)
```bash
# For GPIO support on Raspberry Pi
sudo apt-get install libpigpio-dev

# For Pi 5 users (alternative GPIO library)
sudo apt-get install libgpiod-dev gpiod

# For camera support (if using Raspberry Pi camera)
sudo apt-get install libcamera-dev
```

#### Audio Support (Optional)
```bash
# For audio playback functionality
sudo apt-get install alsa-utils
```

## Installation

### 1. Clone the Repository
```bash
git clone git@github.com:hectorMiranda/robotics.minux.io.git
cd robotics.minux.io/baremetal
```

### 2. Build the System
```bash
# Build both minux shell and explorer
make

# Or build specific components
make minux      # Build only the shell
make explorer   # Build only the file explorer
```

### 3. Install (Optional)
```bash
# Create system installation directories
sudo mkdir -p /usr/local/bin
sudo mkdir -p /var/log/minux

# Install binaries
sudo cp minux /usr/local/bin/
sudo cp explorer /usr/local/bin/

# Set permissions for log directory
sudo chmod 755 /var/log/minux
sudo chown $USER:$USER /var/log/minux
```

## Usage

### Starting MINUX Shell
```bash
# Run from build directory
./minux

# Or if installed system-wide
minux
```

### Starting File Explorer
```bash
# Run from build directory
./explorer

# Or if installed system-wide
explorer
```

## Available Commands

### System Commands
- `help` - Display help message with all available commands
- `version` - Show MINUX version information
- `time` - Display current time
- `date` - Display current date
- `clear` - Clear the terminal screen
- `path` - Display system PATH environment variable

### File System Commands
- `ls [directory]` - List directory contents
- `cd [directory]` - Change current directory
- `cat [filename]` - Display file contents
- `tree` - Display directory structure in tree format
- `explorer` - Launch interactive file explorer

### Hardware Commands (Raspberry Pi)
- `gpio` - Display GPIO pin status and information
- `serial` - Open serial monitor for device communication
- `test camera` - Test camera functionality

### Multimedia Commands
- `play [file|note|scale]` - Play audio files, musical notes, or scales
  - Examples: `play song.wav`, `play C4`, `play major`

### Development & Productivity
- `history` - Display command history
- `log [message]` - Add entry to system log
- `todo [add|list|done|remove|clear] [args]` - Task management
  - `todo add "Task description"` - Add new task
  - `todo list` - Show all tasks
  - `todo done 1` - Mark task 1 as completed
  - `todo remove 1` - Remove task 1
  - `todo clear` - Clear all tasks

### Cryptography & Security
- `crypto [generate|hash|encrypt|decrypt] [data]` - Cryptographic operations
- `wallet [generate|balance|send] [args]` - Cryptocurrency wallet operations

## Configuration

### Environment Setup
The shell automatically detects your environment and configures itself accordingly:

- **Raspberry Pi Detection**: Automatically enables GPIO support if pigpio is available
- **Platform Detection**: Optimizes build flags for Linux/macOS
- **Library Detection**: Gracefully handles missing optional libraries

### Build Configuration
The Makefile supports several build configurations:

```bash
# Force Raspberry Pi mode (uncomment in Makefile)
IS_PI = 1

# Build without pigpio (even on Raspberry Pi)
make HAS_PIGPIO=0

# Debug build
make CXXFLAGS+="-DDEBUG"
```

### Serial Configuration
Default serial port settings:
- **Device**: Auto-detected (typically `/dev/ttyUSB0` or `/dev/ttyACM0`)
- **Baud Rate**: 115200
- **Data Bits**: 8
- **Stop Bits**: 1
- **Parity**: None

## File Structure

```
baremetal/
├── Makefile              # Build configuration
├── minux.cpp             # Main shell implementation
├── explorer.cpp          # File explorer implementation
├── error_console.cpp     # Error handling and logging
├── error_console.h       # Error console header
├── README.md             # This file
└── test_images/          # Sample images for testing
    ├── daylight.jpg
    ├── night_vision.jpg
    └── restored_daylight.jpg
```

## Troubleshooting

### Common Issues

#### Build Errors
```bash
# Missing ncurses
sudo apt-get install libncurses5-dev

# Missing OpenSSL
sudo apt-get install libssl-dev

# Missing secp256k1
sudo apt-get install libsecp256k1-dev
```

#### Permission Issues
```bash
# GPIO access (Raspberry Pi)
sudo usermod -a -G gpio $USER
# Then logout and login again

# Serial port access
sudo usermod -a -G dialout $USER
# Then logout and login again
```

#### GPIO Not Working
1. Ensure you're on a Raspberry Pi
2. Install pigpio: `sudo apt-get install libpigpio-dev`
3. For Pi 5, use gpiod instead: `sudo apt-get install libgpiod-dev`
4. Rebuild: `make clean && make`

#### Audio Not Working
```bash
# Install ALSA
sudo apt-get install alsa-utils

# Test audio
speaker-test -c2 -t wav
```

### Debug Mode
Enable debug output by setting environment variable:
```bash
export MINUX_DEBUG=1
./minux
```

### Log Files
System logs are stored in:
- `/var/log/minux/error.log` - Error messages
- `~/.minux_history` - Command history

## Development

### Adding New Commands
1. Add function prototype to `minux.cpp`
2. Implement the function
3. Add entry to `commands[]` array
4. Rebuild with `make`

### Testing
```bash
# Clean build
make clean && make

# Test on different platforms
make # Test auto-detection

# Test without optional libraries
make HAS_PIGPIO=0
```

## Contributing

1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Test on target platforms
5. Submit a pull request

## License

This project is part of the robotics.minux.io framework. See the main repository for license information.

## Version History

- **v0.0.1** - Initial release with core shell functionality
- Features: Basic commands, GPIO support, file explorer, crypto operations

