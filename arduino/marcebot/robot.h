#ifndef __ROBOT_H__
#define __ROBOT_H__

#include <Arduino.h>
#include <FastAccelStepper.h>

class Robot {
 private:
  FastAccelStepperEngine engine = FastAccelStepperEngine();

  const int    stepsPerRev;
  const double wheelDiameter;
  const double wheelBase;

 public:
  FastAccelStepper* stepperLeft  = NULL;
  FastAccelStepper* stepperRight = NULL;

  /**
   * \brief Construct a new Robot object
   *
   * \param microsteps The number of microsteps
   * \param motorStepsPerRev The number of steps per revolution for the chosen motor (usually 200)
   * \param wheelDiameter The diameter of the wheels
   * \param wheelBase The distance between the wheels
   */
  Robot(int microsteps, int motorStepsPerRev, double wheelDiameter, double wheelBase)
      : stepsPerRev(motorStepsPerRev * microsteps), wheelDiameter(wheelDiameter), wheelBase(wheelBase) {}

  /**
   * \brief Main setup function
   *
   * \param leftStep Step pin for the left driver
   * \param leftDir Dir pin for the left driver
   * \param rightStep Step pin for the right driver
   * \param rightDir Dir pin for the right driver
   */
  void begin(const int leftStep, const int leftDir, const int rightStep, const int rightDir);

  /**
   * \brief Set the speed of the robot
   *
   * \param speed The new speed in inches per second
   */
  void setSpeed(double speed);
  /**
   * \brief Set the acceleration of the robot
   *
   * \param accel The new acceleration in inches per second per second
   */
  void setAccel(double accel);

  /**
   * \brief Move the robot forward
   *
   * \param inches How far to move
   * \param wait If this is true, wait until the move is complete before continuing
   */
  void forward(double inches, bool wait = true);
  /**
   * \brief Move forward forever. Use the stop function to stop.
   */
  void runForward();
  /**
   * \brief Move the robot backward
   *
   * \param inches How far to move
   * \param wait If this is true, wait until the move is complete before continuing
   */
  void backward(double inches, bool wait = true);
  /**
   * \brief Move backward forever. Use the stop function to stop.
   */
  void runBackward();
  /**
   * \brief Turn the robot to the right (Clockwise seen from above)
   *
   * \param degrees How far to turn
   * \param wait If this is true, wait until the move is complete before continuing
   */
  void turnRight(double degrees, bool wait = true);
  /**
   * \brief Turn the robot to the left (Counter clockwise seen from above)
   *
   * \param degrees How far to turn
   * \param wait If this is true, wait until the move is complete before continuing
   */
  void turnLeft(double degrees, bool wait = true);
  /**
   * \brief Stop the current move
   *
   * \param wait If this is true, wait until the robot is completely stopped before continuing
   * \param deceleration
   */
  void stop(bool wait = true, double deceleration = 0);
  /**
   * \brief Is the robot currently moving
   *
   * \return bool True if yes, False if no
   */
  bool isRunning();
  /**
   * \brief Wait until the robot stops moving
   */
  void waitForFinish();

  /**
   * \brief Get the current distance traveled since the last setPosition call
   *
   * \return int32_t The distance in steps, use stepsToInches to get a distance
   */
  int32_t getPosition();
  /**
   * \brief Get the current distance traveled since the last setPosition call
   *
   * \return double The distance in inches
   */
  double getPositionInches();
  /**
   * \brief Get the current distance traveled since the last setPosition call
   *
   * \return double The distance in degrees
   */
  double getPositionDegrees();
  /**
   * \brief Set the distance traveled
   *
   * \param position The distance in steps, use inchesToSteps to enter a distance
   */
  void setPosition(int32_t position = 0);

  /**
   * \brief Convert inches to steps
   */
  int32_t inchesToSteps(double inches) { return (inches / (wheelDiameter * PI)) * stepsPerRev; }
  /**
   * \brief Convert degrees to steps
   */
  int32_t degreesToSteps(double degrees) { return inchesToSteps(wheelBase * PI * degrees / 360.0); }
  /**
   * \brief Convert steps to inches
   */
  double stepsToInches(int32_t steps) { return (wheelDiameter * PI * steps) / stepsPerRev; }
  /**
   * \brief Convert steps to degrees
   */
  double stepsToDegrees(int32_t steps) { return (360.0 / (wheelBase * PI)) * stepsToInches(steps); }
};

#endif
