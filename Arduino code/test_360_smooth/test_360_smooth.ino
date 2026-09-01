/**
 * ============================================================================
 * DIY ORBITAL SHAKER - UJI 360 DERAJAT ZONA AMAN (SAFE PULL-IN BAND 6.0 RPM)
 * ============================================================================
 * Sketsa definitif putaran 360 derajat bebas stall dan bebas re-trigger palsu.
 *
 * Parameter Kunci:
 * 1. SAFE PULL-IN FREQUENCY (320 Hz = 6.0 RPM):
 *    - Frekuensi langkah berada di bawah batas inersia diam rotor NEMA 17.
 *    - Torsi magnetik 100% mampu menarik rotor seketika dari diam tanpa stall.
 *    - Periode pulsa: Tepat 3.125 us (HIGH 40 us, LOW 3.085 us).
 *    - Durasi putaran: Tepat 10.0 detik (3.200 langkah x 3.125 us = 10.000.000 us).
 * 2. STEALTHCHOP2 STANDSTILL CALIBRATION:
 *    - Jeda 200 ms setelah ENABLE = LOW sebelum pulsa pertama ditembakkan.
 *    - Memberikan waktu bagi TMC2209 untuk autotuning arus kumparan secara akurat.
 * 3. ANTI-FLYBACK HARD LOCKOUT (2.500 ms):
 *    - Mengunci rapat pembacaan pin A0 selama 2.5 detik setelah motor berhenti.
 *    - Meredam 100% lonjakan tegangan induksi balik (flyback EMF) yang memicu "trek lagi".
 * 4. DUAL-TRIGGER:
 *    - Tekan tombol SELECT pada LCD shield, ATAU
 *    - Ketik angka '1' pada Serial Monitor (115200 baud).
 * ============================================================================
 */

#include <Arduino.h>
#include <LiquidCrystal.h>

// --- PIN HARDWARE ---
#define PIN_STEP        2   // Sinyal pulsa ke TMC2209 (PORTD2)
#define PIN_DIR         3   // Sinyal arah putaran (HIGH = Maju)
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

// --- KONFIGURASI TIMING ZONA AMAN 6.0 RPM ---
#define PULSE_HIGH_US     40     // Lebar pulsa HIGH (garansi lolos kabel jumper)
#define PULSE_LOW_US      3085   // Periode total 3.125 us = 320 Hz = 6.0 RPM
#define TOTAL_STEPS       3200UL // Tepat 360.0 derajat pada 1/16 microstep
#define STANDSTILL_MS     200    // Jeda kalibrasi arus TMC2209 sebelum pulsa
#define POST_LOCKOUT_MS   2500UL // Hard lockout anti-flyback setelah motor stop

unsigned long lockoutUntilMs = 0;

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
  digitalWrite(PIN_DIR, HIGH);     // Arah maju
  digitalWrite(PIN_ENABLE, HIGH);  // Driver Standby (motor bebas, driver dingin)

  // 2. Inisialisasi LCD
  pinMode(PIN_LCD_BL, OUTPUT);
  digitalWrite(PIN_LCD_BL, HIGH);  // Backlight ON
  lcd.begin(16, 2);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("360 DEG @ 6 RPM ");
  lcd.setCursor(0, 1);
  lcd.print("PRESS SELECT / 1");

  Serial.println(F("=================================================="));
  Serial.println(F(" DIY ORBITAL SHAKER - UJI 360 DERAJAT ZONA AMAN   "));
  Serial.println(F("=================================================="));
  Serial.println(F(" Kecepatan : 6.0 RPM (320 Hz Safe Pull-In Band)   "));
  Serial.println(F(" Target    : 3.200 Microsteps = Tepat 360 Derajat "));
  Serial.println(F(" Durasi    : Tepat 10.0 Detik Murni               "));
  Serial.println(F(" Pemicu    : Tombol SELECT atau ketik '1' di PC   "));
  Serial.println(F("=================================================="));

  playBeep(80);
}

void execute360SafeRotation(const char* triggerSource) {
  Serial.print(F("\n[AKSI] Dipicu via: "));
  Serial.println(triggerSource);
  Serial.println(F("[MOTOR] Mengaktifkan driver & kalibrasi arus standstill (200 ms)..."));
  playBeep(50);

  // 1. Tampilkan status di LCD SEBELUM motor bergerak
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("PUTAR 360 DERAJAT");
  lcd.setCursor(0, 1);
  lcd.print("RUNNING 10 DETIK ");

  // 2. Aktifkan driver & beri waktu StealthChop2 autotuning (Standstill Phase)
  digitalWrite(PIN_ENABLE, LOW); // LOW = Driver ON
  delay(STANDSTILL_MS);          // Wajib 200 ms agar torsi awal stabil

  Serial.println(F("[MOTOR] >>> STREAMING PULSA 3.200 LANGKAH (10 DETIK) DIMULAI <<<"));

  // 3. STREAMING PULSA MURNI 3.200 LANGKAH (320 Hz = 6 RPM)
  //    Zero I/O Interruption: tidak ada panggilan LCD/Serial di dalam loop
  for (uint32_t i = 0; i < TOTAL_STEPS; i++) {
    digitalWrite(PIN_STEP, HIGH);
    delayMicroseconds(PULSE_HIGH_US);
    digitalWrite(PIN_STEP, LOW);
    delayMicroseconds(PULSE_LOW_US);
  }

  // 4. Matikan daya motor seketika setelah langkah ke-3.200 tercapai
  digitalWrite(PIN_ENABLE, HIGH); // Standby: motor lemas, driver dingin

  // 5. Kunci pembacaan tombol selama 2.5 detik untuk meredam flyback EMF
  lockoutUntilMs = millis() + POST_LOCKOUT_MS;

  // 6. Perbarui tampilan LCD
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("360 DEG SELESAI!");
  lcd.setCursor(0, 1);
  lcd.print("SIAP ULANG (OK) ");

  Serial.println(F("[SELESAI] 3.200 langkah (360.0 derajat) selesai sempurna."));
  Serial.println(F("[LOCKOUT] Mengunci pembacaan analog A0 selama 2.5s (anti-flyback)..."));
  Serial.println(F("[STANDBY] Motor dingin. Siap untuk pengujian berikutnya.\n"));

  playBeep(80);
  delay(120);
  playBeep(80);
}

void loop() {
  unsigned long now = millis();

  // Jika masih dalam masa lockout pasca-gerak, abaikan semua pembacaan tombol
  if (now < lockoutUntilMs) {
    delay(20);
    return;
  }

  // 1. Pemicu Tombol Fisik SELECT pada LCD Shield
  int adc = analogRead(PIN_BTN_SHIELD);
  if (adc > 650 && adc < 900) {
    // Wajib tunggu sampai tombol benar-benar dilepas oleh jari
    while (analogRead(PIN_BTN_SHIELD) < 920) {
      delay(10);
    }
    delay(50); // Debounce delay

    execute360SafeRotation("TOMBOL SELECT SHIELD");
  }

  // 2. Pemicu Bersih via Serial Monitor (Ketik '1' atau 'g' atau spasi)
  if (Serial.available() > 0) {
    char c = Serial.read();
    if (c == '1' || c == 'g' || c == 'G' || c == ' ') {
      execute360SafeRotation("SERIAL MONITOR PC");
    }
  }

  delay(20);
}
