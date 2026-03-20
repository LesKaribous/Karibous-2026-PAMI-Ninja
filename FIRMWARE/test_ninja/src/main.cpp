#include <Arduino.h>
#include <Wire.h>
#include <U8g2lib.h>

#define SDA_PIN 5
#define SCL_PIN 6
#define BUZZER_PIN 1

#define I2C_ADDR_SCREEN_1 0x3C
#define I2C_ADDR_SCREEN_2 0x3D

// Type d'ecran a adapter si besoin
U8G2_SSD1306_128X64_NONAME_F_HW_I2C screen1(U8G2_R3, U8X8_PIN_NONE);
U8G2_SSD1306_128X64_NONAME_F_HW_I2C screen2(U8G2_R1, U8X8_PIN_NONE);

bool screen1Detected = false;
bool screen2Detected = false;

void initI2C();
void scanI2C();
bool isI2CDevicePresent(uint8_t address);
void initScreens();
void initBuzzer();
void drawBootScreens();
void drawScreenMessage(U8G2 &display, const char *title, const char *line1, const char *line2, int value);
void updateScreensTest();
void testBuzzer();
void beep(uint16_t frequency, uint16_t durationMs);

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println();
  Serial.println("=== Card test program ===");

  initI2C();
  scanI2C();
  initScreens();
  initBuzzer();
  drawBootScreens();

  // Petit bip de demarrage
  beep(2000, 120);
  delay(120);
  beep(3000, 120);
}

void loop() {
  updateScreensTest();
  testBuzzer();
  delay(1000);
}

void initI2C() {
  Wire.begin(SDA_PIN, SCL_PIN);
  Wire.setClock(100000);

  Serial.println("I2C initialized");
  Serial.print("SDA: ");
  Serial.println(SDA_PIN);
  Serial.print("SCL: ");
  Serial.println(SCL_PIN);
}

void scanI2C() {
  byte error;
  int count = 0;

  screen1Detected = false;
  screen2Detected = false;

  Serial.println();
  Serial.println("Scanning I2C bus...");

  for (uint8_t address = 1; address < 127; address++) {
    Wire.beginTransmission(address);
    error = Wire.endTransmission();

    if (error == 0) {
      Serial.print("I2C device found at 0x");
      if (address < 16) Serial.print("0");
      Serial.println(address, HEX);
      count++;

      if (address == I2C_ADDR_SCREEN_1) screen1Detected = true;
      if (address == I2C_ADDR_SCREEN_2) screen2Detected = true;
    }
    else if (error == 4) {
      Serial.print("Unknown error at 0x");
      if (address < 16) Serial.print("0");
      Serial.println(address, HEX);
    }
  }

  if (count == 0) {
    Serial.println("No I2C device found.");
  } else {
    Serial.println("I2C scan done.");
  }

  Serial.print("Screen 1 (0x3C): ");
  Serial.println(screen1Detected ? "detected" : "not detected");

  Serial.print("Screen 2 (0x3D): ");
  Serial.println(screen2Detected ? "detected" : "not detected");
}

bool isI2CDevicePresent(uint8_t address) {
  Wire.beginTransmission(address);
  return (Wire.endTransmission() == 0);
}

void initScreens() {
  if (screen1Detected) {
    screen1.setI2CAddress(I2C_ADDR_SCREEN_1 << 1);
    screen1.begin();
    Serial.println("Screen 1 initialized");
  }

  if (screen2Detected) {
    screen2.setI2CAddress(I2C_ADDR_SCREEN_2 << 1);
    screen2.begin();
    Serial.println("Screen 2 initialized");
  }
}

void initBuzzer() {
  pinMode(BUZZER_PIN, OUTPUT);
  noTone(BUZZER_PIN);
  Serial.print("Buzzer initialized on pin ");
  Serial.println(BUZZER_PIN);
}

void drawBootScreens() {
  if (screen1Detected) {
    drawScreenMessage(screen1, "SCREEN 1", "I2C OK", "Addr 0x3C", 0);
  }

  if (screen2Detected) {
    drawScreenMessage(screen2, "SCREEN 2", "I2C OK", "Addr 0x3D", 0);
  }
}

void drawScreenMessage(U8G2 &display, const char *title, const char *line1, const char *line2, int value) {
  display.clearBuffer();

  display.setFont(u8g2_font_6x12_tr);
  display.drawStr(0, 12, title);
  display.drawLine(0, 16, 127, 16);

  display.drawStr(0, 30, line1);
  display.drawStr(0, 44, line2);

  display.setCursor(0, 60);
  display.print("Value: ");
  display.print(value);

  display.sendBuffer();
}

void updateScreensTest() {
  static int counter = 0;
  counter++;

  if (screen1Detected) {
    drawScreenMessage(screen1, "SCREEN 1", "Display test", "Addr 0x3C", counter);
  }

  if (screen2Detected) {
    drawScreenMessage(screen2, "SCREEN 2", "Display test", "Addr 0x3D", counter);
  }

  Serial.print("Counter: ");
  Serial.println(counter);
}

void beep(uint16_t frequency, uint16_t durationMs) {
  tone(BUZZER_PIN, frequency, durationMs);
}

void testBuzzer() {
  static int step = 0;

  switch (step) {
    case 0:
      Serial.println("Buzzer test: 1000 Hz");
      beep(1000, 150);
      break;

    case 1:
      Serial.println("Buzzer test: 2000 Hz");
      beep(2000, 150);
      break;

    case 2:
      Serial.println("Buzzer test: 3000 Hz");
      beep(3000, 150);
      break;

    case 3:
      Serial.println("Buzzer test: double beep");
      beep(1500, 80);
      delay(120);
      beep(2500, 80);
      break;
  }

  step++;
  if (step > 3) {
    step = 0;
  }
}