// sensors.cpp
#include "sensors.h"

Adafruit_VL53L0X sensors[3];
uint16_t sensorsValue[3];
bool sensorsState[3];
bool sensorReady[3] = {false, false, false}; // Suit l'état d'initialisation de chaque capteur
uint16_t sensor1 = 0;
uint16_t sensor2 = 0;
uint16_t sensor3 = 0;

long previousTime = 0;
bool debugSensor = false; // Mettre son robot en mode debug : oui / Mettre son robot en mode "des bugs" : Non - HistoriCode97 - 03/12/2023

// Filtrage
const float alpha = 0.2; // entre 0 (très lissé) et 1 (aucun lissage)
const int threshold = 200; // valeur max de variation tolérée

// Nouvelles adresses I2C à assigner aux capteurs (différentes de 0x29)
const uint8_t sensorAddresses[3] = { 0x30, 0x31, 0x32 };

void initSensor() {

  //Réinitialisation matérielle (Hard Reset) de tous les capteurs
  for (uint8_t i = 0; i < 3; i++) {
    pinMode(xshutPins[i], OUTPUT);
    digitalWrite(xshutPins[i], LOW);
  }
  delay(100); // Délai de décharge

  // Séquence d'activation et d'adressage des capteurs
  for (uint8_t i = 0; i < 3; i++) {
    // Réveil du capteur cible
    digitalWrite(xshutPins[i], HIGH);
    delay(50); // Temps d'amorçage du firmware interne

    // La méthode begin() d'Adafruit gère le changement d'adresse
    if (!sensors[i].begin(sensorAddresses[i])) {
      Serial.print(F("Echec initialisation capteur "));
      Serial.println(i);
      sensorReady[i] = false; // Marqué comme défaillant
    } else {
      Serial.print(F("Capteur "));
      Serial.print(i);
      Serial.print(F(" initialise a l'adresse 0x"));
      Serial.println(sensorAddresses[i], HEX);
      sensorReady[i] = true;  // Marqué comme opérationnel
    }
  }
}

bool readSensors(bool setDebug)
{
    bool state = SENSORS_OK;

    if (millis() - previousTime > READ_TIME_PERIOD_MS)
    {
        previousTime = millis();
        readSensor(0,setDebug);
        readSensor(1,setDebug);
        readSensor(2,setDebug);

        state = sensorsState[0] && sensorsState[1] && sensorsState[2];
        
        if (setDebug){
            if (state == true){
                String str = String(sensorsValue[0])+ "   " + String(sensorsValue[1])+ "   " + String(sensorsValue[2]);
                debug(str);
            }
            //else debug("ATTENTION");
        }
        
    }
    return state;
}

bool readSensor(int sensorNumber, bool setDebug){

    VL53L0X_RangingMeasurementData_t measure;

    bool state = true;
    uint16_t tempValue = 0;
    bool timeoutState = false;
    bool maxValueReached =false;

    if (sensorReady[sensorNumber]) {
        sensors[sensorNumber].rangingTest(&measure, false);
        if (measure.RangeStatus != 4) { 
            tempValue = measure.RangeMilliMeter;
            timeoutState = sensors[sensorNumber].timeoutOccurred();
            maxValueReached = tempValue >= MAX_SENSOR_VALUE;

            if (timeoutState || maxValueReached ) state = false;
            else sensorsValue[sensorNumber] = tempValue ;
        
            sensorsState[sensorNumber] = state ;

            if (setDebug){
                if (timeoutState)debugLCD("TIMEOUT");
                if (maxValueReached)debugLCD("MAXVALUE");
            }
        }
    }
    return state;
}


bool checkOpponent(uint16_t distance)
{
    bool detect = false;
    if (readSensors())
    {
        if (sensorsState[0] && sensorsValue[0] <= distance) detect = true;
        if (sensorsState[2] && sensorsValue[2] <= distance) detect = true;
    }
    return detect;
}

uint16_t sensorFilter(uint16_t rawValue, float previousValue) {
    int delta = abs((int)rawValue - (int)previousValue);

    if (delta < threshold) {
        previousValue = alpha * rawValue + (1.0 - alpha) * previousValue;
    }
    return (uint16_t)previousValue;
}
