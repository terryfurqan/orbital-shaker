/**
 * ============================================================================
 * DIY ORBITAL SHAKER - PENGUJIAN 1000 LANGKAH (TEST 1000 STEPS STREAM)
 * ============================================================================
 * Sketsa uji transparan untuk mengeksekusi 1000 langkah secara presisi.
 *
 * Mode Uji:
 * - SELECT  : Eksekusi 1.000 Pulsa Microstep (112.5 derajat = 0.3125 putaran)
 * - UP      : Eksekusi 1.000 Full Steps (16.000 pulsa = 5.0 putaran penuh)
 *
 * Karakteristik:
 * - Anti-bouncing fisik murni (wajib tunggu lepas tombol).
 * - Live progress counter di LCD 1602.
 * - Driver otomatis standby (dingin) seketika langkah ke-1000 tercapai.
 * ============================================================================
 */

#include <Arduino.h>
#include <LiquidCrystal.h>

// --- PIN HARDWARE ---
#define PIN_STEP        2   // Sinyal pulsa ke TMC2209
#define PIN_DIR         3   // Sinyal arah putaran (HIGH = Forward)
#define PIN_ENABLE      A1  // Sinyal kran daya (LOW = ON, HIGH = Standby)
#define PIN_BUZZER      A2  // Indikator audio
#define PIN_BTN_SHIELD  A0  // Resistor ladder tombol Keypad Shield

// Pin LCD 1602
#define PIN_LCD_RS      8
#define PIN_LCD_EN      9
#define PIN_LCD_D4      4
#define PIN_LCD_D5      5
#define PIN_LCD_D6      6
#define PIN_LCD_D7      7
#define PIN_LCD_BL      10

LiquidCrystal lcd(PIN_LCD_RS, PIN_LCD_EN, PIN_LCD_D4, PIN_LCD_D5, PIN_LCD_D6, PIN_LCD_D7);

void playBeep(int ms) {
  digitalWrite(PIN_BUZZER, HIGH);
  delay(ms);
  digitalWrite(PIN_BUZZER, LOW);
}

void setup() {
  Serial.begin(115200);

  // 1. Konfigurasi GPIO
  pinMode(PIN_STEP, OUTPUT);
  pinMode(PIN_DIR, OUTPUT);
  pinMode(PIN_ENABLE, OUTPUT);
  pinMode(PIN_BUZZER, OUTPUT);
  pinMode(PIN_BTN_SHIELD, INPUT);

  // Kondisi awal aman (driver lemas & dingin)
  digitalWrite(PIN_STEP, LOW);
  digitalWrite(PIN_DIR, HIGH);
  digitalWrite(PIN_ENABLE, HIGH);

  // 2. Konfigurasi LCD
  pinMode(PIN_LCD_BL, OUTPUT);
  digitalWrite(PIN_LCD_BL, HIGH);
  lcd.begin(16, 2);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("UJI 1000 LANGKAH");
  lcd.setCursor(0, 1);
  lcd.print("SEL=112  UP=5REV");

  Serial.println(F("=================================================="));
  Serial.println(F(" DIY ORBITAL SHAKER - PENGUJIAN 1000 LANGKAH      "));
  Serial.println(F("=================================================="));
  Serial.println(F(" Tekan SELECT : 1.000 Pulsa Microstep (112.5 deg) "));
  Serial.println(F(" Tekan UP     : 1.000 Full Steps / 5 Putaran (1800 deg)"));
  Serial.println(F("=================================================="));

  playBeep(80);
}

// Eksekusi sejumlah pulsa dengan pembaruan progress ke LCD & Serial
void streamPulses(uint32_t totalPulses, const char* label, float expectedDegrees) {
  playBeep(60);

  // 1. Aktifkan driver (LOW = ON)
  digitalWrite(PIN_ENABLE, LOW);
  delay(50); // Jeda stabilisasi daya

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(label);

  Serial.print(F("[START] Mengirim "));
  Serial.print(totalPulses);
  Serial.print(F(" pulsa (Target: "));
  Serial.print(expectedDegrees, 1);
  Serial.println(F(" derajat)..."));

  // Periode langkah: 625 mikrodetik per pulsa (kecepatan stabil 30 RPM = 1600 Hz)
  // HIGH 40 us, LOW 585 us
  uint32_t progressInterval = totalPulses / 10;
  if (progressInterval == 0) progressInterval = 100;

  for (uint32_t i = 1; i <= totalPulses; i++) {
    digitalWrite(PIN_STEP, HIGH);
    delayMicroseconds(40);
    digitalWrite(PIN_STEP, LOW);
    delayMicroseconds(585);

    // Update progress display berkala (tiap 10%)
    if (i % progressInterval == 0 || i == totalPulses) {
      int pct = (int)((i * 100UL) / totalPulses);
      char buf[17];
      snprintf(buf, sizeof(buf), "STEP:%5lu (%3d%%)", (unsigned long)i, pct);
      lcd.setCursor(0, 1);
      lcd.print(buf);

      Serial.print(F(" -> Progres: "));
      Serial.print(i);
      Serial.print(F(" / "));
      Serial.print(totalPulses);
      Serial.print(F(" pulsa ("));
      Serial.print(pct);
      Serial.println(F("%)"));
    }
  }

  // 2. Matikan driver setelah target selesai (HIGH = Standby)
  digitalWrite(PIN_ENABLE, HIGH);

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("SELESAI 1000 STP");
  lcd.setCursor(0, 1);
  char resBuf[17];
  snprintf(resBuf, sizeof(resBuf), "ROTASI: %4.1f DEG", expectedDegrees);
  lcd.print(resBuf);

  Serial.println(F("[DONE] Target langkah selesai. Driver kembali STANDBY (dingin).\n"));

  playBeep(80);
  delay(100);
  playBeep(80);
}

void loop() {
  int adc = analogRead(PIN_BTN_SHIELD);

  // 1. Tombol SELECT ditekan (ADC ~650 - 900)
  if (adc > 650 && adc < 900) {
    // Tunggu sampai tombol dilepas (anti-bouncing murni)
    while (analogRead(PIN_BTN_SHIELD) < 920) { delay(10); }
    delay(50);

    // 1000 pulsa microstep = 1000 / 3200 * 360 = 112.5 derajat (~0.3125 putaran)
    // Selesai dalam waktu ~0.625 detik
    streamPulses(1000, "1000 uSTEP PULSE", 112.5);
  }

  // 2. Tombol UP ditekan (ADC ~100 - 250)
  else if (adc > 100 && adc < 250) {
    // Tunggu sampai tombol dilepas
    while (analogRead(PIN_BTN_SHIELD) < 920) { delay(10); }
    delay(50);

    // 1000 FULL STEPS = 1000 x 16 microsteps = 16.000 pulsa = 5 putaran penuh (1800 derajat)
    // Selesai dalam waktu 10.0 detik pada 30 RPM
    streamPulses(16000, "1000 FULL STEPS ", 1800.0);
  }

  // Serial listener: kirim 's' untuk 1000 microstep, kirim 'f' untuk 1000 full steps
  if (Serial.available() > 0) {
    char c = Serial.read();
    if (c == 's' || c == 'S') {
      streamPulses(1000, "1000 uSTEP PULSE", 112.5);
    } else if (c == 'f' || c == 'F') {
      streamPulses(16000, "1000 FULL STEPS ", 1800.0);
    }
  }

  delay(20);
}
