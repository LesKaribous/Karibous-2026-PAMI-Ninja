
#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_VL53L0X.h>

// --- Configuration des Écrans OLED ---
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1 

// Instanciation des deux écrans
Adafruit_SSD1306 display1(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
Adafruit_SSD1306 display2(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Adresses I2C typiques des écrans (à adapter si nécessaire)
const uint8_t OLED1_ADDRESS = 0x3C;
const uint8_t OLED2_ADDRESS = 0x3D;

// --- Configuration des Capteurs VL53L0X ---
const int xshutPins[3] = { 44, 10, 43 };
Adafruit_VL53L0X sensors[3];

// Nouvelles adresses I2C à assigner aux capteurs (différentes de 0x29)
const uint8_t sensorAddresses[3] = { 0x30, 0x31, 0x32 };

// Suit l'état d'initialisation de chaque capteur
bool sensorReady[3] = {false, false, false};

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

void setup() {
  Serial.begin(115200);
  delay(1000); // Stabilisation de l'alimentation au démarrage

  // Initialisation du bus I2C (ESP32-S3 pins par défaut ou redéfinies si besoin)
  Wire.begin();
  Wire.setClock(100000); // 100 kHz pour la stabilité du bus

  Serial.println(F("Initialisation du systeme..."));

  // 1. Scanner I2C initial (ne devrait montrer que les écrans à ce stade si XSHUT est LOW)
  scanI2C();

  // 2. Initialisation des écrans OLED
  if(!display1.begin(SSD1306_SWITCHCAPVCC, OLED1_ADDRESS)) {
    Serial.println(F("Echec initialisation OLED 1 (0x3C)"));
  } else {
    display1.clearDisplay();
    display1.setTextSize(1);
    display1.setTextColor(SSD1306_WHITE);
    display1.setCursor(0,0);
    display1.println(F("OLED 1 OK"));
    display1.display();
  }

  if(!display2.begin(SSD1306_SWITCHCAPVCC, OLED2_ADDRESS)) {
    Serial.println(F("Echec initialisation OLED 2 (0x3D)"));
  } else {
    display2.clearDisplay();
    display2.setTextSize(1);
    display2.setTextColor(SSD1306_WHITE);
    display2.setCursor(0,0);
    display2.println(F("OLED 2 OK"));
    display2.display();
  }

  // 3. Réinitialisation matérielle (Hard Reset) de tous les capteurs
  for (uint8_t i = 0; i < 3; i++) {
    pinMode(xshutPins[i], OUTPUT);
    digitalWrite(xshutPins[i], LOW);
  }
  delay(100); // Délai de décharge

  // 4. Séquence d'activation et d'adressage des capteurs
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

  // 5. Scanner I2C final pour vérifier la topologie du bus
  scanI2C();

  //while(1);
}

void loop() {
  VL53L0X_RangingMeasurementData_t measure;
  String donnees[3];

  // Lecture des 3 capteurs
  // Lecture des 3 capteurs
  for (uint8_t i = 0; i < 3; i++) {
    if (sensorReady[i]) {
      // Le capteur est fonctionnel, on interroge le bus I2C
      sensors[i].rangingTest(&measure, false);
      if (measure.RangeStatus != 4) { 
        donnees[i] = String(measure.RangeMilliMeter) + " mm";
      } else {
        donnees[i] = "Hors portee";
      }
    } else {
      // Le capteur est défaillant, on ignore la transaction I2C
      donnees[i] = "HS / Non init";
    }
    
    // Affichage sur le port série
    Serial.print(F("Capteur ")); Serial.print(i);
    Serial.print(F(" : ")); Serial.println(donnees[i]);
  }
  Serial.println(F("---"));

  // Mise à jour de l'Écran 1 (affiche Capteur 0 et 1)
  display1.clearDisplay();
  display1.setCursor(0, 0);
  display1.println(F("=== DONNEES ==="));
  display1.print(F("Capt 0: ")); display1.println(donnees[0]);
  display1.print(F("Capt 1: ")); display1.println(donnees[1]);
  display1.display();

  // Mise à jour de l'Écran 2 (affiche Capteur 2 et statut)
  display2.clearDisplay();
  display2.setCursor(0, 0);
  display2.println(F("=== SYSTEME ==="));
  display2.print(F("Capt 2: ")); display2.println(donnees[2]);
  display2.print(F("Bus I2C: OK")); 
  display2.display();

  delay(500); // Fréquence de rafraîchissement
}