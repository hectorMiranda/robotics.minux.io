#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>


#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET     -1
#define SCREEN_ADDRESS 0x3C
#define BUZZER_PIN 9
#define BUTTON_PIN 2
#define BUTTON2_PIN 10

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

unsigned long lastDemoTime = 0;
int demoIndex = 0;


// Defining frequency of each music note
#define NOTE_C4 262
#define NOTE_D4 294
#define NOTE_E4 330
#define NOTE_F4 349
#define NOTE_G4 392
#define NOTE_A4 440
#define NOTE_B4 494
#define NOTE_C5 523
#define NOTE_D5 587
#define NOTE_E5 659
#define NOTE_F5 698
#define NOTE_G5 784
#define NOTE_A5 880
#define NOTE_B5 988

// Use melodies from the library
const int* piratesNotes = ::piratesNotes;
const int* piratesDurations = ::piratesDurations;
const int totalPiratesNotes = ::totalPiratesNotes;

unsigned long piratesNoteStartTime = 0;
int currentPiratesNote = 0;
bool piratesPlaying = false;

// Button variables
bool lastButtonState = HIGH;
bool currentButtonState = HIGH;
unsigned long lastDebounceTime = 0;
unsigned long debounceDelay = 50;

// Button 2 variables
bool lastButton2State = HIGH;
bool currentButton2State = HIGH;
unsigned long lastDebounce2Time = 0;
bool showUpArrow = false;
unsigned long arrowDisplayTime = 0;
unsigned long arrowDisplayDuration = 1000; // Show arrow for 1 second

 

struct Demo {
  bool enabled;
  unsigned long duration;
  const char* name;
  void (*function)();
};

void demo_splash();
void demo_loadingBar();
void demo_fakeClock();
void demo_bouncingText();
void demo_eyes();
void demo_bitmap();
void demo_odometer();

// Demo configuration - easy to manage in one place!
Demo demos[] = {
  {false,  100,   "Splash",        demo_splash},
  {true,  200,   "Loading Bar",   demo_loadingBar},
  {true,  30000, "Odometer",      demo_odometer},
  {true,  5000,  "Fake Clock",    demo_fakeClock},
  {true,  5000,  "Bouncing Text", demo_bouncingText},
  {true, 5000,  "Eyes",          demo_eyes},
  {true, 500,  "Bitmap",        demo_bitmap}
};

const int totalDemos = sizeof(demos) / sizeof(demos[0]);

int getNextEnabledDemo(int currentIndex) {
  int nextIndex = (currentIndex + 1) % totalDemos;
  
  while (!demos[nextIndex].enabled && nextIndex != currentIndex) {
    nextIndex = (nextIndex + 1) % totalDemos;
  }
  
  return nextIndex;
}

Demo& getCurrentDemo() {
  return demos[demoIndex];
}

unsigned long startMillis = 0;

const int ref_eye_height = 40;
const int ref_eye_width = 40;
const int ref_space_between_eye = 10;
const int ref_corner_radius = 10;
int left_eye_x, left_eye_y, right_eye_x, right_eye_y;
int left_eye_height, right_eye_height;
int left_eye_width, right_eye_width;

