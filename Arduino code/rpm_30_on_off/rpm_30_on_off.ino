/**
 * ============================================================================
 * DIY ORBITAL SHAKER FIRMWARE - REV 4.2 DIRECT HARDWARE PULSE ENGINE (15 SEC)
 * ============================================================================
 * Hardware Target : Arduino Uno R3 (ATmega328P @ 16 MHz) on COM5
 * Motor           : NEMA 17 (17HS4401, 200 steps/rev, 1/16 microstepping = 3200 steps/rev)
 * Driver          : TMC2209 SilentStepStick (StealthChop2, 24V VMOT, Vref ~0.92V)
 * Interface       : LCD 1602 Keypad Shield (HW-555 / DFR0009 Parallel + Analog Buttons)
 * 
 * Hardware Pinout:
 *  - Pin D2  : STEP   (TMC2209 Step Pulse, 40 us Solid HIGH Pulse)
 *  - Pin D3  : DIR    (TMC2209 Direction Output, HIGH = Forward)
 *  - Pin A1  : ENABLE (TMC2209 Driver Enable: Active LOW, 0V=ON/Lock, 5V=Standby/Free)
 *  - Pin A2  : BUZZER (Audio Feedback)
 *  - Pin A0  : BUTTONS (Resistor ladder analog input)
 *  - Pin D4-D10: LCD 1602 parallel bus + Backlight
 * ============================================================================
 */

#include <Arduino.h>
#include <LiquidCrystal.h>

// --- PIN DEFINITIONS ---
#define PIN_STEP        2   // PORTD2 (TMC2209 STEP)
#define PIN_DIR         3   // PORTD3 (TMC2209 DIR)
#define PIN_ENABLE      A1  // PORTC1 (TMC2209 ENABLE - Active LOW)
#define PIN_BUZZER      A2  // PORTC2 (Buzzer)
#define PIN_BTN_SHIELD  A0  // Analog Button Ladder

#define PIN_LCD_RS      8
#define PIN_LCD_EN      9
#define PIN_LCD_D4      4
#define PIN_LCD_D5      5
#define PIN_LCD_D6      6
#define PIN_LCD_D7      7
#define PIN_LCD_BL      10

// --- MOTOR CONFIGURATION ---
// 30 RPM = 0.5 rev/sec * 3200 steps/rev = 1600 steps/sec (1600 Hz)
// Period per step = 1,000,000 / 1600 = 625 microseconds
// Pulse HIGH = 40 us, Pulse LOW = 585 us -> Total = 625 us
#define PULSE_HIGH_US   40
#define PULSE_LOW_US    585

// --- LCD INITIALIZATION ---
LiquidCrystal lcd(PIN_LCD_RS, PIN_LCD_EN, PIN_LCD_D4, PIN_LCD_D5, PIN_LCD_D6, PIN_LCD_D7);

void playBeep(int durationMs) {
  digitalWrite(PIN_BUZZER, HIGH);
  delay(durationMs);
  digitalWrite(PIN_BUZZER, LOW);
}

void setup() {
  Serial.begin(115200);
  delay(100);

  // 1. Configure Hardware GPIOs
  pinMode(PIN_STEP, OUTPUT);
  pinMode(PIN_DIR, OUTPUT);
  pinMode(PIN_ENABLE, OUTPUT);
  pinMode(PIN_BUZZER, OUTPUT);
  pinMode(PIN_BTN_SHIELD, INPUT);

  // 2. Initialize LCD 1602 Keypad Shield
  pinMode(PIN_LCD_BL, OUTPUT);
  digitalWrite(PIN_LCD_BL, HIGH); // Backlight ON
  lcd.begin(16, 2);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("DIY ORBITAL SHKR");
  lcd.setCursor(0, 1);
  lcd.print("STARTING 30 RPM ");

  Serial.println(F("=================================================="));
  Serial.println(F(" DIY ORBITAL SHAKER - REV 4.2 DIRECT PULSE TEST  "));
  Serial.println(F("=================================================="));
  Serial.println(F(" STEP PIN : Pin D2 (40 us HIGH / 585 us LOW)     "));
  Serial.println(F(" DIR PIN  : Pin D3 (HIGH)                         "));
  Serial.println(F(" EN PIN   : Pin A1 (LOW - Driver Active 0V)       "));
  Serial.println(F(" SPEED    : 30 RPM (1600 Hz Constant)             "));
  Serial.println(F(" DURATION : 15 Detik Penuh (24,000 Steps = 7.5 Rev)"));
  Serial.println(F("=================================================="));

  playBeep(80);
  delay(500);

  // 3. ACTIVATE DRIVER
  digitalWrite(PIN_DIR, HIGH);
  digitalWrite(PIN_ENABLE, LOW); // Active LOW -> 0V = ON
  delay(50); // Wake up driver

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("RPM: 30  RUNNING");
  lcd.setCursor(0, 1);
  lcd.print("AUTO-RUN 15 SEC ");

  Serial.println(F("[MOTOR] >>> DIRECT SPINNING STARTED (15 SECONDS) <<<"));

  // 4. DIRECT CONTINUOUS STEPPING: 24,000 steps @ 1600 Hz = EXACTLY 15.0 SECONDS
  // Update countdown every 1600 steps (every 1.0 second)
  for (int sec = 15; sec >= 1; sec--) {
    // Print countdown to LCD
    lcd.setCursor(9, 1);
    if (sec < 10) lcd.print(" ");
    lcd.print(sec);
    lcd.print("s ");

    // Step for 1.0 second (1600 steps)
    for (int step = 0; step < 1600; step++) {
      digitalWrite(PIN_STEP, HIGH);
      delayMicroseconds(PULSE_HIGH_US);
      digitalWrite(PIN_STEP, LOW);
      delayMicroseconds(PULSE_LOW_US);
    }
  }

  // 5. STOP MOTOR AFTER 15 SECONDS
  digitalWrite(PIN_ENABLE, HIGH); // Standby (5V = Driver Disabled / Cool)
  playBeep(150);

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("RPM: 30  STOPPED");
  lcd.setCursor(0, 1);
  lcd.print("15s TEST DONE!  ");

  Serial.println(F("[MOTOR] >>> 15 SECONDS COMPLETED: MOTOR STOPPED (STANDBY) <<<"));
  Serial.println(F("Tekan tombol Reset (RST) pada Arduino untuk mengulang 15 detik!"));
}

void loop() {
  // Live button ADC diagnostic in standby
  static unsigned long lastDiag = 0;
  if (millis() - lastDiag >= 200) {
    lastDiag = millis();
    int adc = analogRead(PIN_BTN_SHIELD);

    char buf[17];
    const char* btn = "NONE  ";
    if (adc < 65)  btn = "RIGHT ";
    else if (adc < 220) btn = "UP    ";
    else if (adc < 420) btn = "DOWN  ";
    else if (adc < 680) btn = "LEFT  ";
    else if (adc < 920) btn = "SELECT";

    snprintf(buf, sizeof(buf), "A0:%4d %s", adc, btn);
    lcd.setCursor(0, 1);
    lcd.print(buf);

    if (adc < 950) {
      Serial.print(F("[ADC READ] A0 = "));
      Serial.print(adc);
      Serial.print(F(" -> "));
      Serial.println(btn);
    }
  }
}
