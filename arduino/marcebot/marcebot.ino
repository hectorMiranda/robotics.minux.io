#include <FastAccelStepper.h>
#include "robot.h"

const int driverL_Step = 10;
const int driverL_Dir = 12;
const int driverR_Step = 9;
const int driverR_Dir = 11;
int sensorVal;
int distance;
int turns;
const int back_right = 2;
const int back_left = 3;
const int front_right = 6;
const int front_left = 7;


const int sensorPin = A0;
const double wheelDiameter = 3.14; // The diameter of the drive wheels
const double wheelBase     = 9.0; // The distance between the centers of the driver wheels
const int microsteps       = 8;
const int motorStepsPerRev = 200;
const double speed = 5; //10; // inches per second
const double accel = 5; //10; // inches per second per second
const int MODE_COLLISION_AVOIDANCE = 0;
const int MODE_DEAD_RECKONING = 1;
const int MODE_PATH_PLANNING = 2;

//const bool collission_avoidance = false;
//const bool path_planning = true;

int currentMode = 2;//collission_avoidance ? MODE_COLLISION_AVOIDANCE : (path_planning ? MODE_PATH_PLANNING : MODE_DEAD_RECKONING);

Robot robot(microsteps, motorStepsPerRev, wheelDiameter, wheelBase);
double currentHeading = 0; // Track the robot's heading



void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(back_right, OUTPUT);
  pinMode(back_left, OUTPUT);
  pinMode(front_right, OUTPUT);
  pinMode(front_left, OUTPUT);

  



  
  Serial.begin(9600);
  robot.begin(driverL_Step, driverL_Dir, driverR_Step, driverR_Dir);

  delay(1000);  // Give time for the robot to be released

  robot.setSpeed(speed);
  robot.setAccel(accel);
  robot.setPosition(0);
}

void loop() {
  switch (currentMode) {
    case MODE_COLLISION_AVOIDANCE:
      Serial.println("Collision avoidance mode");
      digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
      robot.runForward();

      while (robot.isRunning())
      {
        Serial.println("Running again..");
        distance = robot.getPosition();
        sensorVal = analogRead(A0);
        Serial.println(distance);
        if (sensorVal > 300)
        {
          Serial.println("Stop");
          robot.stop(true, 50); // Wait until completely stopped, decel at 50
          robot.backward(2);   // 12 ->Go backwards two inches
          Serial.println("Now turning left 45 degrees");
          robot.turnLeft(45, false);
          delay(1);
        }
      }
      break;
    case MODE_DEAD_RECKONING:
      Serial.println("dead reckoning mode");

      while (turns < 4) {
        Serial.println(turns);
        robot.forward(17);
        robot.stop(true, 50);
        delay(1000);
        if (turns >= 2)
          robot.turnLeft(52, false);
        else
          robot.turnLeft(49, false);
        delay(2000);
        turns++;
      }
      robot.stop(true, 50);
      break;

    case MODE_PATH_PLANNING:
      Serial.println("Path planning mode");
      digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
      robot.runForward();

      int targetDistance = 13000;

      while (robot.isRunning() && distance <= targetDistance)
      {
        Serial.println("Running again..");

        digitalWrite(front_left, HIGH); 
        digitalWrite(front_right, HIGH); 

        
        distance = robot.getPosition();
        sensorVal = analogRead(A0);
        Serial.println(distance);
        if (sensorVal > 300)
        {

          digitalWrite(front_left, LOW); 
          digitalWrite(front_right, LOW); 
          digitalWrite(back_left, HIGH); 
          digitalWrite(back_right, HIGH); 
          

          Serial.println("Stop");
          robot.stop(true, 50); // Wait until completely stopped, decel at 50
          //robot.backward(2);   // 12 ->Go backwards two inches

          Serial.println("Now turning left 45 degrees");
          robot.turnLeft(45, true);
          //robot.stop(true,50);
          //delay(1000);
          robot.forward(9);
          distance = distance - 9;
          robot.stop(true, 50);
          //delay(1000);
          robot.turnRight(45, true);

          robot.forward(9);
          distance = distance - 9;

          //robot.stop(true,50);
          //delay(1000);

          robot.turnRight(45, true);
          robot.forward(9);
          distance = distance - 9;

          robot.turnLeft(43, false);
          delay(1000);

        }

          digitalWrite(back_left, LOW); 
          digitalWrite(back_right, LOW); 
        
      }

      robot.stop(true, 50);
      delay(500);
      break;

    default:
      Serial.println("Unknown mode");
      break;
  }
}
