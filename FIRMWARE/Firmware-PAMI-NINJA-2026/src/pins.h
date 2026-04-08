// pins.h
#ifndef PINS_H
#define PINS_H

// Pins Robot Number
//const int BotNumb01 = 2;
//const int BotNumb02 = 1;

// Pins I2C
const int I2C_SDA = 5;
const int I2C_SCL = 6;

// Pins Motors - Drivers
const int EN      = 4;
const int STEP_G  = 41;
const int DIR_G   = 42;
const int STEP_D  = 40;
const int DIR_D   = 3;
//const int MS1 = 13;
//const int MS2 = 14;

// Pins IHM
const int ColorTeam = 39;
const int Tirette   = 38;
const int ledStatus = 2;
const int Buzzer = 1;

// Pins servo
const int pinServo01  = 9;
const int pinServo02  = 13;

// Pins Sensors
const int xshutPins[3] = { 44, 10, 43 };

// Pins EV pump
const int pinPump01 = 11;
const int pinPump02 = 8;

const int pinEV01 = 7;
const int pinEV02 = 12;

#endif // PINS_H