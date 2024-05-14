/**
 * This template can be used to program the robot for MIT-101. 
 * You will need to edit the values for wheelDiameter and wheelBase before movement will work correctly.
 * 
 * By default, the LED on pin 13 will blink to verify that the program is running.
 *
 * Libraries used:
 *   - FastAccelStepper
 */
#include <FastAccelStepper.h>
#include "robot.h"

const int driverL_Step = 10;
const int driverL_Dir = 12;
const int driverR_Step = 9;
const int driverR_Dir = 11;
int sensorVal;
int distance;
const int sensorPin = A0;
const double wheelDiameter = 3.14; // The diameter of the drive wheels
const double wheelBase     = 9.0; // The distance between the centers of the driver wheels
const int microsteps       = 8;
const int motorStepsPerRev = 200;
const double speed = 5; //10; // inches per second
const double accel = 5; //10; // inches per second per second

Robot robot(microsteps, motorStepsPerRev, wheelDiameter, wheelBase);  

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  Serial.begin(9600);
  robot.begin(driverL_Step, driverL_Dir, driverR_Step, driverR_Dir);

  delay(1000);  // Give time for the robot to be released

  robot.setSpeed(speed);
  robot.setAccel(accel);
  robot.setPosition(0);
}

void loop() {
  digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN)); // Blink the built-in LED
  
 //robot.forward(24,false); // This moves the robot forward 12 inches (1 foot)- true will wait until motion is
                               // done before starting next move
 robot.runForward();          // THis moves the robot forward continuosly...                       
  while(robot.isRunning()) 
  { 
    Serial.println("Running again..");
    distance = robot.getPosition();
    sensorVal = analogRead(A0);
    Serial.println(distance);
    if (sensorVal > 250)
    {                     
       Serial.println("Stop");
       robot.stop(true,50); // Wait until completely stopped, decel at 50
       robot.backward(1);   // 12 ->Go backwards two inches
       Serial.println("Now turning left 45 degrees");
       robot.turnLeft(10, false);
       delay(1);
    }
// This code below will stop the robot if it's moved more than 10000 units
    
//   if (distance > 10000)
//   {
//     robot.stop(true,500);
//     delay(10000);
//     distance = 0;     // reset distance variable
//   }
  }
  //robot.backward(3); // This moves the robot backward 12 inches (1 foot)
  
  delay(2000); // This will delay for 5 seconds

  robot.turnRight(10); // This turns the robot around 360 degrees (1 rotation)
}
