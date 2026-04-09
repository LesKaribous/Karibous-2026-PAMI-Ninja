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
TaskHandle_t sensorTaskHandle = nullptr;
volatile bool opponentMonitoringEnabled = false;
volatile bool opponentDetected = false;
volatile int opponentDetectionDirection = OPPONENT_DIR_NONE;
portMUX_TYPE opponentStateMux = portMUX_INITIALIZER_UNLOCKED;
float filteredSensorValues[3] = {0.0f, 0.0f, 0.0f};
bool filteredSensorReady[3] = {false, false, false};
bool filteredForwardDetected = false;
bool filteredBackwardDetected = false;
uint8_t forwardDetectCount = 0;
uint8_t forwardReleaseCount = 0;
uint8_t backwardDetectCount = 0;
uint8_t backwardReleaseCount = 0;

// Filtrage
const float alpha = 0.5; // entre 0 (très lissé) et 1 (aucun lissage)
const int threshold = 200; // valeur max de variation tolérée

// Nouvelles adresses I2C à assigner aux capteurs (différentes de 0x29)
const uint8_t sensorAddresses[3] = { 0x30, 0x31, 0x32 };

static bool detectOpponentFromLastReadings(uint16_t distance) {
    int direction;
    portENTER_CRITICAL(&opponentStateMux);
    direction = opponentDetectionDirection;
    portEXIT_CRITICAL(&opponentStateMux);

    if (direction == OPPONENT_DIR_FORWARD) {
        return filteredForwardDetected;
    }
    else if (direction == OPPONENT_DIR_BACKWARD) {
        return filteredBackwardDetected;
    }

    return false;
}

static void resetZoneFilterState() {
    filteredForwardDetected = false;
    filteredBackwardDetected = false;
    forwardDetectCount = 0;
    forwardReleaseCount = 0;
    backwardDetectCount = 0;
    backwardReleaseCount = 0;
}

static void resetAllFilters() {
    for (int i = 0; i < 3; ++i) {
        filteredSensorValues[i] = 0.0f;
        filteredSensorReady[i] = false;
    }
    resetZoneFilterState();
}

static void updateFilteredSensorValue(int sensorNumber) {
    if (!sensorsState[sensorNumber]) {
        filteredSensorReady[sensorNumber] = false;
        return;
    }

    if (!filteredSensorReady[sensorNumber]) {
        filteredSensorValues[sensorNumber] = sensorsValue[sensorNumber];
        filteredSensorReady[sensorNumber] = true;
        return;
    }

    filteredSensorValues[sensorNumber] = (alpha * sensorsValue[sensorNumber]) + ((1.0f - alpha) * filteredSensorValues[sensorNumber]);
}

static bool isForwardCandidateDetected() {
    bool frontLeftDetected = filteredSensorReady[0] && filteredSensorValues[0] <= MIN_DISTANCE_MM;
    bool frontRightDetected = filteredSensorReady[2] && filteredSensorValues[2] <= MIN_DISTANCE_MM;
    return frontLeftDetected || frontRightDetected;
}

static bool isBackwardCandidateDetected() {
    return filteredSensorReady[1] && filteredSensorValues[1] <= MIN_DISTANCE_MM;
}

static bool isForwardZoneClear() {
    bool frontLeftClear = !filteredSensorReady[0] || filteredSensorValues[0] >= RELEASE_DISTANCE_MM;
    bool frontRightClear = !filteredSensorReady[2] || filteredSensorValues[2] >= RELEASE_DISTANCE_MM;
    return frontLeftClear && frontRightClear;
}

static bool isBackwardZoneClear() {
    return !filteredSensorReady[1] || filteredSensorValues[1] >= RELEASE_DISTANCE_MM;
}