const uint8_t myBitmap[] PROGMEM ={
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0xff, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0xf0, 0x00, 0x07, 0xc1, 0xf0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x01, 0x8f, 0xf0, 0x1c, 0x00, 0x1c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x01, 0x80, 0x7f, 0x70, 0x00, 0x06, 0x00, 0x03, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0xc2, 0x07, 0xe0, 0x00, 0x03, 0xff, 0xff, 0xc0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x60, 0x60, 0x00, 0x80, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x30, 0x1c, 0x00, 0x49, 0x00, 0x00, 0x70, 0xc0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x18, 0x06, 0x00, 0x08, 0x00, 0x0f, 0x81, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x08, 0x03, 0x01, 0x00, 0x7c, 0x30, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x0c, 0x01, 0x3f, 0x00, 0x7e, 0x60, 0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x06, 0x01, 0x7f, 0x80, 0xfe, 0x40, 0x0c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x02, 0x01, 0x3f, 0x89, 0xfe, 0x40, 0x0c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x03, 0x00, 0x3f, 0x91, 0xfe, 0x00, 0x18, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x01, 0xc0, 0x1f, 0x81, 0xf8, 0x00, 0x70, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x70, 0x00, 0x00, 0x00, 0x81, 0xc0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x3c, 0x00, 0x00, 0x01, 0x9f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x03, 0xc0, 0x00, 0x3f, 0xf0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x01, 0x3f, 0xff, 0xf0, 0x78, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x01, 0x1e, 0xe0, 0x00, 0xf8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x01, 0x80, 0x00, 0x01, 0x98, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0xc0, 0x00, 0x07, 0x8c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0xe0, 0x00, 0x1d, 0x0c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x01, 0x98, 0x00, 0xcd, 0x3c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x03, 0x8f, 0x81, 0x99, 0xf0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x03, 0x4b, 0x9f, 0x19, 0x30, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x03, 0x72, 0x68, 0x2c, 0x60, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x03, 0x92, 0x48, 0x4c, 0xc0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x01, 0xd0, 0x48, 0xdf, 0xc0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x03, 0x7c, 0x09, 0x9e, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x01, 0xf8, 0x0b, 0x1c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x60, 0x88, 0x0c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x20, 0x88, 0x0c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x20, 0x88, 0x0e, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x70, 0x88, 0x0e, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x60, 0x88, 0x0e, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x60, 0x88, 0x0e, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x78, 0xc8, 0x1e, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x3f, 0xff, 0xfc, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};


void draw_eyes(bool update = true);
void center_eyes(bool update = true);
void blink();
void demo_splash();
void demo_eyes();
void demo_fakeClock();
void demo_bouncingText();
void demo_loadingBar();
void demo_bitmap();
void demo_odometer();
void playStartupMelody();
void playPiratesTheme();
void playPiratesNoteNonBlocking(int noteIndex);
void updatePiratesThemeNonBlocking();
void drawUpArrow();

void draw_eyes(bool update) {
  display.clearDisplay();
  display.fillRoundRect(left_eye_x - left_eye_width / 2, left_eye_y - left_eye_height / 2,
                        left_eye_width, left_eye_height, ref_corner_radius, SSD1306_WHITE);
  display.fillRoundRect(right_eye_x - right_eye_width / 2, right_eye_y - right_eye_height / 2,
                        right_eye_width, right_eye_height, ref_corner_radius, SSD1306_WHITE);
  if (update) display.display();
}

void center_eyes(bool update) {
  left_eye_height = right_eye_height = ref_eye_height;
  left_eye_width = right_eye_width = ref_eye_width;
  left_eye_x = SCREEN_WIDTH / 2 - ref_eye_width / 2 - ref_space_between_eye / 2;
  right_eye_x = SCREEN_WIDTH / 2 + ref_eye_width / 2 + ref_space_between_eye / 2;
  left_eye_y = right_eye_y = SCREEN_HEIGHT / 2;
  if (update) draw_eyes();
}

void blink() {
  for (int i = 0; i < 3; i++) {
    left_eye_height -= 6;
    right_eye_height -= 6;
    draw_eyes();
    delay(40);
  }
  for (int i = 0; i < 3; i++) {
    left_eye_height += 6;
    right_eye_height += 6;
    draw_eyes();
    delay(40);
  }
}

void demo_splash() {
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(15, 25);
  display.println("minux.io");
  display.display();
}

void demo_eyes() {
  center_eyes();
  draw_eyes();
  blink();
}

void demo_fakeClock() {
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(15, 10);

  unsigned long elapsed = (millis() - startMillis) / 1000;
  int h = (elapsed / 3600) % 24;
  int m = (elapsed / 60) % 60;
  int s = elapsed % 60;

  char buffer[9];
  sprintf(buffer, "%02d:%02d:%02d", h, m, s);
  display.println(buffer);

  display.setTextSize(1);
  display.setCursor(20, 42);
  display.println("Los Angeles, CA");
  display.display();
}

