// actuators.h
#ifndef ACTUATORS_H
#define ACTUATORS_H

#include <Arduino.h>
#include <ESP32Servo.h>

#include "pins.h" 
#include "ihm.h"
#include "match.h"

#define ARM_UP_LEFT 120
#define ARM_DOWN_LEFT 45
#define ARM_UP_RIGHT 45
#define ARM_DOWN_RIGHT 120

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
