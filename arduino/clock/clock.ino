/*
Project: clock
Theme: Pac-man
by Hector Miranda

This program is a combination from part A through D. It uses S3 on the LDB as the Alarm Set button. 
To set the alarm we must press and hold the alarm button and then press either the minutes or hours buttons 
to set the alarm time hours and alarm time minutes.  

When the actual hours and minutes are equal to the alarmHours and alarmMinutes an 
alarm should go off. The program uses the buzzer to create a pacman-like tone when the alarm goes off.

*/

#include <SPI.h>
#include <Wire.h>

// Notes for Pacman
#define NOTE_B4  494
#define NOTE_B5  988
#define NOTE_FS5 740
#define NOTE_DS5 622
#define NOTE_C5  523
#define NOTE_C6  1047
#define NOTE_G6  1568
#define NOTE_E6  1319
#define NOTE_E5  659
#define NOTE_F5  698
#define NOTE_G5  784
#define NOTE_GS5 831
#define NOTE_A5  880
#define REST      0

const int minButtonPin = 3;
const int hourButtonPin = 4;
const int alarmSetButtonPin = 5;
const int pmLEDPin = 6;
const int speakerPin = 7;
const int interruptPin = 2;

volatile int timeKeeper = 0;
int seconds = 55, minutes = 59, hours = 1;
int alarmMinutes = 1, alarmHours = 2;
bool alarmSetMode = false, isPM = false, setMode = false, alarmHasFired = false;
byte minHighNibble, minLowNibble;
int lastMinute = -1;

int tempo = 105;
int melody[] = {
  NOTE_B4, 16, NOTE_B5, 16, NOTE_FS5, 16, NOTE_DS5, 16,
  NOTE_B5, 32, NOTE_FS5, -16, NOTE_DS5, 8, NOTE_C5, 16,
  NOTE_C6, 16, NOTE_G6, 16, NOTE_E6, 16, NOTE_C6, 32, NOTE_G6, -16, NOTE_E6, 8,
  NOTE_B4, 16, NOTE_B5, 16, NOTE_FS5, 16, NOTE_DS5, 16, NOTE_B5, 32,
  NOTE_FS5, -16, NOTE_DS5, 8, NOTE_E5, 32, NOTE_F5, 32,
  NOTE_F5, 32, NOTE_FS5, 32, NOTE_G5, 32, NOTE_G5, 32, NOTE_GS5, 32, NOTE_A5, 16, NOTE_B5, 8
};
int notes = sizeof(melody) / sizeof(melody[0]) / 2;
int wholenote = (60000 * 4) / tempo;
int divider = 0, noteDuration = 0;

void setup() {
  Serial.begin(9600);
  Serial.println("Starting...");

  pinMode(hourButtonPin, INPUT_PULLUP);
  pinMode(minButtonPin, INPUT_PULLUP);
  pinMode(alarmSetButtonPin, INPUT_PULLUP);
  pinMode(pmLEDPin, OUTPUT);
  pinMode(speakerPin, OUTPUT);
  pinMode(interruptPin, INPUT);

  DDRB = B11111111;
  DDRC = B11111111;
  PORTC = hours % 12;

  attachInterrupt(digitalPinToInterrupt(interruptPin), Sixty_Hz, RISING);
}

void loop() {
  checkAlarmSetButton();

  if (!alarmSetMode) {
    checkButtons();
  }

  if (!alarmSetMode && !setMode) {
    updatePMIndicator();  // Only update when not in set modes
  }


  if (!setMode && !alarmSetMode && timeKeeper >= 60) {
    timeKeeper -= 60;
    seconds++;

    if (seconds > 59) {
      seconds = 0;
      minutes++;
      playTickMinute();
      if (minutes > 59) {
        minutes = 0;
        hours++;
        playTickHour();
        if (hours > 23) hours = 0;
        PORTC = hours % 12;
      }
    }

    printTimeToSerial();
    displayMinutes();
    if (minutes != lastMinute) lastMinute = minutes;
  }

  if (!alarmHasFired && !setMode && !alarmSetMode && hours == alarmHours && minutes == alarmMinutes) {
    Serial.println("!!! Alarm Triggered — Time MATCHED Alarm Time (Loop Post-Time-Increment) !!!");
    playAlarm();
    alarmHasFired = true;
  }


  if (hours != alarmHours || minutes != alarmMinutes) {
    alarmHasFired = false;
  }

  if (setMode || alarmSetMode) {
    printTimeToSerial();
    displayMinutes();
    delay(250);
    setMode = false;
    alarmSetMode = false;
  }
}

void Sixty_Hz() {
  timeKeeper++;
}

void displayMinutes() {
  minHighNibble = minutes / 10;
  minLowNibble = minutes % 10;

  PORTB = (PORTB & 0b11100000) | minLowNibble;
  PORTB |= 0b00010000;
  delayMicroseconds(5);
  PORTB &= 0b11101111;

  PORTB = (PORTB & 0b11100000) | minHighNibble;
  PORTB |= 0b00100000;
  delayMicroseconds(5);
  PORTB &= 0b11011111;
}

void checkButtons() {
  if (digitalRead(hourButtonPin) == LOW) {
    delay(50);
    if (digitalRead(hourButtonPin) == LOW) {
      hours = (hours + 1) % 24;
      PORTC = hours % 12;
      setMode = true;
      Serial.print("Hour manually set to: ");
      Serial.println(hours);
      playTickHour();
    }
  }

  if (digitalRead(minButtonPin) == LOW) {
    delay(50);
    if (digitalRead(minButtonPin) == LOW) {
      minutes = (minutes + 1) % 60;
      setMode = true;
      Serial.print("Minute manually set to: ");
      Serial.println(minutes);
      playTickMinute();
    }
  }
}