void demo_bouncingText() {
  static int x = 0;
  static int dir = 1;
  static unsigned long lastMove = 0;
  static bool initialized = false;
  static unsigned long lastDemoStart = 0;
  
  // Check if this is a new demo cycle (demo just started)
  if (lastDemoStart != lastDemoTime) {
    initialized = false;
    lastDemoStart = lastDemoTime;
  }
  
  // Initialize once per demo cycle
  if (!initialized) {
    x = 0;
    dir = 1;
    lastMove = millis();
    initialized = true;
  }
  
  // Move text every 50ms for smooth animation
  if (millis() - lastMove >= 50) {
    x += dir * 2;
    
    // Bounce off edges - adjust for "Ohm's Revenge" text width
    if (x <= 0) {
      dir = 1;
    }
    if (x >= SCREEN_WIDTH - 120) { // Width for "Ohm's Revenge" text
      dir = -1;
    }
    
    lastMove = millis();
  }
  
  // Display the bouncing text
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(x, SCREEN_HEIGHT / 2 - 8);
  display.print("Ohm's Revenge");
  display.display();
}

void demo_loadingBar() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(10, 5);
  display.println("Loading...");
  int barWidth = (millis() / 10) % (SCREEN_WIDTH - 20);
  display.drawRect(10, 30, SCREEN_WIDTH - 20, 10, SSD1306_WHITE);
  display.fillRect(10, 30, barWidth, 10, SSD1306_WHITE);
  display.display();
}

void demo_bitmap() {
  display.clearDisplay();
  display.drawBitmap(0, 0, myBitmap, 128, 64, SSD1306_WHITE);
  display.display();
}

void playStartupMelody() {
  // Define notes (frequencies in Hz)
  #define NOTE_C4  262
  #define NOTE_D4  294
  #define NOTE_E4  330
  #define NOTE_F4  349
  #define NOTE_G4  392
  #define NOTE_A4  440
  #define NOTE_B4  494
  #define NOTE_C5  523
  
  // Startup melody: "Robot Boot Sequence"
  int melody[] = {
    NOTE_C4, NOTE_E4, NOTE_G4, NOTE_C5,  // Rising arpeggio
    NOTE_G4, NOTE_E4, NOTE_C4,           // Quick descent
    NOTE_F4, NOTE_A4, NOTE_C5,           // Power up sound
    NOTE_C5, NOTE_C5                     // Ready beeps
  };
  
  int noteDurations[] = {
    150, 150, 150, 300,   // Arpeggio timing
    100, 100, 200,        // Quick descent
    150, 150, 400,        // Power up
    200, 200              // Ready beeps
  };
  
  // Play the melody
  for (int i = 0; i < 12; i++) {
    tone(BUZZER_PIN, melody[i], noteDurations[i]);
    delay(noteDurations[i] + 50); // Small pause between notes
  }
  
  noTone(BUZZER_PIN); // Stop any lingering tone
}

void playPiratesTheme() {
  // Define all note frequencies
  #define NOTE_C4 262
  #define NOTE_D4 294
  #define NOTE_E4 330
  #define NOTE_F4 349
  #define NOTE_G4 392
  #define NOTE_A4 440
  #define NOTE_B4 494
  #define NOTE_C5 523
  #define NOTE_D5 587
  #define NOTE_E5 659
  #define NOTE_F5 698
  #define NOTE_G5 784
  #define NOTE_A5 880
  #define NOTE_B5 988

  // Pirates of the Caribbean theme notes
  int notes[] = {
    NOTE_E4, NOTE_G4, NOTE_A4, NOTE_A4, 0,
    NOTE_A4, NOTE_B4, NOTE_C5, NOTE_C5, 0,
    NOTE_C5, NOTE_D5, NOTE_B4, NOTE_B4, 0,
    NOTE_A4, NOTE_G4, NOTE_A4, 0,

    NOTE_E4, NOTE_G4, NOTE_A4, NOTE_A4, 0,
    NOTE_A4, NOTE_B4, NOTE_C5, NOTE_C5, 0,
    NOTE_C5, NOTE_D5, NOTE_B4, NOTE_B4, 0,
    NOTE_A4, NOTE_G4, NOTE_A4, 0,

    NOTE_E4, NOTE_G4, NOTE_A4, NOTE_A4, 0,
    NOTE_A4, NOTE_C5, NOTE_D5, NOTE_D5, 0,
    NOTE_D5, NOTE_E5, NOTE_F5, NOTE_F5, 0,
    NOTE_E5, NOTE_D5, NOTE_E5, NOTE_A4, 0,

    NOTE_A4, NOTE_B4, NOTE_C5, NOTE_C5, 0,
    NOTE_D5, NOTE_E5, NOTE_A4, 0,
    NOTE_A4, NOTE_C5, NOTE_B4, NOTE_B4, 0,
    NOTE_C5, NOTE_A4, NOTE_B4, 0
  };

  // Note durations
  int durations[] = {
    125, 125, 250, 125, 125,
    125, 125, 250, 125, 125,
    125, 125, 250, 125, 125,
    125, 125, 375, 125,

    125, 125, 250, 125, 125,
    125, 125, 250, 125, 125,
    125, 125, 250, 125, 125,
    125, 125, 375, 125,

    125, 125, 250, 125, 125,
    125, 125, 250, 125, 125,
    125, 125, 250, 125, 125,
    125, 125, 125, 250, 125,

    125, 125, 250, 125, 125,
    250, 125, 250, 125,
    125, 125, 250, 125, 125,
    125, 125, 375, 375
  };

  const int totalNotes = sizeof(notes) / sizeof(int);
  
  // Play the melody
  for (int i = 0; i < totalNotes; i++) {
    if (notes[i] != 0) {
      tone(BUZZER_PIN, notes[i], durations[i]);
    } else {
      noTone(BUZZER_PIN);
    }
    delay(durations[i]);
  }
  
  noTone(BUZZER_PIN); // Stop any lingering tone
}

