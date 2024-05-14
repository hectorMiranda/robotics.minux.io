#include "robot-1.h"

void Robot::begin(const int leftStep, const int leftDir, const int rightStep, const int rightDir) {
  engine.init();

  stepperLeft = engine.stepperConnectToPin(leftStep);
  if(stepperLeft) {
    stepperLeft->setDirectionPin(leftDir);
  }
  stepperRight = engine.stepperConnectToPin(rightStep);
  if(stepperRight) {
    stepperRight->setDirectionPin(rightDir);
  }
}

void Robot::setSpeed(double speed) {
  const uint32_t stepsPerSec = inchesToSteps(speed);
  stepperLeft->setSpeedInHz(stepsPerSec);
  stepperRight->setSpeedInHz(stepsPerSec);
}

void Robot::setAccel(double accel) {
  const uint32_t stepsPerSec = inchesToSteps(accel);
  stepperLeft->setAcceleration(stepsPerSec);
  stepperRight->setAcceleration(stepsPerSec);
}

void Robot::forward(double inches, bool wait) {
  const int32_t steps = inchesToSteps(inches);
  stepperLeft->move(steps);
  stepperRight->move(-steps);

  if(wait) {
    waitForFinish();
  }
}

void Robot::runForward() {
  stepperLeft->runForward();
  stepperRight->runBackward();
}

void Robot::backward(double inches, bool wait) {
  return forward(-inches, wait);
}

void Robot::runBackward() {
  stepperLeft->runBackward();
  stepperRight->runForward();
}

void Robot::turnRight(double degrees, bool wait) {
  const int32_t steps = degreesToSteps(degrees);
  stepperLeft->move(steps);
  stepperRight->move(steps);

  if(wait) {
    waitForFinish();
  }
}

void Robot::turnLeft(double degrees, bool wait) {
  return turnRight(-degrees, wait);
}

void Robot::stop(bool wait, double deceleration) {
  const uint32_t steps = inchesToSteps(deceleration);
  uint32_t       leftAccel, rightAccel;
  if(steps > 0) {
    leftAccel  = stepperLeft->getAcceleration();
    rightAccel = stepperRight->getAcceleration();
    stepperLeft->setAcceleration(steps);
    stepperRight->setAcceleration(steps);
    stepperLeft->applySpeedAcceleration();
    stepperRight->applySpeedAcceleration();
  }
  stepperLeft->stopMove();
  stepperRight->stopMove();
  if(wait) {
    waitForFinish();
    if(steps > 0) {
      stepperLeft->setAcceleration(leftAccel);
      stepperRight->setAcceleration(rightAccel);
    }
  }
}

bool Robot::isRunning() {
  return stepperLeft->isRunning() || stepperRight->isRunning();
}

void Robot::waitForFinish() {
  while(isRunning())
    ;
}

int32_t Robot::getPosition() {
  return stepperLeft->getCurrentPosition();
}

double Robot::getPositionInches() {
  return stepsToInches(getPosition());
}

double Robot::getPositionDegrees() {
  return stepsToDegrees(getPosition());
}

void Robot::setPosition(int32_t position) {
  stepperLeft->setCurrentPosition(position);
  stepperRight->setCurrentPosition(-position);
}
