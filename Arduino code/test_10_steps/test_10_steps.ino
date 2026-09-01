/**
 * ============================================================================
 * DIY ORBITAL SHAKER - PENGUJIAN 10 LANGKAH (TEST 10 FULL STEPS)
 * ============================================================================
 * Sketsa pengujian paling dasar, murni, dan transparan.
 * Tanpa interupsi timer, tanpa akselerasi rumit, tanpa multi-tasking.
 *
 * Tujuan:
 * 1. Menunjukkan secara kasat mata dan terdengar jelas bagaimana motor NEMA 17
 *    melangkah persis 1 langkah demi 1 langkah hingga total 10 langkah (18 derajat).
 * 2. Membuktikan bahwa driver TMC2209 dan motor bekerja 100% normal.
 * 
 * Pengaturan:
 * - Driver TMC2209 diset microstepping 1/16 (1 putaran penuh = 3200 pulsa).
 * - 1 Full Step mekanik = 1.8 derajat = 16 pulsa microstep.
 * - 10 Full Step = 10 x 16 pulsa = 160 pulsa = 18 derajat (jelas terlihat mata).
 * ============================================================================
 */

#include <Arduino.h>
#include <LiquidCrystal.h>

// --- PIN HARDWARE ---
#define PIN_STEP        2   // Sinyal detak langkah ke TMC2209
#define PIN_DIR         3   // Sinyal arah putaran (HIGH = Maju / Searah jarum jam)
#define PIN_ENABLE      A1  // Sinyal kran daya (Active LOW: LOW = ON/Kunci, HIGH = Standby/Bebas)
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

void playBeep(int durationMs) {
  digitalWrite(PIN_BUZZER, HIGH);
  delay(durationMs);
  digitalWrite(PIN_BUZZER, LOW);
}

void setup() {
  Serial.begin(115200);

  // 1. Inisialisasi pin kendali motor
  pinMode(PIN_STEP, OUTPUT);
  pinMode(PIN_DIR, OUTPUT);
  pinMode(PIN_ENABLE, OUTPUT);
  pinMode(PIN_BUZZER, OUTPUT);
  pinMode(PIN_BTN_SHIELD, INPUT);

  // Set kondisi awal yang AMAN:
  digitalWrite(PIN_STEP, LOW);
  digitalWrite(PIN_DIR, HIGH);     // Arah maju
  digitalWrite(PIN_ENABLE, HIGH);  // Driver Standby (motor lemas, driver dingin)

  // 2. Inisialisasi LCD
  pinMode(PIN_LCD_BL, OUTPUT);
  digitalWrite(PIN_LCD_BL, HIGH);  // Lampu latar LCD ON
  lcd.begin(16, 2);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("UJI 10 LANGKAH ");
  lcd.setCursor(0, 1);
  lcd.print("TEKAN SELECT    ");

  Serial.println(F("=================================================="));
  Serial.println(F(" UJI DASAR NEMA 17: 10 FULL STEPS (18 DERAJAT)    "));
  Serial.println(F("=================================================="));
  Serial.println(F("Silakan tekan tombol SELECT pada shield untuk mulai..."));

  playBeep(80);
}

// Fungsi mengirim 1 pulsa microstep ke driver
void sendSinglePulse() {
  digitalWrite(PIN_STEP, HIGH);
  delayMicroseconds(40);           // Pulsa HIGH 40 mikrodetik
  digitalWrite(PIN_STEP, LOW);
  delayMicroseconds(585);          // Jeda LOW 585 mikrodetik
}

// Fungsi mengeksekusi 1 Full Step (16 microstep = 1.8 derajat)
void moveOneFullStep() {
  for (int p = 0; p < 16; p++) {
    sendSinglePulse();
  }
}

void runTenSteps() {
  Serial.println(F("\n[AKSI] Memulai 10 langkah bertahap..."));
  playBeep(60);

  // 1. Buka kran daya motor (Active LOW -> 0V = ON)
  digitalWrite(PIN_ENABLE, LOW);
  delay(50); // Jeda sejenak agar driver siap dan menahan torsi

  // 2. Lakukan 10 langkah satu demi satu
  for (int step = 1; step <= 10; step++) {
    // Tampilkan di LCD
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("MELANGKAH...");
    lcd.setCursor(0, 1);
    lcd.print("LANGKAH: ");
    lcd.print(step);
    lcd.print(" / 10");

    Serial.print(F("Langkah "));
    Serial.print(step);
    Serial.println(F(" / 10 (+1.8 derajat)"));

    // Gerakkan motor 1 full step (16 microstep)
    moveOneFullStep();

    // Bunyikan beep klik sangat pendek agar gerakan terdengar sinkron
    playBeep(15);

    // JEDA 600 MILIDETIK: Mata bisa melihat dan menghitung tiap langkah!
    delay(600);
  }

  // 3. Matikan kran daya motor (HIGH = Standby, motor lemas, driver dingin)
  digitalWrite(PIN_ENABLE, HIGH);

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("SELESAI 10 STEP!");
  lcd.setCursor(0, 1);
  lcd.print("TEKAN SELECT LG ");

  Serial.println(F("[SELESAI] 10 langkah (total 18 derajat) berhasil dieksekusi."));
  Serial.println(F("Driver kembali standby (dingin). Tekan SELECT untuk mengulang.\n"));

  playBeep(100);
  delay(100);
  playBeep(100);
}

void loop() {
  int adc = analogRead(PIN_BTN_SHIELD);

  // Ambang tombol SELECT pada LCD Keypad Shield (biasanya di kisaran 650 - 900)
  if (adc > 650 && adc < 900) {
    // Tunggu sampai tombol dilepas (anti-bouncing & anti multi-trigger)
    while (analogRead(PIN_BTN_SHIELD) < 920) {
      delay(10);
    }
    delay(50); // Debounce delay

    runTenSteps();
  }

  // Debug Serial sederhana jika ada perintah kirim karakter 'g' dari PC
  if (Serial.available() > 0) {
    char c = Serial.read();
    if (c == 'g' || c == 'G') {
      runTenSteps();
    }
  }

  delay(20);
}