void playPiratesNoteNonBlocking(int noteIndex) {
  if (noteIndex < totalPiratesNotes) {
    piratesNoteStartTime = millis();
    currentPiratesNote = noteIndex;
    piratesPlaying = true;
    
    if (piratesNotes[noteIndex] != 0) {
      // Use tone with duration to prevent blocking
      tone(BUZZER_PIN, piratesNotes[noteIndex], piratesDurations[noteIndex]);
    }
    // Note: for silent notes (0), we don't call tone at all
  }
}

void updatePiratesThemeNonBlocking() {
  if (!piratesPlaying || currentPiratesNote >= totalPiratesNotes) {
    // Music not playing or finished
    if (piratesPlaying) {
      // Song finished, clean up
      noTone(BUZZER_PIN);
      piratesPlaying = false;
      currentPiratesNote = 0;
    }
    return;
  }
  
  // Check if current note duration has elapsed
  if (millis() - piratesNoteStartTime >= piratesDurations[currentPiratesNote]) {
    currentPiratesNote++;
    
    if (currentPiratesNote < totalPiratesNotes) {
      // Play next note
      playPiratesNoteNonBlocking(currentPiratesNote);
    } else {
      // Song finished
      noTone(BUZZER_PIN);
      piratesPlaying = false;
      currentPiratesNote = 0;
    }
  }
}

void drawUpArrow() {
  // Clear display and draw a large up arrow
  display.clearDisplay();
  
  // Draw arrow shaft (vertical line)
  display.fillRect(60, 30, 8, 20, SSD1306_WHITE);
  
  // Draw arrow head (triangle pointing up)
  // Top point
  display.drawLine(64, 15, 64, 15, SSD1306_WHITE);
  // Arrow head lines
  for(int i = 0; i < 15; i++) {
    display.drawLine(64-i, 15+i, 64+i, 15+i, SSD1306_WHITE);
  }
  
  // Add some text below
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(45, 55);
  display.println("UP ARROW");
  
  display.display();
}

