## Projects

### Marcebot

The Marcebot is a small, autonomous robot designed for navigation and object avoidance. It uses a differential drive system with two stepper motors and an ultrasonic sensor for obstacle detection. The robot's behavior is controlled by an Arduino Nano, which runs a program that implements three modes: collision avoidance, dead reckoning, and path planning.

- **Collision Avoidance Mode**: The robot moves forward and uses the ultrasonic sensor to detect obstacles. When an obstacle is detected, the robot stops, moves backward, and turns to avoid the obstacle.
- **Dead Reckoning Mode**: The robot follows a predefined path of forward movements and turns.
- **Path Planning Mode**: The robot navigates a path while avoiding obstacles.

The `arduino/marcebot` directory contains the Arduino code for the robot, including the main `.ino` file and the `Robot` class, which provides an API for controlling the robot's movements.

### Digital Clock

This project is a digital clock with an alarm function, built using an ATmega328P microcontroller and a 7-segment display. The clock displays the time in 12-hour format and includes a PM indicator. The user can set the time and alarm using pushbuttons. When the alarm is triggered, it plays a Pac-Man-themed melody.

The `arduino/clock` directory contains the Arduino code for the clock, including the main `.ino` file and the logic for the alarm and display.

### Baremetal Minux

This project is a lightweight, ncurses-based file explorer with multi-tab support and modern features, designed for minimal resource usage and maximum efficiency. It also includes a cryptocurrency wallet and camera functionality.

The `baremetal` directory contains the source code for the file explorer, including the main `minux.cpp` file, the `explorer.cpp` file, and the `error_console.cpp` file, which implements a DOOM-style error console.

### CUDA Status Checker

This project is a Python script that checks the status of the CUDA installation on a system. It verifies the NVIDIA driver, CUDA Toolkit, and CUDA runtime libraries, and it also checks for PyTorch and TensorFlow CUDA support.

The `cuda` directory contains the `cuda_status.py` script.

### ESP32-S3 Camera

This project is a simple web server that streams video from an ESP32-S3 camera module. The server provides a web page with an MJPEG stream from the camera.

The `esp32-s3-cam` directory contains the C++ code for the camera server.

### Galileo

This project contains a collection of Arduino and Python scripts for the Intel Galileo board. The Arduino scripts include examples for creating a web server, an interactive web server, and a Wi-Fi web server with Telnet. The Python scripts include examples for controlling motors and reading from sensors.

The `galileo` directory contains the Arduino and Python scripts.

### GoPiGo

This project is a Python library for controlling the GoPiGo robot. The library provides a simple API for controlling the robot's motors, reading from sensors, and controlling the LEDs.

The `raspberrypi/src` directory contains the `gopigo.py` library.

### SolidWorks

This project contains the SolidWorks assembly files for the MIT-101 project.

The `solidworks` directory contains the SolidWorks files.

## License

This project is licensed under the MIT License. See the `LICENSE` file for details.