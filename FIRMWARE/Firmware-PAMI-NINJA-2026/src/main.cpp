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
		goTo(3000 - 715, 100, 180);
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
		goTo(715, 100, 0);
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
	setOpponentChecking(false);
	setMaxSpeed(MAX_SPEED * 0.80f);
	setAcceleration(MAX_ACCELERATION * 0.80f);
	armsUp();
	if (getTeamColor() == TEAM_BLUE)
	{
		goTo(3000 - 715, 100);
		goTo(3000 - 1350, 70);

		datumY(30, -90);
		armsDown();
		goTo(3000 - 1350, 415, 270);
		armsUp();
		goTo(3000 - 1220, 100);
		goTo(3000 - 1103, 100);

		datumY(80, -90);
		armsDown();
		goTo(3000 - 1100, 415, 270);
		armsUp();

		goTo(3000 - 1000, 150);
		goTo(3000 - 800, 150);
		datumY(120, -90);
		armsDown();
		goTo(3000 - 800, 417, 270);
		goRelative(-50);
		goRelative(53);
		armsUp();
		goRelative(-150);
		goTo(3000 - 1250, 400, 270 + 45);
	}
	else
	{
		goTo(715, 100);
		goTo(800, 150);
		datumY(100);
		armsDown();
		goTo(800, 417, 270); // Push 4 pile
		goRelative(-50);
		goRelative(53);
		armsUp();

		goRelative(-130);
		datumX(150, TEAM_YELLOW);
		// armsDown();
		goTo(1100, 280, 0); // Offset Yellow_Fridge01
		goRelative(-70);
		// armsUp();
		goTo(1000, 70); // Return to wall

		goTo(1480, 70);
		datumY(100);
		goTo(1480, 225, 180);
		// armsDown();
		goTo(1400, 225); // Offset Yellow_Fridge02
		goRelative(-70);
		// armsUp();
		goTo(1450, 70, 180);
		goTo(1250, 70, 270);
		datumY(100);
		armsDown();
		goTo(1250, 417); // Push fridge pile
		goRelative(-50);
		goRelative(53);
		armsUp();

		turn(-45);

		// goTo(1350, 70);
		// datumY(30);
		// armsDown();
		// goTo(1350, 415, 270);
		// armsUp();

		// goTo(1220, 100);
		// goTo(1103, 100);

		// datumY(80);
		// armsDown();
		// goTo(1100, 415, 270);
		// armsUp();

		// goTo(1000, 150);
		// goTo(1250, 400, 270 + 45);
	}
}
