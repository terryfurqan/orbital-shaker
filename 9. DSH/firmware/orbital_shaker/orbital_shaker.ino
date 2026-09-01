/**
 * ============================================================================
 * DIY ORBITAL SHAKER FIRMWARE - REV 5.0 (DSH Agent) - TOGGLE CONTROL
 * ============================================================================
 * Hardware : Arduino Uno R3 (ATmega328P @ 16 MHz) on COM5
 * Motor    : NEMA 17 (17HS4401, 200 steps/rev @ 1/16 microstep = 3200 steps/rev)
 * Driver   : MKS TMC2209 V2.0 (StealthChop2, 24V VMOT, Vref 0.92V) - UNLOADED
 * Display  : LCD 1602 Keypad Shield (parallel 4-bit + analog buttons A0)
 *
 * CARA KERJA (goal utama):
 *  - SELECT : toggle START <-> STOP.
 *             1x tekan        -> motor berputar KONTINU & KONSTAN pada RPM
 *                                aktif (default 30 RPM) dgn ramp akselerasi
 *                                halus (10 RPM -> target, +10 RPM / 40 ms)
 *                                agar tidak stall saat start RPM tinggi.
 *             1x tekan lagi   -> STOP: driver standby (ENABLE HIGH, arus
 *                                dimatikan agar driver dingin).
 *  - UP     : +5 RPM (rentang 5 .. 300).
 *  - DOWN   : -5 RPM.
 *  - LEFT   : (debug) tampilkan nilai ADC A0 live di baris 2 LCD.
 *  - LCD    : baris 1 = "RPM:  30 RUNNING" (spinner | / - \) / "RPM:  30 STOPPED"
 *             baris 2 = "SELECT=STOP" / "SELECT=START" / "A0:xxxx TOMBOL"
 *
 * Teknik pemicu motor = DIRECT PULSE LOOP (teknik yang TERBUKTI menggerakkan
 * motor di hardware_diagnostics.ino & Rev 4.2): digitalWrite(STEP) 40 us HIGH
 * + (period-40) us LOW. Pulsa dikirim per-chunk (40 ms) lalu tombol & LCD
 * dipindai, sehingga UI tetap responsif tanpa mengubah cara pulsa dihasilkan.
 * ============================================================================
 */

#include <Arduino.h>
#include <LiquidCrystal.h>

// ----------------------------- PIN MAP -----------------------------
#define PIN_STEP        2    // TMC2209 STEP   (PORTD2)
#define PIN_DIR         3    // TMC2209 DIR    (PORTD3, HIGH = Forward)
#define PIN_ENABLE      A1   // TMC2209 ENABLE (Active LOW: LOW=ON, HIGH=Standby)
#define PIN_BUZZER      A2   // Buzzer audio
#define PIN_BTN_SHIELD  A0   // Resistor ladder buttons (analog)

#define PIN_LCD_RS      8
#define PIN_LCD_EN      9
#define PIN_LCD_D4      4
#define PIN_LCD_D5      5
#define PIN_LCD_D6      6
#define PIN_LCD_D7      7
#define PIN_LCD_BL      10

// --------------------------- MOTOR CONFIG ---------------------------
#define STEPS_PER_REV   3200UL
#define PULSE_HIGH_US   40           // lebar pulsa STEP (terbukti solid 40 us)
#define RPM_DEFAULT     30
#define RPM_STEP        5
#define RPM_MIN         5
#define RPM_MAX         300
#define RAMP_START_RPM  10   // RPM awal saat START (anti stall)
#define RAMP_STEP_RPM   10   // kenaikan RPM per chunk (40 ms) saat ramp

// ------------------- BUTTON (resistor ladder A0) --------------------
// Ambang ADC sesuai doc proyek: Select < 900, Idle > 920.
// Dead-zone 901..920 = NONE (hysteresis, anti trigger palsu & anti SELECT mati).
#define BTN_NONE        0
#define BTN_RIGHT       1
#define BTN_UP          2
#define BTN_DOWN        3
#define BTN_LEFT        4
#define BTN_SELECT      5

#define ADC_IDLE_HI     920
#define ADC_RIGHT_HI    65
#define ADC_UP_HI       220
#define ADC_DOWN_HI     420
#define ADC_LEFT_HI     680
#define ADC_SELECT_HI   900

#define DEBOUNCE_READS  3    // jumlah baca ADC identik berurutan sebelum aksi
#define ACTION_LOCKOUT  350  // ms: anti double-trigger (bouncing SELECT)

// ----------------------------- TIMING -------------------------------
#define CHUNK_MS        40    // lama motor jalan per-slice sebelum scan UI
#define LCD_REFRESH_MS  200   // periode refresh LCD
#define IDLE_SCAN_MS    15    // jeda scan tombol saat STOP

LiquidCrystal lcd(PIN_LCD_RS, PIN_LCD_EN, PIN_LCD_D4, PIN_LCD_D5, PIN_LCD_D6, PIN_LCD_D7);