static void updateZoneDetectionState(bool candidateDetected, bool zoneClear, bool &zoneDetected, uint8_t &detectCount, uint8_t &releaseCount) {
    if (!zoneDetected) {
        releaseCount = 0;
        if (candidateDetected) {
            if (detectCount < OPPONENT_DETECT_CONFIRMATIONS) detectCount++;
            if (detectCount >= OPPONENT_DETECT_CONFIRMATIONS) zoneDetected = true;
        } else {
            detectCount = 0;
        }
        return;
    }

    detectCount = 0;
    if (zoneClear) {
        if (releaseCount < OPPONENT_RELEASE_CONFIRMATIONS) releaseCount++;
        if (releaseCount >= OPPONENT_RELEASE_CONFIRMATIONS) zoneDetected = false;
    } else {
        releaseCount = 0;
    }
}

static void updateOpponentFilters() {
    updateFilteredSensorValue(0);
    updateFilteredSensorValue(1);
    updateFilteredSensorValue(2);

    updateZoneDetectionState(
        isForwardCandidateDetected(),
        isForwardZoneClear(),
        filteredForwardDetected,
        forwardDetectCount,
        forwardReleaseCount
    );

    updateZoneDetectionState(
        isBackwardCandidateDetected(),
        isBackwardZoneClear(),
        filteredBackwardDetected,
        backwardDetectCount,
        backwardReleaseCount
    );
}

static bool readSensorsNow(bool setDebug)
{
    readSensor(0, setDebug);
    readSensor(1, setDebug);
    readSensor(2, setDebug);

    bool state = sensorsState[0] && sensorsState[1] && sensorsState[2];

    if (setDebug && state){
        String str = String(sensorsValue[0])+ "   " + String(sensorsValue[1])+ "   " + String(sensorsValue[2]);
        debug(str);
    }

    return state;
}

static void sensorTaskLoop(void *parameter) {
    (void)parameter;

    for (;;) {
        bool detect = false;

        if (opponentMonitoringEnabled) {
            bool sensorsOk = readSensorsNow(false);
            if (sensorsOk) {
                updateOpponentFilters();
                detect = detectOpponentFromLastReadings(MIN_DISTANCE_MM);
            } else {
                resetAllFilters();
            }
        } else {
            resetAllFilters();
        }

        portENTER_CRITICAL(&opponentStateMux);
        opponentDetected = detect;
        portEXIT_CRITICAL(&opponentStateMux);

        vTaskDelay(pdMS_TO_TICKS(READ_TIME_PERIOD_MS));
    }
}

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

void initSensorTask() {
    if (sensorTaskHandle != nullptr) return;

    xTaskCreatePinnedToCore(
        sensorTaskLoop,
        "sensorTask",
        4096,
        nullptr,
        1,
        &sensorTaskHandle,
        0
    );
}

void setOpponentMonitoring(bool enabled) {
    portENTER_CRITICAL(&opponentStateMux);
    opponentMonitoringEnabled = enabled;
    if (!enabled) {
        opponentDetected = false;
        opponentDetectionDirection = OPPONENT_DIR_NONE;
    }
    portEXIT_CRITICAL(&opponentStateMux);

    if (!enabled) resetAllFilters();
}

void setOpponentDetectionDirection(int direction) {
    portENTER_CRITICAL(&opponentStateMux);
    opponentDetectionDirection = direction;
    if (direction == OPPONENT_DIR_NONE) opponentDetected = false;
    portEXIT_CRITICAL(&opponentStateMux);

    if (direction == OPPONENT_DIR_NONE) {
        portENTER_CRITICAL(&opponentStateMux);
        resetZoneFilterState();
        portEXIT_CRITICAL(&opponentStateMux);
    }
}

bool isOpponentDetected() {
    bool detect;
    portENTER_CRITICAL(&opponentStateMux);
    detect = opponentDetected;
    portEXIT_CRITICAL(&opponentStateMux);
    return detect;
}

bool readSensors(bool setDebug)
{
    bool state = SENSORS_OK;

    if (millis() - previousTime > READ_TIME_PERIOD_MS)
    {
        previousTime = millis();
        state = readSensorsNow(setDebug);
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
    if (sensorTaskHandle != nullptr) return isOpponentDetected();

    bool detect = false;
    if (readSensors())
    {
        updateOpponentFilters();
        detect = detectOpponentFromLastReadings(distance);
    } else {
        resetAllFilters();
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
