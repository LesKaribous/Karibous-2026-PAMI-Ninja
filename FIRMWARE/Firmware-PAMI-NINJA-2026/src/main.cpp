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
void strategiePAMIgather();
void datumY(float dist, float angle = 270);
void datumX(float dist, int color, float angle = 0);

void setup()
{
	delay(2000); // Attendre que tout soit stable
	initBuzzer();

	// init wire
	Wire.begin(I2C_SDA, I2C_SCL);
	// Wire.setClock(100000);
	Wire.setTimeOut(150);
	delay(1000);

	initIHM();

	initSensor();
	scanI2C();
	initSensorTask();

	initMotion();

	initActuators();
	armsDown();
	delay(1000);
	armsUp();

	drawSplashScreen();
	// pairingScreen();
	drawBackLcd();

	disableMotors();

	// while(1) readSensors(true); // TODO : Test sensors

	waitStart();
}

void loop()
{
	updateMatchTime();
	match();
}

// --- Déclaration de la fonction du Scanner I2C ---
void scanI2C()
{
	byte error, address;
	int nDevices = 0;
	Serial.println(F("\n--- Diagnostic du Bus I2C ---"));
	for (address = 1; address < 127; address++)
	{
		Wire.beginTransmission(address);
		error = Wire.endTransmission();
		if (error == 0)
		{
			Serial.print(F("Composant I2C detecte a l'adresse 0x"));
			if (address < 16)
				Serial.print("0");
			Serial.println(address, HEX);
			nDevices++;
		}
		else if (error == 4)
		{
			Serial.print(F("Erreur inconnue a l'adresse 0x"));
			if (address < 16)
				Serial.print("0");
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
	unsigned long lastSlowUpdate = 0;
	// Attendre que la tirette ne soit plus présente
	infoLCD("Remove Tirette");
	while (getTirette())
	{
		updateEyes();

		if (millis() - lastSlowUpdate >= 250)
		{
			checkColorTeam();
			lastSlowUpdate = millis();
		}
	}
	playTirette();
	// Attendre que la tirette soit insérée
	infoLCD("Insert Tirette");
	while (!getTirette())
	{
		updateEyes();

		if (millis() - lastSlowUpdate >= 500)
		{
			checkColorTeam();
			lastSlowUpdate = millis();
		}
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
	{
		updateEyes();
		if (millis() - lastSlowUpdate >= 500)
		{
			lastSlowUpdate = millis();
		}
	}
	playTirette();
	// Attendre que la tirette soit retirée pour débuter le match
	infoLCD("Wait Start");
	while (getTirette())
	{
		updateEyes();
		if (millis() - lastSlowUpdate >= 250)
		{
			lastSlowUpdate = millis();
		}
	}
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
		armsMid();
		// Datum at low Speed
		setMaxSpeed(DATUM_SPEED);
		setAcceleration(DATUM_ACCELERATION);

		// Save Y position and orientation
		setCurrentY(DATUM_DISTANCE_MM);
		setCurrentX(3000 - (615 + DATUM_DISTANCE_MM));
		setCurrentRot(225.0);

		// Go to safe position
		// goTo(getCurrentX()-10, getCurrentY()+10,180.0);
		// goTo(3000 - 715, 100, 180);
		goRelative(20);
	}
	else if (teamColor == TEAM_YELLOW)
	{
		armsMid();
		// Datum at low Speed
		setMaxSpeed(DATUM_SPEED);
		setAcceleration(DATUM_ACCELERATION);

		// Save Y position and orientation
		setCurrentY(DATUM_DISTANCE_MM);
		setCurrentX(615 + DATUM_DISTANCE_MM);
		setCurrentRot(315.0);

		// Go to safe position
		// goTo(getCurrentX()+10.0, getCurrentY()+10.0,0.0);
		// goTo(715, 100, 0);
		goRelative(20);
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

void datumY(float dist, float angle)
{
	// Datum at low Speed
	setOpponentChecking(false);
	setMaxSpeed(DATUM_SPEED);
	setAcceleration(DATUM_ACCELERATION);

	float delta = angle - getCurrentRot();
	while (delta > 180.0f)
		delta -= 360.0f;
	while (delta < -180.0f)
		delta += 360.0f;
	turnRelative(delta);

	go(-dist);
	setCurrentY(CENTER_POSITION_MM);

	setMaxSpeed(MAX_SPEED);
	setAcceleration(MAX_ACCELERATION);
	setOpponentChecking(true);
}

void datumX(float dist, int color, float angle)
{
	setOpponentChecking(false);
	setMaxSpeed(DATUM_SPEED);
	setAcceleration(DATUM_ACCELERATION);

	float delta = angle - getCurrentRot();
	while (delta > 180.0f)
		delta -= 360.0f;
	while (delta < -180.0f)
		delta += 360.0f;
	turnRelative(delta);

	go(-dist);
	if (color == TEAM_YELLOW)
		setCurrentX(612 + CENTER_POSITION_MM);
	else
		setCurrentX(2388 - CENTER_POSITION_MM);

	setMaxSpeed(MAX_SPEED);
	setAcceleration(MAX_ACCELERATION);
	setOpponentChecking(true);
}

void strategiePAMI()
{
	setOpponentChecking(true);
	setMaxSpeed( MAX_SPEED);
	setAcceleration(MAX_ACCELERATION);
	armsUp();
	if (getTeamColor() == TEAM_BLUE)
	{
		// goTo(3000 - 715, 70);

		goTo(3000 - 1480, 70); // Go straight
		datumY(30);

		goTo(3000 - 1480, 225, 0); // Go to side of Blue_Fridge02
		// armsDown();
		goTo(3000 - 1385, 225); // Offset Blue_Fridge02 PUSH 50mm
		goRelative(-50);
		setOpponentChecking(false);
		goTo(3000 - 1450, 70, 0); // Go back to wall
		setOpponentChecking(true);
		goTo(3000 - 970, 70); // Go to side of Blue_Fridge01
		datumY(30);
		goTo(3000 - 970, 275);

		goTo(3000 - 1100, 275, 180); // Offset Blue_Fridge01 PUSH 100mm
		goRelative(-70);
		setOpponentChecking(false);
		goTo(3000 - 1050, 70);
		setOpponentChecking(true);
		goTo(3000 - 1250, 70, 270); // Go to center of fridge pile
		datumY(20);
		armsDown();
		wait(800);
		goTo(3000 - 1250, 417); // Push fridge pile
		goRelative(-50);
		goRelative(53);
		goRelative(-60);
		align(-135);
	}
	else
	{
		// goTo(715, 70);

		goTo(1480, 70); //Go straight
		datumY(30);

		goTo(1480, 225, 180); // Go to side of Yellow_Fridge02
		// armsDown();
		goTo(1385, 225); // Offset Yellow_Fridge02 PUSH 50mm
		goRelative(-50);
		setOpponentChecking(false);
		goTo(1450, 70, 180); //Go back to wall
		setOpponentChecking(true);
		goTo(970, 70); //Go to side of Yellow_Fridge01
		datumY(30);
		goTo(970, 275);

		goTo(1100, 275, 0); // Offset Yellow_Fridge01 PUSH 100mm
		goRelative(-70);
		setOpponentChecking(false);
		goTo(1050,70);
		setOpponentChecking(true);
		goTo(1250, 70, 270); //Go to center of fridge pile
		datumY(20);
		armsDown();
		wait(800);
		goTo(1250, 417); // Push fridge pile
		goRelative(-50);
		goRelative(53);
		goRelative(-60);
		align(-45);
	}
}
