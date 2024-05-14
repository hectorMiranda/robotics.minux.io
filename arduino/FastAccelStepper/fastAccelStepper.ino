/**
 * \file MIT-101.template.ino
 * \brief A simplified template for programming the MIT-101 robot
 * \version 1.0
 * \date 01/30/2022
 * This template can be used to program the robot for MIT-101. 
 * You will need to edit the values for wheelDiameter and wheelBase before movement will work correctly.
 * 
 * By default, the LED on pin 13 will blink to verify that the program is running.
 *
 * Libraries used:
 *   - FastAccelStepper
 */
#include <FastAccelStepper.h>
#include "robot-1.h"

/* Pins */
const int driverL_Step = 10;
const int driverL_Dir = 12;
const int driverR_Step = 9;
const int driverR_Dir = 11;

const int sensorPin = A0;

/* Constants */
//const double wheelDiameter = 3.14; // The diameter of the drive wheels
const double wheelDiameter = 3.25; // The diameter of the drive wheels

const double wheelBase     = 9.0; // The distance between the centers of the driver wheels

const int microsteps       = 8;
const int motorStepsPerRev = 200;

const double speed = 10; // inches per second
const double accel = 10; // inches per second per second

/* Definitions (Do not edit) */
Robot robot(microsteps, motorStepsPerRev, wheelDiameter, wheelBase);

/* Main Program (Edit this) */
void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  Serial.begin(9600);
  robot.begin(driverL_Step, driverL_Dir, driverR_Step, driverR_Dir);

  delay(1000);  // Give time for the robot to be released

  robot.setSpeed(speed);
  robot.setAccel(accel);
}

/* Write your program here */
void loop() {
  digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN)); // Blink the built-in LED
  
  //robot.forward(36); // This moves the robot forward 12 inches (1 foot)
  robot.backward(9); // This moves the robot forward 12 inches (1 foot)

  robot.turnRight(51); // This turns the robot around 360 degrees (1 rotation)
  
  //robot.backward(12); // This moves the robot backward 12 inches (1 foot)
  
  //robot.backward(5); // This moves the robot backward 12 inches (1 foot)
  
  delay(1000); // This will delay for 5 seconds
  
  
}
