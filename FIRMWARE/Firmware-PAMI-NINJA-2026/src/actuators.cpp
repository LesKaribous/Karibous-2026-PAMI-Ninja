// actuators.h
#include "actuators.h"

Servo servoArmLeft;
Servo servoArmRight;
int frequency = 200; // Hz

static void waitWithEyesUpdate(unsigned long durationMs) {
  unsigned long start = millis();
  while (millis() - start < durationMs) {
    updateEyes();
    delay(10);
  }
}

void initActuators() {
    servoArmLeft.setPeriodHertz(frequency);
    servoArmRight.setPeriodHertz(frequency);

    servoArmLeft.attach(
        pinServo01,
        1000,
        2000
    );
    delay(250);

    servoArmRight.attach(
        pinServo02,
        1000,
        2000
    );
    delay(250);
}

void leftUp(){
  servoArmLeft.write(ARM_UP_LEFT);
}

void rightUp(){
  servoArmRight.write(ARM_UP_RIGHT);
}

void leftMid(){
  servoArmLeft.write(ARM_MID_LEFT);
}

void rightMid(){
  servoArmRight.write(ARM_MID_RIGHT);
}

void leftDown(){
  servoArmLeft.write(ARM_DOWN_LEFT);
}

void rightDown(){
  servoArmRight.write(ARM_DOWN_RIGHT);
}

void armsUp(){
  leftUp();
  rightUp();
  debug("Arms up");
}

void armsMid(){
  leftMid();
  rightMid();
  debug("Arms mid");
}

void armsDown(){
  leftDown();
  rightDown();
  debug("Arms down");
}

void testArms(){
  armsDown();
  delay(1000);
  armsUp();
  delay(1000);
}

void armsFiesta() {

  updateEyes();

  rightUp();
  leftUp();
  waitWithEyesUpdate(1000);
  
  rightDown();
  leftDown();
  waitWithEyesUpdate(1000);
}
