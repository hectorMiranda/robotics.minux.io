#include "FastAccelStepper.h"

const int stepPin = 2;  // Stepper motor step pin
const int dirPin = 3;   // Stepper motor direction pin

// Create stepper object
FastAccelStepperEngine engine = FastAccelStepperEngine();
FastAccelStepper *stepper;

void setup() {
  engine.init();
  stepper = engine.stepperConnectToPins(stepPin, dirPin);
  if (stepper) {
    stepper->setDirectionPinPolarity(HIGH);
    stepper->setEnablePinHighActive(false);

    // Set maximum speed and acceleration
    stepper->setMaxSpeed(1000);       // Steps per second
    stepper->setAcceleration(500);    // Steps per second^2
  }
}

void loop() {
  // Move 1000 steps forward
  stepper->moveTo(1000);
  while (stepper->isRunning()) {
    // Wait until the movement is complete
  }

  delay(1000); // Wait for a second

  // Move back to position 0
  stepper->moveTo(0);
  while (stepper->isRunning()) {
    // Wait until the movement is complete
  }

  delay(1000); // Wait for a second
}
