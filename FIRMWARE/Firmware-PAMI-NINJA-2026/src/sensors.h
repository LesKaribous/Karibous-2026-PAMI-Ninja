// sensors.h
#ifndef SENSORS_H
#define SENSORS_H

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <Wire.h>
#include <Adafruit_VL53L0X.h>
//#include <VL53L0X.h>

#include "pins.h" 
#include "ihm.h" 

// SENSOR STATE
#define SENSOR_NOK   0
#define SENSOR_OK  1
#define SENSORS_NOK   0
#define SENSORS_OK  1

// Opponent detection distances
#define MIN_DISTANCE_MM 200 // TODO : 300
#define MAX_DISTANCE_MM 500
// Sensors limit and parameters
#define MAX_SENSOR_VALUE 8100 // Strange value at 8190
#define READ_TIME_PERIOD_MS 250 // Lecture plus espacee pour moins perturber les moteurs
// Filtrage
#define NUM_READINGS 9             // Nombre de mesures à faire
#define OUTLIERS_TO_REMOVE 2       // Nombre de valeurs extrêmes à retirer de chaque côté

// Déclaration des fonctions
void initSensor();
void initSensorTask();
void setOpponentMonitoring(bool enabled);
bool isOpponentDetected();
bool readSensors(bool setDebug = false);
bool readSensor(int sensorNumber, bool setDebug = false);
uint16_t sensorFilter(uint16_t rawValue, float previousValue);
bool checkOpponent(uint16_t distance = MIN_DISTANCE_MM);

#endif // SENSORS_H