// ------------------------------ STATE -------------------------------
bool motorRunning = false;
int  rpm = RPM_DEFAULT;
bool diagMode = false;
bool ramping = false;       // akselerasi awal aktif
int  effRpm = RPM_DEFAULT;  // RPM efektif selama ramp

unsigned long lastActionByBtn[6] = {0, 0, 0, 0, 0, 0}; // lockout per tombol
unsigned long lastLcdMs = 0;
uint8_t spinnerIdx = 0;
int  lastAdc = 1023;
int  lastRawBtn = BTN_NONE;

static const char SPINNER[4] = {'|', '/', '-', '\\'};

// ============================== HELPERS ==============================

void beep(int ms) {
  digitalWrite(PIN_BUZZER, HIGH);
  delay(ms);
  digitalWrite(PIN_BUZZER, LOW);
}

void setDriverEnabled(bool en) {
  digitalWrite(PIN_ENABLE, en ? LOW : HIGH);   // Active LOW
}

int readButtonRaw() {
  lastAdc = analogRead(PIN_BTN_SHIELD);
  if (lastAdc > ADC_IDLE_HI)        lastRawBtn = BTN_NONE;
  else if (lastAdc < ADC_RIGHT_HI)  lastRawBtn = BTN_RIGHT;
  else if (lastAdc < ADC_UP_HI)     lastRawBtn = BTN_UP;
  else if (lastAdc < ADC_DOWN_HI)   lastRawBtn = BTN_DOWN;
  else if (lastAdc < ADC_LEFT_HI)   lastRawBtn = BTN_LEFT;
  else if (lastAdc < ADC_SELECT_HI) lastRawBtn = BTN_SELECT;
  else                              lastRawBtn = BTN_NONE;
  return lastRawBtn;
}

// Edge-triggered debounce: hanya mengembalikan tombol saat ada transisi
// stabil baru (IDLE -> tombol). Penahanan tombol tidak pernah repeat-trigger,
// lockout 350 ms memblokir bouncing saat jari ditekan/dilepas.
int scanButton() {
  static int lastStable = BTN_NONE;
  static int candidate = BTN_NONE;
  static uint8_t count = 0;

  int raw = readButtonRaw();

  if (raw == candidate) {
    if (count < 250) count++;
  } else {
    candidate = raw;
    count = 1;
  }

  if (count >= DEBOUNCE_READS && candidate != lastStable) {
    int ev = candidate;
    int prev = lastStable;
    lastStable = candidate;
    // Hanya transisi IDLE -> tombol yang memicu aksi (edge-trigger murni).
    // Transisi lain (tombol->tombol, tombol->IDLE) hanya memperbarui lastStable.
    if (ev != BTN_NONE && prev == BTN_NONE) {
      unsigned long now = millis();
      if (now - lastActionByBtn[ev] < ACTION_LOCKOUT) return BTN_NONE;
      lastActionByBtn[ev] = now;
      return ev;
    }
  }
  return BTN_NONE;
}

// ============================== MOTOR ===============================

void stepPulse(uint32_t lowUs) {
  digitalWrite(PIN_STEP, HIGH);
  delayMicroseconds(PULSE_HIGH_US);
  digitalWrite(PIN_STEP, LOW);
  if (lowUs > 0) delayMicroseconds(lowUs);
}

// Jalankan motor selama kurang-lebih ms milidetik dengan pulsa langsung
// (teknik terbukti Rev 4.2). Selama ramp, RPM efektif (effRpm) naik bertahap
// menuju target (rpm) agar start di RPM tinggi tidak stall.
void spinChunk(uint32_t ms) {
  int runRpm = ramping ? effRpm : rpm;

  uint32_t stepsPerSec = (uint32_t)runRpm * STEPS_PER_REV / 60UL;
  if (stepsPerSec == 0) stepsPerSec = 1;
  uint32_t steps = stepsPerSec * ms / 1000UL;
  if (steps == 0) steps = 1;

  // us per step (dibulatkan; 30 RPM = 625 us, 300 RPM = 63 us)
  uint32_t periodUs = (60000000UL + (uint32_t)runRpm * STEPS_PER_REV / 2)
                      / ((uint32_t)runRpm * STEPS_PER_REV);
  uint32_t lowUs = (periodUs > PULSE_HIGH_US) ? (periodUs - PULSE_HIGH_US) : 2;

  for (uint32_t i = 0; i < steps; i++) stepPulse(lowUs);

  // Majukan ramp akselerasi satu langkah per chunk
  if (ramping) {
    effRpm += RAMP_STEP_RPM;
    if (effRpm >= rpm) { effRpm = rpm; ramping = false; }
  }
}

// ============================== LCD =================================

