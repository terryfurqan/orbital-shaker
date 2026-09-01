/**
 * ============================================================================
 * DIY ORBITAL SHAKER - UJI PUTARAN 360 DERAJAT HALUS (SMOOTH TRAPEZOIDAL RAMP)
 * ============================================================================
 * Sketsa uji presisi untuk memutar motor NEMA 17 tepat 1 putaran penuh (360 derajat)
 * dengan pergerakan sehalus sutra (silky smooth) tanpa getaran "brrrp".
 *
 * Rahasia Kehalusan:
 * 1. ZERO I/O JITTER: Tidak ada panggilan lcd.print() atau Serial.print() di dalam
 *    loop pulsa langkah. Hal ini mencegah jeda 2 milidetik yang merusak StealthChop2.
 * 2. TRAPEZOIDAL ACCELERATION RAMP:
 *    - Ramp Up (800 langkah / 90 deg): Meluncur lembut dari 7.5 RPM ke 30 RPM.
 *    - Cruise (1600 langkah / 180 deg): Kecepatan konstan stabil 30 RPM.
 *    - Ramp Down (800 langkah / 90 deg): Melambat mulus dari 30 RPM ke 7.5 RPM lalu berhenti.
 *    Total = Tepat 3.200 microsteps = Tepat 360.0 derajat.
 * ============================================================================
 */

#include <Arduino.h>
#include <LiquidCrystal.h>

// --- PIN HARDWARE ---
#define PIN_STEP        2   // Sinyal pulsa ke TMC2209
#define PIN_DIR         3   // Sinyal arah (HIGH = Maju)
#define PIN_ENABLE      A1  // Sinyal daya (Active LOW: LOW = ON, HIGH = Standby)
#define PIN_BUZZER      A2  // Indikator audio
#define PIN_BTN_SHIELD  A0  // Resistor ladder tombol Keypad Shield

// Pin LCD 1602 Keypad Shield
#define PIN_LCD_RS      8
#define PIN_LCD_EN      9
#define PIN_LCD_D4      4
#define PIN_LCD_D5      5
#define PIN_LCD_D6      6
#define PIN_LCD_D7      7
#define PIN_LCD_BL      10

LiquidCrystal lcd(PIN_LCD_RS, PIN_LCD_EN, PIN_LCD_D4, PIN_LCD_D5, PIN_LCD_D6, PIN_LCD_D7);

// --- KONFIGURASI PROFIL GERAK 360 DERAJAT ---
#define TOTAL_STEPS     3200UL // 1 putaran penuh pada 1/16 microstep (360 derajat)
#define RAMP_STEPS      800UL  // 90 derajat akselerasi & 90 derajat deselerasi
#define CRUISE_STEPS    1600UL // 180 derajat kecepatan jelajah

#define PULSE_HIGH_US   20     // Lebar pulsa HIGH yang solid & efisien

// Timing Pulsa (Mikrodetik):
// 7.5 RPM  -> Periode 2500 us (Start lembut)
// 30.0 RPM -> Periode 625 us  (Cruise kecepatan normal)
#define START_PERIOD_US   2500UL
#define CRUISE_PERIOD_US  625UL

void playBeep(int ms) {
  digitalWrite(PIN_BUZZER, HIGH);
  delay(ms);
  digitalWrite(PIN_BUZZER, LOW);
}

void setup() {
  Serial.begin(115200);

  // 1. Inisialisasi pin kendali
  pinMode(PIN_STEP, OUTPUT);
  pinMode(PIN_DIR, OUTPUT);
  pinMode(PIN_ENABLE, OUTPUT);
  pinMode(PIN_BUZZER, OUTPUT);
  pinMode(PIN_BTN_SHIELD, INPUT);

  // Set kondisi awal aman: motor lemas, driver dingin
  digitalWrite(PIN_STEP, LOW);
  digitalWrite(PIN_DIR, HIGH);
  digitalWrite(PIN_ENABLE, HIGH);

  // 2. Inisialisasi LCD
  pinMode(PIN_LCD_BL, OUTPUT);
  digitalWrite(PIN_LCD_BL, HIGH);
  lcd.begin(16, 2);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("360 DEG SMOOTH  ");
  lcd.setCursor(0, 1);
  lcd.print("TEKAN SELECT    ");

  Serial.println(F("=================================================="));
  Serial.println(F(" DIY ORBITAL SHAKER - UJI 360 DERAJAT HALUS (RAMP)"));
  Serial.println(F("=================================================="));
  Serial.println(F(" Tekan SELECT untuk memutar 360 derajat halus...  "));

  playBeep(80);
}

