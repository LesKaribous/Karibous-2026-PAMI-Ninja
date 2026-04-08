// actuators.h
#include "actuators.h"

Servo servoArmLeft;
Servo servoArmRight;
int frequency = 200; // Hz

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
  // Attendre que elapsedTime soit un multiple de 1000
  while ((millis() - getStartTime()) % 1000 != 0) {
    bool enableMelody = (millis() - getStartTime()) >= TIME_END_MATCH;
    if (enableMelody) runMelody();
    delay(1);
  }

  rightUp();
  leftUp();
  pauseWithMelody(500, (millis() - getStartTime()) >= TIME_END_MATCH);

  rightDown();
  leftDown();
  pauseWithMelody(500, (millis() - getStartTime()) >= TIME_END_MATCH);
}

