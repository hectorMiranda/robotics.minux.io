#!/usr/bin/env python
import sys
import os
import curses

galileo_path = "/media/mmcblk0p1/";
if galileo_path not in sys.path:
    sys.path.append(galileo_path);

from pyGalileo import *

screen = curses.initscr()
curses.noecho()
curses.cbreak()
screen.keypad(1)

def init():
 #galileo pins to use
 pinMode(PWM4_11, OUTPUT);     
 pinMode(PWM7_10, OUTPUT);     
 pinMode(PWM1_9, OUTPUT);     
 pinMode(PWM6_6, OUTPUT);     


if __name__ == "__main__":
 PWM4_11 = 11
 PWM7_10 = 10
 PWM1_9 = 9
 PWM6_6 = 6

 init();
    
 while True:
        event=screen.getch()
        if event == ord("q"):
         screen.addstr("Exiting...")
	 curses.nocbreak();
	 screen.keypad(0);
	 curses.echo()
         break
        elif event == curses.KEY_UP:
         screen.clear()
         screen.addstr("Forward")
	 digitalWrite(PWM4_11, HIGH);
	 digitalWrite(PWM7_10, LOW);
	 digitalWrite(PWM1_9, LOW);
	 digitalWrite(PWM6_6, LOW);
        elif event == curses.KEY_DOWN:
         screen.clear()
         screen.addstr("Backwards")
	 digitalWrite(PWM4_11, LOW);
	 digitalWrite(PWM7_10, HIGH);
	 digitalWrite(PWM1_9, LOW);
	 digitalWrite(PWM6_6, LOW);
        elif event == curses.KEY_LEFT:
         screen.clear()
         screen.addstr("Left")
	 digitalWrite(PWM4_11, LOW);
	 digitalWrite(PWM7_10, LOW);
	 digitalWrite(PWM1_9, HIGH);
	 digitalWrite(PWM6_6, LOW);
        elif event == curses.KEY_RIGHT:
         screen.clear()
         screen.addstr("Right")
	 digitalWrite(PWM4_11, LOW);
	 digitalWrite(PWM7_10, LOW);
	 digitalWrite(PWM1_9, LOW);
	 digitalWrite(PWM6_6, HIGH);
 curses.endwin()

