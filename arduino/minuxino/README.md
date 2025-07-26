# Instructions on how to run Minuxino, the minux instance for arduino
**PlatformIO + WSL + Arduino Nano (CH340)**

This project controls a 128x64 I²C SSD1306 OLED to animate blinking robot eyes, a loading bar, bouncing text, and a fake clock — perfect for expressive robot faces or demo displays.

---

## ⚙️ Hardware Requirements

- Arduino Nano (ATmega328P, CH340 chip)
- SSD1306 OLED Display (128x64, I2C)
- USB cable (for flashing)
- CH340 drivers installed on Windows (required for COM port access)

---

## 🧰 Software Stack

| Component      | Platform       | Purpose                        |
|----------------|----------------|--------------------------------|
| PlatformIO CLI | WSL (Ubuntu)   | Code compilation, library mgmt |
| VS Code        | Windows + WSL  | Editor                         |
| Python + PIO   | Windows         | For uploading via COM port     |

---

## 📁 Folder Structure

```text
robot_eyes/
├── src/
│   └── main.cpp           # Your robot eyes code
├── platformio.ini         # Project config
└── README.md              # This file
```

---

## 🛠️ Setup Instructions

### 1. In WSL (Ubuntu Terminal)

```bash
# Install required libraries
pio lib install "adafruit/Adafruit GFX Library"
pio lib install "adafruit/Adafruit SSD1306"

# Paste your code into src/main.cpp
```

### 2. Build the code (still in WSL)

```bash
pio run
```

---

## ⚡ Uploading Code to Arduino Nano (from Windows)

Since **WSL2 cannot access USB/COM ports directly**, use Windows to upload:

### A. Install PlatformIO in Windows (if not done yet)

```powershell
pip install -U platformio
```

> Add this to your system PATH if `pio` is not found:
> `C:\Users\YourUser\AppData\Roaming\Python\Python310\Scripts`

### B. Find your COM port

Open **Device Manager → Ports (COM & LPT)** and locate your Arduino.  
e.g. `COM3` for CH340 USB.

---

### C. Upload from Windows terminal

```powershell
cd \\wsl$\Ubuntu\home\marcetux\robot_eyes
pio run --target upload --upload-port COM3
```

> ✅ This compiles in WSL, flashes from Windows. No need to move files!

---

## 🔌 Serial Monitor (Optional)

From **Windows PowerShell**:

```powershell
pio device monitor --port COM3 --baud 9600
```

---

## ⚠️ Arduino IDE vs PlatformIO: Key Differences

| Feature             | Arduino IDE          | PlatformIO (this project)      |
|---------------------|----------------------|--------------------------------|
| Project Structure   | Flat `.ino` file     | Structured `main.cpp`, `lib/`  |
| Library Mgmt        | Manually installed   | Auto-managed with `lib_deps`   |
| Version Control     | Poor (hidden folders)| Git-friendly                   |
| Build Output        | Silent               | Full CLI build logs            |
| Serial Upload (WSL) | ✅                   | ❌ (must use Windows COM)       |

---

## 🧠 Gotchas

- WSL2 can't access `/dev/ttyUSB0` or `/dev/ttyS#` for real flashing.
- Always use **Windows PowerShell** or CMD to flash via `COMx`.
- CH340-based boards show up as `COM3`, `COM4`, etc — check Device Manager.
- Avoid using `.ino` files in PlatformIO. Use `main.cpp` and `#include <Arduino.h>`.

---

## 🧪 Demos

| # | Name           | Description                       |
|---|----------------|-----------------------------------|
| 0 | Splash         | Shows `MARCETUX`                  |
| 1 | Loading Bar    | Fake loading animation            |
| 2 | Fake Clock     | Ticking up with timestamp         |
| 3 | Bouncing Text  | "minux.io" bounces on screen |
| 4 | Robot Eyes     | Blinking eye animation            |

---

## 📄 `platformio.ini`

```ini
[env:nanoatmega328]
platform = atmelavr
board = nanoatmega328
framework = arduino
upload_port = COM3
monitor_speed = 9600

lib_deps =
  adafruit/Adafruit GFX Library @ ^1.12.1
  adafruit/Adafruit SSD1306 @ ^2.5.15
```

---

## 🧾 License

MIT – use freely, share widely.


