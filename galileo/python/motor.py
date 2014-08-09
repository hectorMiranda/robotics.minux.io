#!/usr/bin/env python
import sys
import os
import curses

galileo_path = "/media/mmcblk0p1/";
if galileo_path not in sys.path:
    sys.path.append(galileo_path);

from pyGalileo import *

def setup():
 pinMode(PWM4_11, OUTPUT);     
 pinMode(PWM7_10, OUTPUT);     
 pinMode(PWM1_9, OUTPUT);     
 pinMode(PWM6_6, OUTPUT);     

 digitalWrite(PWM4_11, HIGH);
 digitalWrite(PWM7_10, HIGH);
 digitalWrite(PWM1_9, HIGH);
 digitalWrite(PWM6_6, HIGH);
    

if __name__ == "__main__":
 PWM4_11 = 11
 PWM7_10 = 10
 PWM1_9 = 9
 PWM6_6 = 6

 setup();
    
 while True:
  pin = raw_input('Enter pin number>')
  intensity = raw_input('Intensity for pin %s>' % pin)
  
  if "11" in pin:
   analogWrite(PWM4_11, int(intensity));

  if "10" in pin:
   analogWrite(PWM7_10, int(intensity));

  if "9" in pin:
   analogWrite(PWM1_9, int(intensity));

  if "6a" in pin:
   analogWrite(PWM6_6, int(intensity));

  if "6h" in pin:
   digitalWrite(PWM6_6, HIGH);

  if "6l" in pin:
   digitalWrite(PWM6_6, LOW);