void rotate360Smooth() {
  Serial.println(F("[START] Memulai putaran 360 derajat (Trapezoidal Ramp)..."));
  playBeep(50);

  // 1. Perbarui tampilan SEBELUM motor bergerak (agar tidak mengganggu pulsa)
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("PUTAR 360 DERAJAT");
  lcd.setCursor(0, 1);
  lcd.print("RAMP->CRUISE->STP");

  // 2. Aktifkan driver (Active LOW)
  digitalWrite(PIN_ENABLE, LOW);
  delay(60); // Waktu stabilisasi charge pump TMC2209

  // 3. EKSEKUSI 3.200 LANGKAH DENGAN TIMING MURNI TANPA INTERUPSI
  //    Fase 1: Ramp Up (0 s.d. 799)
  //    Fase 2: Cruise  (800 s.d. 2399)
  //    Fase 3: Ramp Down (2400 s.d. 3199)
  for (uint32_t i = 0; i < TOTAL_STEPS; i++) {
    uint32_t periodUs;

    if (i < RAMP_STEPS) {
      // Akselerasi: periode turun dari 2500 us ke 625 us
      uint32_t delta = (START_PERIOD_US - CRUISE_PERIOD_US) * i / RAMP_STEPS;
      periodUs = START_PERIOD_US - delta;
    } else if (i < (RAMP_STEPS + CRUISE_STEPS)) {
      // Kecepatan jelajah stabil: 625 us
      periodUs = CRUISE_PERIOD_US;
    } else {
      // Deselerasi: periode naik dari 625 us kembali ke 2500 us
      uint32_t stepInDecel = i - (RAMP_STEPS + CRUISE_STEPS);
      uint32_t delta = (START_PERIOD_US - CRUISE_PERIOD_US) * stepInDecel / RAMP_STEPS;
      periodUs = CRUISE_PERIOD_US + delta;
    }

    uint32_t lowUs = (periodUs > PULSE_HIGH_US) ? (periodUs - PULSE_HIGH_US) : 2;

    // Tembakan pulsa murni
    digitalWrite(PIN_STEP, HIGH);
    delayMicroseconds(PULSE_HIGH_US);
    digitalWrite(PIN_STEP, LOW);
    delayMicroseconds(lowUs);
  }

  // 4. Matikan kran daya motor seketika setelah 360 derajat selesai
  digitalWrite(PIN_ENABLE, HIGH);

  // 5. Perbarui tampilan SETELAH motor selesai bergerak
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("360 DEG SELESAI!");
  lcd.setCursor(0, 1);
  lcd.print("TEKAN SELECT LG ");

  Serial.println(F("[DONE] 3.200 microstep selesai. Tepat 1 putaran penuh (360 derajat)."));
  Serial.println(F("Driver kembali STANDBY (dingin).\n"));

  playBeep(80);
  delay(120);
  playBeep(80);
}

void loop() {
  int adc = analogRead(PIN_BTN_SHIELD);

  // Tombol SELECT (ADC ~650 - 900)
  if (adc > 650 && adc < 900) {
    // Tunggu sampai tombol benar-benar dilepas (anti-bouncing total)
    while (analogRead(PIN_BTN_SHIELD) < 920) {
      delay(10);
    }
    delay(50);

    rotate360Smooth();
  }

  // Serial listener: kirim 'g' atau spasi dari Serial Monitor
  if (Serial.available() > 0) {
    char c = Serial.read();
    if (c == 'g' || c == 'G' || c == ' ') {
      rotate360Smooth();
    }
  }

  delay(20);
}
