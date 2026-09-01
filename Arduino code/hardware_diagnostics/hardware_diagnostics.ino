#include <Arduino.h>
#include <Wire.h>
#include <LiquidCrystal.h>

#define PIN_STEP        2
#define PIN_DIR         3
#define PIN_ENABLE      A1
#define PIN_BUZZER      A2
#define PIN_BTN_SHIELD  A0

#define PIN_LCD_RS      8
#define PIN_LCD_EN      9
#define PIN_LCD_D4      4
#define PIN_LCD_D5      5
#define PIN_LCD_D6      6
#define PIN_LCD_D7      7
#define PIN_LCD_BL      10

LiquidCrystal lcd(PIN_LCD_RS, PIN_LCD_EN, PIN_LCD_D4, PIN_LCD_D5, PIN_LCD_D6, PIN_LCD_D7);

bool motorTestActive = true;
int  testSpeed = 250;
unsigned long lastDiagPrint = 0;

void scanI2CBus() {
  Serial.println(F("\n--- [1] I2C BUS SCAN (A4=SDA, A5=SCL) ---"));
  byte error, address;
  int nDevices = 0;
  Wire.begin();
  for (address = 1; address < 127; address++) {
    Wire.beginTransmission(address);
    error = Wire.endTransmission();
    if (error == 0) {
      Serial.print(F(" -> Ditemukan I2C Device pada address: 0x"));
      if (address < 16) Serial.print(F("0"));
      Serial.print(address, HEX);
      if (address == 0x27 || address == 0x3F) {
        Serial.print(F(" (Modul LCD 1602 I2C Backpack PCF8574)"));
      } else if (address == 0x68) {
        Serial.print(F(" (Modul RTC DS3231 / DS1307)"));
      }
      Serial.println();
      nDevices++;
    }
  }
  if (nDevices == 0) {
    Serial.println(F(" -> Tidak ada device I2C (Normal untuk LCD Keypad Shield paralel)"));
  }
}

void scanAnalogPins() {
  Serial.println(F("\n--- [2] ANALOG PINS & BUTTON READINGS ---"));
  int a0Val = analogRead(A0);
  int a1Val = analogRead(A1);
  int a2Val = analogRead(A2);

  Serial.print(F(" Pin A0 (Shield Buttons) : RAW="));
  Serial.print(a0Val);
  Serial.print(F(" ("));
  Serial.print((float)a0Val * 5.0 / 1023.0, 2);
  Serial.print(F("V) -> "));
  if (a0Val > 950) Serial.println(F("[IDLE / TIDAK ADA TOMBOL DITEKAN]"));
  else if (a0Val < 60) Serial.println(F("[TOMBOL: RIGHT DITEKAN]"));
  else if (a0Val < 220) Serial.println(F("[TOMBOL: UP DITEKAN]"));
  else if (a0Val < 420) Serial.println(F("[TOMBOL: DOWN DITEKAN]"));
  else if (a0Val < 650) Serial.println(F("[TOMBOL: LEFT DITEKAN]"));
  else if (a0Val < 900) Serial.println(F("[TOMBOL: SELECT DITEKAN]"));
  else Serial.println(F("[TOMBOL TIDAK DIKENAL]"));

  Serial.print(F(" Pin A1 (Driver ENABLE)  : RAW="));
  Serial.print(a1Val);
  Serial.print(F(" ("));
  Serial.print((float)a1Val * 5.0 / 1023.0, 2);
  Serial.println(F("V)"));

  Serial.print(F(" Pin A2 (Buzzer)         : RAW="));
  Serial.print(a2Val);
  Serial.print(F(" ("));
  Serial.print((float)a2Val * 5.0 / 1023.0, 2);
  Serial.println(F("V)"));
}

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println(F("=================================================="));
  Serial.println(F("   FULL HARDWARE DIAGNOSTIC - ORBITAL SHAKER     "));
  Serial.println(F("=================================================="));

  pinMode(PIN_STEP, OUTPUT);
  pinMode(PIN_DIR, OUTPUT);
  pinMode(PIN_ENABLE, OUTPUT);
  pinMode(PIN_BUZZER, OUTPUT);

  // Set Driver to ACTIVE (LOW)
  digitalWrite(PIN_ENABLE, LOW);
  digitalWrite(PIN_DIR, HIGH);

  // LCD Keypad Shield
  pinMode(PIN_LCD_BL, OUTPUT);
  digitalWrite(PIN_LCD_BL, HIGH);
  lcd.begin(16, 2);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("DIAGNOSTIC MODE");
  lcd.setCursor(0, 1);
  lcd.print("DRV:ON STEP:RUN ");

  // Beep
  digitalWrite(PIN_BUZZER, HIGH);
  delay(100);
  digitalWrite(PIN_BUZZER, LOW);

  // Scans
  scanI2CBus();
  scanAnalogPins();

  Serial.println(F("\n--- [3] MOTOR DRIVER TEST STATUS ---"));
  Serial.println(F(" Pin D2 (STEP)   : PULSING (Mengirim pulsa langkah)"));
  Serial.println(F(" Pin D3 (DIR)    : HIGH (Arah Maju)"));
  Serial.println(F(" Pin A1 (ENABLE) : LOW (0V - Driver DI-PAKSA AKTIF / LOCK)"));
  Serial.println(F("Ketik SCAN untuk scan ulang, atau tekan tombol shield!"));
  Serial.println(F("=================================================="));
}

void loop() {
  if (motorTestActive) {
    digitalWrite(PIN_STEP, HIGH);
    delayMicroseconds(40);
    digitalWrite(PIN_STEP, LOW);
    delayMicroseconds(testSpeed);
  }

  if (Serial.available()) {
    String cmd = Serial.readStringUntil('\n');
    cmd.trim();
    if (cmd.equalsIgnoreCase("SCAN")) {
      scanI2CBus();
      scanAnalogPins();
    } else if (cmd.equalsIgnoreCase("EN:ON")) {
      digitalWrite(PIN_ENABLE, LOW);
      Serial.println(F("-> Driver ENABLE Dipaksa LOW (0V - ACTIVE LOCK)"));
    } else if (cmd.equalsIgnoreCase("EN:OFF")) {
      digitalWrite(PIN_ENABLE, HIGH);
      Serial.println(F("-> Driver ENABLE Dipaksa HIGH (5V - DISABLED/FREE)"));
    } else if (cmd.equalsIgnoreCase("SLOW")) {
      testSpeed = 2000;
      Serial.println(F("-> Motor Speed: SLOW (2000 us)"));
    } else if (cmd.equalsIgnoreCase("FAST")) {
      testSpeed = 250;
      Serial.println(F("-> Motor Speed: FAST (250 us)"));
    }
  }

  if (millis() - lastDiagPrint >= 1500) {
    lastDiagPrint = millis();
    int btnVal = analogRead(A0);
    if (btnVal < 950) {
      Serial.print(F("[TOMBOL TERDETEKSI] Nilai ADC A0 = "));
      Serial.println(btnVal);
    }
  }
}
