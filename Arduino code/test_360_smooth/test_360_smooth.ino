/**
 * ============================================================================
 * DIY ORBITAL SHAKER - UJI PUTARAN 360 DERAJAT MURNI (PROVEN 40us/585us ENGINE)
 * ============================================================================
 * Sketsa perbaikan murni 360 derajat (3.200 microstep).
 *
 * Menggunakan timing yang SUDAH TERBUKTI BERHASIL memutar motor pada uji 1000 step:
 * - PULSE_HIGH_US : 40 mikrodetik (TIDAK BOLEH dikurangi karena redaman kabel)
 * - PULSE_LOW_US  : 585 mikrodetik (Kecepatan teruji 30 RPM = 1600 Hz)
 * - TOTAL_STEPS   : 3.200 pulsa = Tepat 360.0 derajat (1 putaran penuh)
 * - DURASI GERAK  : Tepat 2.0 detik penuh (3200 x 625 us)
 * - ZERO JITTER   : LCD hanya diupdate SEBELUM dan SESUDAH putaran selesai.
 *                   TIDAK ADA interupsi lcd.print() di tengah jalan!
 * ============================================================================
 */

#include <Arduino.h>
#include <LiquidCrystal.h>

// --- PIN HARDWARE ---
#define PIN_STEP        2   // Sinyal pulsa ke TMC2209 (PORTD2)
#define PIN_DIR         3   // Sinyal arah putaran (HIGH = Maju)
#define PIN_ENABLE      A1  // Sinyal kran daya (Active LOW: LOW = ON, HIGH = Standby)
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

// Parameter pulsa yang TERBUKTI bekerja pada hardware ini
#define PULSE_HIGH_US   40
#define PULSE_LOW_US    585
#define TOTAL_STEPS     3200UL // 1 putaran penuh pada 1/16 microstep (360 derajat)

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
  lcd.print("360 DEG PUTARAN ");
  lcd.setCursor(0, 1);
  lcd.print("TEKAN SELECT    ");

  Serial.println(F("=================================================="));
  Serial.println(F(" DIY ORBITAL SHAKER - UJI 360 DERAJAT MURNI       "));
  Serial.println(F("=================================================="));
  Serial.println(F(" Pulsa: 40 us HIGH / 585 us LOW (30 RPM Teruji)   "));
  Serial.println(F(" Total: 3.200 Microsteps = 1 Putaran Penuh (360°) "));
  Serial.println(F(" Durasi: 2.0 Detik Murni (Zero LCD Interruption)  "));
  Serial.println(F("=================================================="));

  playBeep(80);
}

void rotate360Clean() {
  Serial.println(F("\n[AKSI] Memulai putaran 360 derajat murni (3.200 step)..."));
  playBeep(40);

  // 1. Tampilkan status di LCD SEBELUM motor bergerak
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("BERPUTAR 360 DEG");
  lcd.setCursor(0, 1);
  lcd.print("3200 STEP (2.0s)");

  // 2. Buka kran daya motor (LOW = Driver ON)
  digitalWrite(PIN_ENABLE, LOW);
  delay(50); // Waktu bangun driver TMC2209

  // 3. STREAMING PULSA MURNI 3.200 LANGKAH TANPA GANGGUAN I/O LCD/SERIAL
  for (uint32_t i = 0; i < TOTAL_STEPS; i++) {
    digitalWrite(PIN_STEP, HIGH);
    delayMicroseconds(PULSE_HIGH_US);
    digitalWrite(PIN_STEP, LOW);
    delayMicroseconds(PULSE_LOW_US);
  }

  // 4. Matikan kran daya seketika setelah step 3.200 selesai
  digitalWrite(PIN_ENABLE, HIGH); // Standby (motor lemas, driver dingin)

  // 5. Tampilkan status SELESAI di LCD setelah motor berhenti
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("360 DEG SELESAI!");
  lcd.setCursor(0, 1);
  lcd.print("TEKAN SELECT LG ");

  Serial.println(F("[SELESAI] 3.200 langkah selesai sempurna (360 derajat)."));
  Serial.println(F("Driver kembali standby (dingin). Tekan SELECT untuk mengulang.\n"));

  playBeep(80);
}

void loop() {
  int adc = analogRead(PIN_BTN_SHIELD);

  // Tombol SELECT (ADC ~650 - 900)
  if (adc > 650 && adc < 900) {
    // Tunggu sampai tombol benar-benar dilepas oleh jari
    while (analogRead(PIN_BTN_SHIELD) < 920) {
      delay(10);
    }
    delay(50); // Debounce jeda

    rotate360Clean();
  }

  // Serial listener: kirim 'g' dari serial monitor jika ingin memicu via PC
  if (Serial.available() > 0) {
    char c = Serial.read();
    if (c == 'g' || c == 'G' || c == ' ') {
      rotate360Clean();
    }
  }

  delay(20);
}
