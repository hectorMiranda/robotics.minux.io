#include <FastAccelStepper.h>
#include "robot.h"

const int driverL_Step = 10;
const int driverL_Dir = 12;
const int driverR_Step = 9;
const int driverR_Dir = 11;
int sensorVal;
int distance;
int turns;
const int sensorPin = A0;
const double wheelDiameter = 3.14; // The diameter of the drive wheels
const double wheelBase     = 9.0; // The distance between the centers of the driver wheels
const int microsteps       = 8;
const int motorStepsPerRev = 200;
const double speed = 5; //10; // inches per second
const double accel = 5; //10; // inches per second per second
const bool collission_avoidance = false;

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
  
  if(collission_avoidance){
      digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN)); 
      robot.runForward();                           
      
      while(robot.isRunning())
      { 
          Serial.println("Running again..");
          distance = robot.getPosition();
          sensorVal = analogRead(A0);
          Serial.println(distance);
          if (sensorVal > 300)
          {                     
             Serial.println("Stop");
             robot.stop(true,50); // Wait until completely stopped, decel at 50
             robot.backward(2);   // 12 ->Go backwards two inches
             Serial.println("Now turning left 45 degrees");
             robot.turnLeft(45, false);
             delay(1);
          }
      }
  }
  else //dead reckoning mode...
  {
      while(turns<4){
          Serial.println(turns);
          robot.forward(17);
          robot.stop(true,50);      
          delay(1000);
          if(turns >= 2)            
            robot.turnLeft(52, false);
          else
            robot.turnLeft(49, false);
          delay(2000);                           
          turns++;
      }
      robot.stop(true,50); 

  }
}
