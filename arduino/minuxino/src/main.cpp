#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET     -1
#define SCREEN_ADDRESS 0x3C

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

unsigned long lastDemoTime = 0;
const int demoInterval = 5000;
int demoIndex = 0;
const int totalDemos = 5;

unsigned long startMillis = 0;

const int ref_eye_height = 40;
const int ref_eye_width = 40;
const int ref_space_between_eye = 10;
const int ref_corner_radius = 10;
int left_eye_x, left_eye_y, right_eye_x, right_eye_y;
int left_eye_height, right_eye_height;
int left_eye_width, right_eye_width;

void draw_eyes(bool update = true);
void center_eyes(bool update = true);
void blink();
void demo_splash();
void demo_eyes();
void demo_fakeClock();
void demo_bouncingText();
void demo_loadingBar();

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
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(x, SCREEN_HEIGHT / 2);
  display.print("Ohm's Revenge");
  display.display();

  x += dir * 3;
  if (x <= 0 || x >= SCREEN_WIDTH - 60) dir *= -1;
}

void demo_loadingBar() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(10, 5);
  display.println("Awesomeness Loading...");
  int barWidth = (millis() / 10) % (SCREEN_WIDTH - 2);
  display.drawRect(10, 30, SCREEN_WIDTH - 20, 10, SSD1306_WHITE);
  display.fillRect(10, 30, barWidth, 10, SSD1306_WHITE);
  display.display();
}

void setup() {
  display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS);
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(2);
  display.setCursor(15, 25);
  display.println("minux.io");
  display.display();
  delay(2000); // Show splash

  startMillis = millis();
  lastDemoTime = millis();
}

void loop() {
  if (millis() - lastDemoTime >= demoInterval) {
    demoIndex = (demoIndex + 1) % totalDemos;
    lastDemoTime = millis();
  }

  switch (demoIndex) {
    case 0: demo_splash(); break;
    case 1: demo_loadingBar(); break;
    case 2: demo_fakeClock(); break;
    case 3: demo_bouncingText(); break;
    case 4: demo_eyes(); break;
  }
}