void updatePMIndicatorFromHour(int hour) {
  digitalWrite(pmLEDPin, hour >= 12 ? HIGH : LOW);
}


void checkAlarmSetButton() {
  static bool s3PreviouslyPressed = false;
  static unsigned long lastPress = 0;
  static bool shouldDisplayAlarmTime = false;
  static bool soundPlayed = false;

  bool s3Pressed = digitalRead(alarmSetButtonPin) == LOW;
  bool minPressed = digitalRead(minButtonPin) == LOW;
  bool hourPressed = digitalRead(hourButtonPin) == LOW;

  if (s3Pressed) {
    if (!s3PreviouslyPressed) {
      Serial.println("=== ALARM SET MODE ACTIVE ===");
      s3PreviouslyPressed = true;
      playTickAlarm();
    }

    alarmSetMode = true;

    if (hourPressed && millis() - lastPress > 250) {
      alarmHours = (alarmHours + 1) % 24;
      Serial.print("Alarm Hour set to: ");
      Serial.println(alarmHours);
      playTickHour();
      lastPress = millis();
    }

    if (minPressed && millis() - lastPress > 250) {
      alarmMinutes = (alarmMinutes + 1) % 60;
      Serial.print("Alarm Minute set to: ");
      Serial.println(alarmMinutes);
      displayAlarmMinutes(); 
      playTickMinute();
      lastPress = millis();
    }

    displayAlarmMinutes();
    updatePMIndicatorFromHour(alarmHours);// Keep LED in sync with alarm time

  } else {
    if (s3PreviouslyPressed) {
      shouldDisplayAlarmTime = true;
    }
    s3PreviouslyPressed = false;
    alarmSetMode = false;
  }

  if (shouldDisplayAlarmTime) {
    displayAlarmMinutes();
    updatePMIndicatorFromHour(alarmHours);  //Show correct PM state while previewing
    delay(1000);
    shouldDisplayAlarmTime = false;
    displayMinutes();                       //Restore real time
    updatePMIndicatorFromHour(hours);       //Restore PM LED to match current time
  }

  if (!s3Pressed && !shouldDisplayAlarmTime) {
    updatePMIndicatorFromHour(hours);       //Normal fallback
  }
}



void displayAlarmMinutes() {
  byte alarmMinHighNibble = alarmMinutes / 10;
  byte alarmMinLowNibble = alarmMinutes % 10;

  PORTB = (PORTB & 0b11100000) | alarmMinLowNibble;
  PORTB |= 0b00010000; delayMicroseconds(5); PORTB &= 0b11101111;

  PORTB = (PORTB & 0b11100000) | alarmMinHighNibble;
  PORTB |= 0b00100000; delayMicroseconds(5); PORTB &= 0b11011111;

  PORTC = (PORTC & 0b11110000) | (alarmHours % 12);
  
}

void updatePMIndicator() {
  if (hours >= 12) {
    digitalWrite(pmLEDPin, HIGH);
    isPM = true;
  } else {
    digitalWrite(pmLEDPin, LOW);
    isPM = false;
  }
}

void printTimeToSerial() {
  int displayHour = hours % 12;
  if (displayHour == 0) displayHour = 12;

  Serial.print(displayHour);
  Serial.print(" : ");
  if (minutes < 10) Serial.print("0");
  Serial.print(minutes);
  Serial.print(" : ");
  if (seconds < 10) Serial.print("0");
  Serial.print(seconds);
  Serial.print(" ");
  Serial.println(isPM ? "PM" : "AM");
}

void playAlarm() {
  Serial.println("=== ALARM SOUNDING START ===");
  for (int i = 0; i < 3; i++) {
    playPacman();
    displayMinutes();
  }

  PORTC = hours % 12;  // ✅ Restore correct hour LEDs
  Serial.println("=== ALARM SOUNDING END ===");
}

void playPacman() {
  for (int thisNote = 0; thisNote < notes * 2; thisNote += 2) {
    divider = melody[thisNote + 1];
    if (divider > 0) {
      noteDuration = wholenote / divider;
    } else {
      noteDuration = wholenote / abs(divider);
      noteDuration *= 1.5;
    }

    byte ledC = (1 << ((thisNote / 2) % 4));
    byte ledB = (1 << ((thisNote / 2) % 4));
    PORTC = ledC;
    PORTB = (PORTB & 0b11110000) | ledB;

    byte randLow = random(0, 10);
    byte randHigh = random(0, 6);

    PORTB = (PORTB & 0b11100000) | randLow;
    PORTB |= 0b00010000; delayMicroseconds(5); PORTB &= 0b11101111;

    PORTB = (PORTB & 0b11100000) | randHigh;
    PORTB |= 0b00100000; delayMicroseconds(5); PORTB &= 0b11011111;

    tone(speakerPin, melody[thisNote], noteDuration * 0.9);
    delay(noteDuration);
    noTone(speakerPin);

    PORTB &= 0b11110000;
    PORTC = 0;
  }
}

void playTickMinute() {
  tone(speakerPin, 880, 100);
  delay(120);
  tone(speakerPin, 660, 100);
  delay(120);
  noTone(speakerPin);
}

void playTickHour() {
  tone(speakerPin, 988, 80);
  delay(100);
  tone(speakerPin, 622, 80);
  delay(100);
  tone(speakerPin, 784, 80);
  delay(120);
  noTone(speakerPin);
}

void playTickAlarm() {
  tone(speakerPin, 1047, 60);
  delay(80);
  tone(speakerPin, 1319, 60);
  delay(80);
  tone(speakerPin, 1568, 60);
  delay(100);
  noTone(speakerPin);
}
