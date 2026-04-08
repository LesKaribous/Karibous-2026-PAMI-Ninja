// actuators.h
#ifndef ACTUATORS_H
#define ACTUATORS_H

#include <Arduino.h>
#include <ESP32Servo.h>

#include "pins.h" 
#include "ihm.h"
#include "match.h"

#define ARM_UP_LEFT 130
#define ARM_DOWN_LEFT 10
#define ARM_UP_RIGHT 62
#define ARM_DOWN_RIGHT 180

// Déclaration des fonctions
void initActuators();
void leftUp();
void rightUp();
void leftDown();
void rightDown();
void armsDown();
void armsUp();
void testArms();
void armsFiesta();

#endif // ACTUATORS_H