void demo_odometer() {
  static unsigned long odometerStartTime = 0;
  static float distance = 0.0;
  static int batteryLevel = 100;
  static int currentState = 0;
  static float speed = 0.0;
  static bool turboMode = false;
  
  if (odometerStartTime == 0) {
    odometerStartTime = millis();
  }
  
  unsigned long elapsed = millis() - odometerStartTime;
  
  // Cycle through different states every 4 seconds for better readability
  currentState = (elapsed / 4000) % 5;
  
  // Simulate turbo mode activation (every 15 seconds for 3 seconds)
  turboMode = ((elapsed / 1000) % 15) < 3;
  
  display.clearDisplay();
  
  // ===== CRITICAL FULL-SCREEN ALERTS =====
  
  // BATTERY LOW ALERT (when battery < 20%)
  if (batteryLevel < 1) {
    // Animated border effect
    int borderOffset = (elapsed / 150) % 4;
    display.drawRect(borderOffset, borderOffset, 128-2*borderOffset, 64-2*borderOffset, SSD1306_WHITE);
    display.drawRect(borderOffset+1, borderOffset+1, 126-2*borderOffset, 62-2*borderOffset, SSD1306_WHITE);
    
    display.setTextSize(2);
    display.setCursor(5, 8);
    if ((elapsed / 300) % 2) { // Fast blinking
      display.println("BATTERY");
    }
    display.setTextSize(2);
    display.setCursor(20, 35);
    if ((elapsed / 400) % 2) {
      display.println("LOW!");
    }
    
    // Warning stripes at bottom
    for(int i = 0; i < 128; i += 8) {
      if ((i + elapsed/100) % 16 < 8) {
        display.fillRect(i, 58, 4, 6, SSD1306_WHITE);
      }
    }
    
    display.setTextSize(1);
    display.setCursor(48, 50);
    display.print(batteryLevel);
    display.print("%");
    display.display();
    return;
  }
  
  // LINE NOT DETECTED (during searching state)
  if (currentState == 2) {
    // Scanning radar effect
    int sweepAngle = (elapsed / 50) % 360;
    for(int r = 10; r < 60; r += 10) {
      display.drawCircle(64, 32, r, SSD1306_WHITE);
    }
    
    // Radar sweep line
    float radians = sweepAngle * 3.14159 / 180;
    int endX = 64 + 50 * cos(radians);
    int endY = 32 + 50 * sin(radians);
    display.drawLine(64, 32, endX, endY, SSD1306_WHITE);
    
    display.setTextSize(2);
    display.setCursor(30, 5);
    if ((elapsed / 250) % 2) { // Fast blinking
      display.println("LINE");
    }
    display.setCursor(15, 50);
    if ((elapsed / 350) % 2) {
      display.println("NOT FOUND");
    }
    
    // Scanning dots
    display.setTextSize(3);
    int dots = (elapsed / 300) % 4;
    display.setCursor(45 + dots * 10, 25);
    display.print(".");
    
    display.display();
    return;
  }
  
  // TURBO MODE ALERT
  if (turboMode) {
    // Speed lines effect
    for(int i = 0; i < 20; i++) {
      int lineX = (elapsed/20 + i*15) % 140 - 10;
      int lineY = 10 + i * 2;
      if(lineX > 0 && lineX < 128) {
        display.drawLine(lineX, lineY, lineX + 10, lineY, SSD1306_WHITE);
      }
    }
    
    // Double border for emphasis
    display.drawRect(0, 0, 128, 64, SSD1306_WHITE);
    display.drawRect(2, 2, 124, 60, SSD1306_WHITE);
    
    display.setTextSize(3);
    display.setCursor(15, 15);
    if ((elapsed / 150) % 2) { // Fast blinking
      display.println("TURBO");
    }
    
    // Lightning bolt effect
    if ((elapsed / 100) % 3 == 0) {
      display.drawLine(10, 45, 15, 50, SSD1306_WHITE);
      display.drawLine(15, 50, 10, 55, SSD1306_WHITE);
      display.drawLine(10, 55, 15, 60, SSD1306_WHITE);
      
      display.drawLine(118, 45, 113, 50, SSD1306_WHITE);
      display.drawLine(113, 50, 118, 55, SSD1306_WHITE);
      display.drawLine(118, 55, 113, 60, SSD1306_WHITE);
    }
    
    display.setTextSize(1);
    display.setCursor(20, 45);
    display.println("MAX SPEED MODE");
    display.setCursor(35, 55);
    display.println("ENGAGED!");
    display.display();
    return;
  }
  
  // ===== CLEAN DASHBOARD DESIGN =====
  
  // === TOP ROW: Distance and Battery (well spaced) ===
  display.setTextSize(1);
  
  // Distance - left side with box
  distance += 0.02; // Slower increment for readability
  display.drawRect(1, 4, 62, 30, SSD1306_WHITE);
  display.setCursor(3, 6);
  display.print("DISTANCE");
  display.setCursor(3, 16);
  display.setTextSize(2);
  display.print(distance, 1);
  display.setTextSize(1);
  display.setCursor(3, 26);
  display.print("meters");
  
  // Battery - right side with box and proper spacing
  if (elapsed % 200 == 0 && batteryLevel > 5) { // Faster battery drain for demo
    batteryLevel--;
  }
  
  // Demo sequence: show different critical alerts at specific times
  if (elapsed > 6000 && elapsed < 10000) {
    batteryLevel = 15; // Force low battery alert demo
  } else if (elapsed >= 14000 && elapsed < 18000) {
    batteryLevel = 8; // Critical battery alert demo
  }
  
  display.drawRect(65, 4, 62, 30, SSD1306_WHITE);
  display.setCursor(67, 6);
  display.print("BATTERY");
  display.setCursor(67, 16);
  display.setTextSize(1); // Keep text size small
  display.print(batteryLevel);
  display.print("%");
  
  // Clean battery bar visualization - positioned below text
  display.drawRect(67, 24, 48, 8, SSD1306_WHITE);
  int batBarWidth = map(batteryLevel, 0, 100, 0, 46);
  if (batBarWidth > 0) {
    display.fillRect(68, 25, batBarWidth, 6, SSD1306_WHITE);
  }
  
  // === MIDDLE SECTION: Speed and Status with boxes ===
  // Speed box
  display.drawRect(1, 37, 62, 26, SSD1306_WHITE);
  display.setCursor(3, 39);
  display.print("SPEED:");
  display.setCursor(3, 49);
  
  // Status box
  display.drawRect(65, 37, 62, 26, SSD1306_WHITE);
  
  // State-dependent information with better spacing
  switch(currentState) {
    case 0: // Following path
      speed = turboMode ? 20.0 : 12.0; // Faster in turbo mode
      display.print(speed, 0);
      display.print(" cm/s");
      
      display.setCursor(67, 39);
      display.println("STATUS:");
      display.setCursor(67, 49);
      if (turboMode) {
        display.println("TURBO!");
      } else {
        display.println("FOLLOWING");
      }
      
      // Clean path indicator - bottom area
      for(int i = (elapsed/150) % 8; i < 128; i += 8) {
        display.fillRect(i, 63, 4, 1, SSD1306_WHITE);
      }
      break;
      
    case 1: // Obstacle detected
      speed = 0.0;
      display.print(speed, 0);
      display.print(" cm/s");
      
      display.setCursor(67, 39);
      display.println("STATUS:");
      display.setCursor(67, 49);
      if ((elapsed / 300) % 2) { // Slower blink for readability
        display.println("OBSTACLE!");
      } else {
        display.println("STOP");
      }
      
      // Warning indicators - cleaner
      if ((elapsed / 500) % 2) {
        display.fillRect(0, 35, 2, 30, SSD1306_WHITE);
        display.fillRect(126, 35, 2, 30, SSD1306_WHITE);
      }
      break;
      
    case 2: // Path lost - searching (this now only shows if not critical)
      speed = 2.0;
      display.print(speed, 0);
      display.print(" cm/s");
      
      display.setCursor(67, 39);
      display.println("STATUS:");
      display.setCursor(67, 49);
      display.println("SEARCHING");
      
      // Clean scanning animation
      {
        int scanPos = (elapsed / 80) % 128;
        display.drawLine(scanPos - 5, 63, scanPos + 5, 63, SSD1306_WHITE);
      }
      break;
      
    case 3: // Navigation/Turn
      speed = turboMode ? 25.0 : 6.0; // Faster in turbo mode
      display.print(speed, 0);
      display.print(" cm/s");
      
      display.setCursor(67, 39);
      display.println("STATUS:");
      display.setCursor(67, 49);
      display.println("TURNING");
      
      // Clean turn arrow
      display.drawLine(115, 55, 120, 52, SSD1306_WHITE);
      display.drawLine(115, 55, 120, 58, SSD1306_WHITE);
      display.drawLine(115, 55, 125, 55, SSD1306_WHITE);
      break;
      
    case 4: // Mission complete
      speed = 0.0;
      display.print(speed, 0);
      display.print(" cm/s");
      
      display.setCursor(67, 39);
      display.println("STATUS:");
      display.setCursor(67, 49);
      display.println("COMPLETE");
      
      // Clean success indicator
      if ((elapsed / 600) % 2) {
        display.drawLine(115, 55, 118, 58, SSD1306_WHITE);
        display.drawLine(118, 58, 125, 51, SSD1306_WHITE);
      }
      break;
  }
  
  // Clean status indicator (top right)
  if (currentState == 0) { // Normal operation
    display.fillCircle(122, 3, 3, SSD1306_WHITE);
  } else if (currentState == 1) { // Alert
    if ((elapsed / 200) % 2) {
      display.fillCircle(122, 3, 3, SSD1306_WHITE);
    }
  } else { // Other states
    if ((elapsed / 800) % 2) {
      display.fillCircle(122, 3, 3, SSD1306_WHITE);
    }
  }
  
  // Reset cycle
  if (elapsed >= 20000) { // 20 seconds total cycle
    odometerStartTime = millis();
    distance = 0.0;
    batteryLevel = 100;
  }
  
  display.display();
}