const char* btnName(int b) {
  switch (b) {
    case BTN_RIGHT:  return "RIGHT ";
    case BTN_UP:     return "UP    ";
    case BTN_DOWN:   return "DOWN  ";
    case BTN_LEFT:   return "LEFT  ";
    case BTN_SELECT: return "SELECT";
    default:         return "NONE  ";
  }
}

void renderLcd() {
  char line0[17];
  char line1[17];
  memset(line0, ' ', 16); line0[16] = '\0';
  memset(line1, ' ', 16); line1[16] = '\0';

  snprintf(line0, 17, "RPM: %3d %s", rpm, motorRunning ? "RUNNING" : "STOPPED");

  if (diagMode) {
    snprintf(line1, 17, "A0:%4d %s", lastAdc, btnName(lastRawBtn));
  } else if (motorRunning) {
    snprintf(line1, 17, "SELECT=STOP    %c", SPINNER[spinnerIdx]);
  } else {
    snprintf(line1, 17, "SELECT=START    ");
  }

  lcd.setCursor(0, 0); lcd.print(line0);
  lcd.setCursor(0, 1); lcd.print(line1);
}

// ============================ BUTTON ACT ============================

void handleButton(int ev) {
  switch (ev) {
    case BTN_SELECT:
      motorRunning = !motorRunning;
      if (motorRunning) {
        ramping = true;            // start dengan akselerasi halus (anti stall)
        effRpm = RAMP_START_RPM;
      } else {
        ramping = false;
      }
      setDriverEnabled(motorRunning);
      beep(motorRunning ? 60 : 100);
      Serial.print(F("[BTN] SELECT -> "));
      Serial.println(motorRunning ? F("RUNNING") : F("STOPPED"));
      renderLcd();
      break;

    case BTN_UP:
      rpm += RPM_STEP;
      if (rpm > RPM_MAX) rpm = RPM_MAX;
      beep(30);
      Serial.print(F("[BTN] UP   -> RPM ")); Serial.println(rpm);
      renderLcd();
      break;

    case BTN_DOWN:
      rpm -= RPM_STEP;
      if (rpm < RPM_MIN) rpm = RPM_MIN;
      beep(30);
      Serial.print(F("[BTN] DOWN -> RPM ")); Serial.println(rpm);
      renderLcd();
      break;

    case BTN_LEFT:
      diagMode = !diagMode;
      beep(30);
      Serial.println(diagMode ? F("[DIAG] ADC mode ON") : F("[DIAG] ADC mode OFF"));
      renderLcd();
      break;

    default:
      break;
  }
}

// ============================== SETUP ===============================

void setup() {
  // 0) Safety-critical pins PALING AWAL: driver standby sejak boot
  //    (hindari ENABLE A1 mengambang & driver nyala di 700 ms pertama)
  pinMode(PIN_ENABLE, OUTPUT);
  pinMode(PIN_DIR, OUTPUT);
  pinMode(PIN_STEP, OUTPUT);
  pinMode(PIN_BUZZER, OUTPUT);
  digitalWrite(PIN_STEP, LOW);
  digitalWrite(PIN_DIR, HIGH);      // Forward
  digitalWrite(PIN_ENABLE, HIGH);   // Standby: motor bebas, driver dingin

  Serial.begin(115200);
  delay(100);

  // LCD
  pinMode(PIN_LCD_BL, OUTPUT);
  digitalWrite(PIN_LCD_BL, HIGH);   // Backlight ON
  lcd.begin(16, 2);

  lcd.setCursor(0, 0);
  lcd.print("DIY ORBITAL SHKR");
  lcd.setCursor(0, 1);
  lcd.print("REV 5.0 READY    ");

  Serial.println(F("=============================================="));
  Serial.println(F(" DIY ORBITAL SHAKER - REV 5.0 (DSH Agent)"));
  Serial.println(F(" SELECT : START / STOP toggle"));
  Serial.println(F(" UP/DN  : +/- 5 RPM  (range 5..300)"));
  Serial.println(F(" LEFT   : ADC diagnostic mode"));
  Serial.println(F("=============================================="));

  beep(80);
  delay(600);

  renderLcd();
}

// =============================== LOOP ===============================

void loop() {
  // 1. Scan tombol (edge-trigger + debounce + lockout)
  int ev = scanButton();
  if (ev != BTN_NONE) handleButton(ev);

  unsigned long now = millis();

  if (motorRunning) {
    // 2. Refresh LCD + spinner tiap 200 ms
    if (now - lastLcdMs >= LCD_REFRESH_MS) {
      spinnerIdx = (spinnerIdx + 1) & 3;
      renderLcd();
      lastLcdMs = millis();
    }
    // 3. Kirim pulsa motor ~40 ms, lalu kembali ke scan tombol
    spinChunk(CHUNK_MS);
  } else {
    // Saat STOP: LCD refresh (biar mode diag live) + jeda singkat
    if (now - lastLcdMs >= LCD_REFRESH_MS) {
      renderLcd();
      lastLcdMs = millis();
    }
    delay(IDLE_SCAN_MS);
  }
}
