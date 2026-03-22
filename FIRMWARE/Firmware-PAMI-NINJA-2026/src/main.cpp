#include <Arduino.h>
#include "pins.h"
#include "ihm.h"
#include "sensors.h"
#include "actuators.h"
#include "motion.h"
#include "match.h"

void scanI2C();
void waitStart();
void datumPosition(int teamColor);
void match();
void strategiePAMI();

void setup()
{
  delay (2000); // Attendre que tout soit stable
  initBuzzer();
  
  //init wire 
  Wire.begin(I2C_SDA, I2C_SCL);
  //Wire.setClock(100000);
  Wire.setTimeOut(150);
  delay(1000);

  initIHM();
  initSensor();
  scanI2C();
  
  initMotion();
  
  initActuators();

  drawSplashScreen();
  // pairingScreen();
  drawBackLcd();

  disableMotors();
  armsDown();

  //while(1) readSensors(true); // TODO : Test sensors

  waitStart();
}

void loop()
{
  updateMatchTime();
  match();
}

// --- Déclaration de la fonction du Scanner I2C ---
void scanI2C() {
  byte error, address;
  int nDevices = 0;
  Serial.println(F("\n--- Diagnostic du Bus I2C ---"));
  for (address = 1; address < 127; address++) {
    Wire.beginTransmission(address);
    error = Wire.endTransmission();
    if (error == 0) {
      Serial.print(F("Composant I2C detecte a l'adresse 0x"));
      if (address < 16) Serial.print("0");
      Serial.println(address, HEX);
      nDevices++;
    } else if (error == 4) {
      Serial.print(F("Erreur inconnue a l'adresse 0x"));
      if (address < 16) Serial.print("0");
      Serial.println(address, HEX);
    }
  }
  Serial.print(F("Total : "));
  Serial.print(nDevices);
  Serial.println(F(" peripherique(s) detecte(s)."));
  Serial.println(F("-----------------------------\n"));
}

void waitStart()
{
  // Attendre que la tirette ne soit plus présente
  infoLCD("Remove Tirette");
  while (getTirette())
  {
    delay(250);
    checkColorTeam();
  }
  playTirette();
  // Attendre que la tirette soit insérée
  infoLCD("Insert Tirette");
  while (!getTirette())
  {
    delay(500);
    checkColorTeam();
  }
  playTirette();
  // Datum position du PAMI
  delay(1000);
  datumPosition(getTeamColor());
  setRobotState(READY);
  infoLCD("Robot Ready");
  delay(2000);
  // Attendre que la tirette soit bien insérée pour éviter les faux-départs
  infoLCD("Insert Tirette");
  while (!getTirette())
    delay(500);
  playTirette();
  // Attendre que la tirette soit retirée pour débuter le match
  infoLCD("Wait Start");
  while (getTirette())
    delay(250);
  playTirette();
  // Le match commence
  setRobotState(MATCH_STARTED);
  infoLCD("Go Match !");
  // Démarrage du compteur !
  startMatch();
}



void datumPosition(int teamColor)
{
  enableMotors();
  if (teamColor == TEAM_BLUE)
  {
    // Datum at low Speed
    setMaxSpeed(DATUM_SPEED);
    setAcceleration(DATUM_ACCELERATION);

    // Datum X
    go(-100);
    // Save X position and orientation
    setCurrentX(3000 - CENTER_POSITION_MM);
    setCurrentRot(180);
    // Orientate robot
    goTo(3000 - 250, 0, 90);

    go(-100);
    // SaveY position
    setCurrentY(CENTER_POSITION_MM);
    setCurrentRot(90);

    // Go to safe position

      goTo(3000 - 250, 100, 180);
      go(-150);
      setCurrentX(2900);
  }
  else if (teamColor == TEAM_YELLOW)
  {
    // Datum at low Speed
    setMaxSpeed(DATUM_SPEED);
    setAcceleration(DATUM_ACCELERATION);

    // Datum X
    go(-100);
    // Save X position and orientation
    setCurrentX(CENTER_POSITION_MM);
    setCurrentRot(0);
    // Orientate robot
    goTo(250, 0, 90);

    go(-100);
    // SaveY position
    setCurrentY(CENTER_POSITION_MM);
    setCurrentRot(90);

    // Go to safe position
      goTo(250, 100, 0);
      go(-150);
      setCurrentX(100);
  }

  setMaxSpeed(MAX_SPEED);
  setAcceleration(MAX_ACCELERATION);
}

void match()
{
  if (getMatchState() == PAMI_RUN)
  {
    enableMotors();
    strategiePAMI();
    setMatchState(PAMI_STOP);
  }
  else if (getMatchState() == PAMI_STOP)
  {
    disableMotors(); // Desactive les moteurs
    infoLCD("Fin Match !");
    while (1)
      armsFiesta(); // Fin de match
  }
  else
  {
    disableMotors(); // Desactive les moteurs
  }
}

void strategiePAMI()
{
  setOpponentChecking(false);
    if (getTeamColor() == TEAM_BLUE)
    {
      goTo(3000-500, 100);
      setOpponentChecking(false);
      setMaxSpeed(MAX_SPEED * 0.40f);
      setAcceleration(MAX_ACCELERATION * 0.40f);
      goTo(3000-1300, 100, 90);
      setMaxSpeed(MAX_SPEED * 0.10f);
      setAcceleration(MAX_ACCELERATION * 0.10f);
      go(-200);
      setCurrentY(CENTER_POSITION_MM);
      setMaxSpeed(MAX_SPEED * 0.30f);
      setAcceleration(MAX_ACCELERATION * 0.30f);
      goTo(3000-1300, 430);
    }
    else
    {
      goTo(500, 100);
      setMaxSpeed(MAX_SPEED * 0.40f);
      setAcceleration(MAX_ACCELERATION * 0.40f);
      goTo(1300, 100, 90);
      setMaxSpeed(MAX_SPEED * 0.10f);
      setAcceleration(MAX_ACCELERATION * 0.10f);
      go(-200);
      setCurrentY(CENTER_POSITION_MM);
      setMaxSpeed(MAX_SPEED * 0.30f);
      setAcceleration(MAX_ACCELERATION * 0.30f);
      goTo(1300, 430);
    }
}