void setup() {
  // Initialize buzzer pin
  pinMode(BUZZER_PIN, OUTPUT);
  
  // Initialize button pins with internal pullup
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(BUTTON2_PIN, INPUT_PULLUP);
  
  // Removed startup melody to prevent button interference
  // playStartupMelody();
  
  display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS);
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(2);
  display.setCursor(15, 25);
  display.println("minux.io");
  display.display();
  delay(500); // Show splash

  startMillis = millis();
  lastDemoTime = millis();
  
  // Start with the first enabled demo AFTER splash (index 1 = Loading Bar)
  demoIndex = 1; // Skip splash and start with Loading Bar regardless of splash enabled/disabled
  // Ensure the demo we're starting with is actually enabled
  if (!demos[demoIndex].enabled) {
    demoIndex = getNextEnabledDemo(demoIndex);
  }
}

void handleButton() {
  // Read the button state
  int reading = digitalRead(BUTTON_PIN);
  
  // Check if button state changed (for debouncing)
  if (reading != lastButtonState) {
    lastDebounceTime = millis();
  }
  
  // If enough time has passed since last state change, consider it a stable reading
  if ((millis() - lastDebounceTime) > debounceDelay) {
    // If button state has changed from HIGH to LOW (button pressed)
    if (currentButtonState == HIGH && reading == LOW) {
      // Play a quick non-blocking beep - no delays that freeze the system
      tone(BUZZER_PIN, NOTE_A4, 100); // Just a 100ms beep, non-blocking
    }
    currentButtonState = reading;
  }
  
  lastButtonState = reading;
}

