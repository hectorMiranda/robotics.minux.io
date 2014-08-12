#!/usr/bin/env python
import sys
import os
import curses

galileo_path = "/media/mmcblk0p1/";
if galileo_path not in sys.path:
    sys.path.append(galileo_path);

from pyGalileo import *

class EarthRover:
 """EarthRover class"""
 #PWMArduinoDigitalPinNumber_PWMChannelNumber
 PWM4_11 = 11
 PWM7_10 = 10
 PWM1_9 = 9
 PWM6_6 = 6

 def MoveForward(self):
  self.LastCmdOutput = 'Forward '
  digitalWrite(EarthRover.PWM4_11, HIGH);
  digitalWrite(EarthRover.PWM7_10, LOW);
  digitalWrite(EarthRover.PWM1_9, LOW);
  digitalWrite(EarthRover.PWM6_6, LOW);

 def MoveBackward(self):
  self.LastCmdOutput = 'Backward'
  digitalWrite(EarthRover.PWM4_11, LOW);
  digitalWrite(EarthRover.PWM7_10, HIGH);
  digitalWrite(EarthRover.PWM1_9, LOW);
  digitalWrite(EarthRover.PWM6_6, LOW);

 def MoveLeft(self):
  self.LastCmdOutput = 'Left    '
  digitalWrite(EarthRover.PWM4_11, LOW);
  digitalWrite(EarthRover.PWM7_10, LOW);
  digitalWrite(EarthRover.PWM1_9, HIGH);
  digitalWrite(EarthRover.PWM6_6, LOW);

 def MoveRight(self):
  self.LastCmdOutput = 'Right   '
  digitalWrite(EarthRover.PWM4_11, LOW);
  digitalWrite(EarthRover.PWM7_10, LOW);
  digitalWrite(EarthRover.PWM1_9, LOW);
  digitalWrite(EarthRover.PWM6_6, HIGH);

 def __init__(self, name):
  self.name = name
  self.LastCmdOutput = '[Ready] '
  pinMode(EarthRover.PWM4_11, OUTPUT);     
  pinMode(EarthRover.PWM7_10, OUTPUT);     
  pinMode(EarthRover.PWM1_9, OUTPUT);     
  pinMode(EarthRover.PWM6_6, OUTPUT);     
  analogWrite(EarthRover.PWM4_11, 255);
  analogWrite(EarthRover.PWM7_10, 255);
  analogWrite(EarthRover.PWM1_9, 255);
  analogWrite(EarthRover.PWM6_6, 255);




def initUI():
 screen.border(0)
 curses.noecho()
 curses.cbreak()
 screen.keypad(1)
 screen.addstr(1,1, "Command >")

def shutdownUI():
 curses.nocbreak();
 screen.keypad(0);
 curses.echo()
 curses.endwin()

if __name__ == "__main__":

 screen = curses.initscr()
 initUI();

 rover = EarthRover('MotionBoard')
    
 while True:
        event=screen.getch()
        if event == ord("q"):
         shutdownUI();
	 break
        elif event == curses.KEY_UP:
         rover.MoveForward()
        elif event == curses.KEY_DOWN:
         rover.MoveBackward()
        elif event == curses.KEY_LEFT:
         rover.MoveLeft()
        elif event == curses.KEY_RIGHT:
         rover.MoveRight()

        screen.addstr(1, 12, rover.LastCmdOutput)