void handleButton2() {
  // Read the second button state
  int reading = digitalRead(BUTTON2_PIN);
  
  // Check if button state changed (for debouncing)
  if (reading != lastButton2State) {
    lastDebounce2Time = millis();
  }
  
  // If enough time has passed since last state change, consider it a stable reading
  if ((millis() - lastDebounce2Time) > debounceDelay) {
    // If button state has changed from HIGH to LOW (button pressed)
    if (currentButton2State == HIGH && reading == LOW) {
      // Show up arrow for specified duration
      showUpArrow = true;
      arrowDisplayTime = millis();
      drawUpArrow();
    }
    currentButton2State = reading;
  }
  
  lastButton2State = reading;
}

void loop() {
  Demo& currentDemo = getCurrentDemo();
  
  // Handle button presses first
  handleButton();
  handleButton2();
  
  // Check if we should stop showing the up arrow
  if (showUpArrow && (millis() - arrowDisplayTime >= arrowDisplayDuration)) {
    showUpArrow = false;
    // Force demo to redraw by resetting the demo timer
    lastDemoTime = millis() - currentDemo.duration + 100; // Give 100ms to show current demo
  }
  
  // If showing up arrow, don't run demos
  if (showUpArrow) {
    return;
  }
  
  // Removed automatic Pirates music updates - music now only plays on button press
  // if (piratesPlaying) {
  //   updatePiratesThemeNonBlocking();
  // }
  
  // Ensure demos can run regardless of music state
  if (!currentDemo.enabled) {
    demoIndex = getNextEnabledDemo(demoIndex);
    lastDemoTime = millis();
    return;
  }
  
  // Check if it's time to switch demos
  if (millis() - lastDemoTime >= currentDemo.duration) {
    // No need to stop music when switching demos since we're not playing continuous music
    // if (piratesPlaying) {
    //   noTone(BUZZER_PIN);
    //   piratesPlaying = false;
    //   currentPiratesNote = 0;
    // }
    
    demoIndex = getNextEnabledDemo(demoIndex);
    lastDemoTime = millis();
  }

  // Run the current demo
  currentDemo.function();
}
