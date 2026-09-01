/**
 * ============================================================================
 * DIY ORBITAL SHAKER FIRMWARE (UNIVERSAL: LCD KEYPAD SHIELD & I2C)
 * ============================================================================
 * Project      : DIY Digital Orbital Shaker (25x30 cm table, 1.5 kg, 30-300 RPM)
 * Target MCU   : Arduino Uno R3 (ATmega328P @ 16 MHz)
 * Display      : LCD Keypad Shield HW-555 (Parallel 4-bit) / LCD 1602 I2C
 * Features     : Real-Time Digital Clock on LCD, Timer1 Step Generator, S-Curve
 *                Acceleration Ramp, Dual Mode (Continuous & Countdown Timer),
 *                EEPROM persistent memory.
 * ============================================================================
 */

#include <Arduino.h>
#include <Wire.h>
#include <EEPROM.h>
#include "config.h"

#if USE_LCD_KEYPAD_SHIELD
  #include <LiquidCrystal.h>
  LiquidCrystal lcd(PIN_SHIELD_RS, PIN_SHIELD_EN, PIN_SHIELD_D4, PIN_SHIELD_D5, PIN_SHIELD_D6, PIN_SHIELD_D7);
#else
  #include <LiquidCrystal_I2C.h>
  LiquidCrystal_I2C lcd(LCD_I2C_ADDR, LCD_COLS, LCD_ROWS);
#endif

// ============================================================================
// SYSTEM ENUMS & TYPES
// ============================================================================
enum SystemState {
  STATE_IDLE,        // Motor standby
  STATE_RAMP_UP,     // Accelerating softly
  STATE_RUNNING,     // Running at set RPM
  STATE_RAMP_DOWN,   // Decelerating softly
  STATE_PAUSED       // Paused in Timer Mode
};

enum ShakerMode {
  MODE_CONTINUOUS,   // Continuous shaking
  MODE_TIMER         // Countdown timer shaking
};

enum ShieldButton {
  BTN_NONE,
  BTN_RIGHT,
  BTN_UP,
  BTN_DOWN,
  BTN_LEFT,
  BTN_SELECT
};

// ============================================================================
// GLOBAL STATE & CLOCK VARIABLES
// ============================================================================
SystemState systemState = STATE_IDLE;
ShakerMode  shakerMode  = MODE_CONTINUOUS;

int   targetRpm       = DEFAULT_RPM;
float currentRpm      = 0.0f;
int   timerMinutes    = DEFAULT_TIMER_MINUTES;
long  remainingSeconds = 0;
long  elapsedSeconds   = 0;

// --- Real-Time Digital Clock ---
int   clockHours   = 6;
int   clockMinutes = 50;
int   clockSeconds = 0;

// Timing management
unsigned long lastRampUpdateMillis = 0;
unsigned long lastSecondTickMillis = 0;
unsigned long lastLcdUpdateMillis  = 0;
unsigned long lastBtnScanMillis    = 0;
unsigned long stopTimestampMillis  = 0;
unsigned long lastUserInteractionMillis = 0;
bool          eepromDirty = false;

// Buzzer management
unsigned long buzzerOffMillis = 0;
byte          buzzerAlarmRemaining = 0;
unsigned long buzzerNextAlarmMillis = 0;

// Animation frame
byte animFrame = 0;

// Shield Button debouncing
ShieldButton lastPressedBtn = BTN_NONE;

// ============================================================================
// HARDWARE TIMER1 STEP ENGINE (16-bit ISR)
// ============================================================================
void initTimer1() {
  noInterrupts();
  TCCR1A = 0;
  TCCR1B = 0;
  TCNT1  = 0;
  OCR1A  = 65535;
  TCCR1B |= (1 << WGM12); // CTC mode
  TCCR1B |= (1 << CS11);  // Prescaler = 8 (2 MHz clock)
  TIMSK1 &= ~(1 << OCIE1A);
  interrupts();
}

void setTimerStepFrequency(float rpm) {
  if (rpm < 1.0f) {
    TIMSK1 &= ~(1 << OCIE1A);
    return;
  }

  float stepFreq = (rpm / 60.0f) * (float)STEPS_PER_PLATFORM_REV;
  if (stepFreq < 5.0f) stepFreq = 5.0f;

  uint32_t ocr = (2000000UL / (uint32_t)stepFreq) - 1;
  if (ocr > 65535UL) ocr = 65535UL;
  if (ocr < 40UL)    ocr = 40UL;

  noInterrupts();
  OCR1A = (uint16_t)ocr;
  TIMSK1 |= (1 << OCIE1A);
  interrupts();
}

ISR(TIMER1_COMPA_vect) {
  // Pin 2 (PORTD bit 2)
  PORTD |= (1 << 2);
  __builtin_avr_delay_cycles(24); // 1.5 us pulse width
  PORTD &= ~(1 << 2);
}

// ============================================================================
// MOTOR & BUZZER CONTROLLERS
// ============================================================================
void enableMotorDriver(bool enable) {
  if (enable) {
    digitalWrite(PIN_ENABLE, LOW); // Active LOW
  } else {
    digitalWrite(PIN_ENABLE, HIGH);
  }
}

void triggerBeep(uint16_t durationMs = BUZZER_BEEP_MS) {
  digitalWrite(PIN_BUZZER, HIGH);
  buzzerOffMillis = millis() + durationMs;
}

void triggerAlarmSequence(byte beepCount = 4) {
  buzzerAlarmRemaining = beepCount;
  buzzerNextAlarmMillis = millis();
}

void updateBuzzer() {
  unsigned long now = millis();
  if (buzzerOffMillis > 0 && now >= buzzerOffMillis) {
    digitalWrite(PIN_BUZZER, LOW);
    buzzerOffMillis = 0;
  }

  if (buzzerAlarmRemaining > 0 && now >= buzzerNextAlarmMillis) {
    triggerBeep(120);
    buzzerAlarmRemaining--;
    buzzerNextAlarmMillis = now + 250;
  }
}

// ============================================================================
// CLOCK INITIALIZATION (FROM COMPILE TIME)
// ============================================================================
void initClockFromCompileTime() {
  char timeStr[] = __TIME__;
  clockHours   = (timeStr[0] - '0') * 10 + (timeStr[1] - '0');
  clockMinutes = (timeStr[3] - '0') * 10 + (timeStr[4] - '0');
  clockSeconds = (timeStr[6] - '0') * 10 + (timeStr[7] - '0');
}

// ============================================================================
// EEPROM PERSISTENCE
// ============================================================================
void loadSettingsFromEEPROM() {
  uint16_t magic = 0;
  EEPROM.get(EEPROM_ADDR_MAGIC, magic);
  if (magic == EEPROM_MAGIC_VAL) {
    EEPROM.get(EEPROM_ADDR_SAVED_RPM, targetRpm);
    EEPROM.get(EEPROM_ADDR_SAVED_TIMER, timerMinutes);
    byte modeByte = 0;
    EEPROM.get(EEPROM_ADDR_SAVED_MODE, modeByte);
    shakerMode = (modeByte == 1) ? MODE_TIMER : MODE_CONTINUOUS;

    if (targetRpm < MIN_RPM || targetRpm > MAX_RPM) targetRpm = DEFAULT_RPM;
    if (timerMinutes < 1 || timerMinutes > MAX_TIMER_MINUTES) timerMinutes = DEFAULT_TIMER_MINUTES;
  }
}

void saveSettingsToEEPROM() {
  EEPROM.put(EEPROM_ADDR_MAGIC, (uint16_t)EEPROM_MAGIC_VAL);
  EEPROM.put(EEPROM_ADDR_SAVED_RPM, targetRpm);
  EEPROM.put(EEPROM_ADDR_SAVED_TIMER, timerMinutes);
  EEPROM.put(EEPROM_ADDR_SAVED_MODE, (byte)(shakerMode == MODE_TIMER ? 1 : 0));
  eepromDirty = false;
}

void markSettingsChanged() {
  eepromDirty = true;
  lastUserInteractionMillis = millis();
}

// ============================================================================
// STATE MACHINE CONTROLS
// ============================================================================
void startShaker() {
  if (systemState == STATE_IDLE || systemState == STATE_PAUSED) {
    enableMotorDriver(true);
    digitalWrite(PIN_DIR, HIGH);
    systemState = STATE_RAMP_UP;
    if (shakerMode == MODE_TIMER && systemState != STATE_PAUSED) {
      remainingSeconds = (long)timerMinutes * 60L;
    }
    triggerBeep(100);
    Serial.println(F("[MOTOR] Shaker START"));
  }
}

void stopShaker() {
  if (systemState == STATE_RUNNING || systemState == STATE_RAMP_UP) {
    systemState = STATE_RAMP_DOWN;
    triggerBeep(100);
    Serial.println(F("[MOTOR] Shaker STOP (Ramping down)"));
  } else if (systemState == STATE_PAUSED) {
    systemState = STATE_IDLE;
    currentRpm = 0.0f;
    setTimerStepFrequency(0);
    stopTimestampMillis = millis();
    triggerBeep(100);
  }
}

// ============================================================================
// MOTION RAMP CONTROLLER
// ============================================================================
void updateRamp() {
  unsigned long now = millis();
  if (now - lastRampUpdateMillis < RAMP_INTERVAL_MS) return;
  lastRampUpdateMillis = now;

  float rampUpRate = (float)targetRpm / ((float)RAMP_UP_DURATION_MS / (float)RAMP_INTERVAL_MS);
  float rampDownRate = (float)targetRpm / ((float)RAMP_DOWN_DURATION_MS / (float)RAMP_INTERVAL_MS);
  if (rampUpRate < 0.5f) rampUpRate = 0.5f;
  if (rampDownRate < 0.5f) rampDownRate = 0.5f;

  switch (systemState) {
    case STATE_RAMP_UP:
      currentRpm += rampUpRate;
      if (currentRpm >= (float)targetRpm) {
        currentRpm = (float)targetRpm;
        systemState = STATE_RUNNING;
      }
      setTimerStepFrequency(currentRpm);
      break;

    case STATE_RUNNING:
      if (currentRpm < (float)targetRpm) {
        currentRpm += rampUpRate;
        if (currentRpm > (float)targetRpm) currentRpm = (float)targetRpm;
        setTimerStepFrequency(currentRpm);
      } else if (currentRpm > (float)targetRpm) {
        currentRpm -= rampDownRate;
        if (currentRpm < (float)targetRpm) currentRpm = (float)targetRpm;
        setTimerStepFrequency(currentRpm);
      }
      break;

    case STATE_RAMP_DOWN:
      currentRpm -= rampDownRate;
      if (currentRpm <= 0.0f) {
        currentRpm = 0.0f;
        setTimerStepFrequency(0);
        stopTimestampMillis = millis();
        systemState = (remainingSeconds > 0 && shakerMode == MODE_TIMER) ? STATE_PAUSED : STATE_IDLE;
        
        if (shakerMode == MODE_TIMER && remainingSeconds <= 0) {
          systemState = STATE_IDLE;
          triggerAlarmSequence();
        }
      } else {
        setTimerStepFrequency(currentRpm);
      }
      break;

    case STATE_IDLE:
    case STATE_PAUSED:
      if (AUTO_DISABLE_DRIVER && stopTimestampMillis > 0) {
        if (now - stopTimestampMillis >= IDLE_DISABLE_TIMEOUT_MS) {
          enableMotorDriver(false);
          stopTimestampMillis = 0;
        }
      }
      break;
  }
}

// ============================================================================
// SECOND TICK ENGINE
// ============================================================================
void updateSecondTick() {
  unsigned long now = millis();
  if (now - lastSecondTickMillis < 1000) return;
  lastSecondTickMillis = now;

  // Real-Time Clock increment
  clockSeconds++;
  if (clockSeconds >= 60) {
    clockSeconds = 0;
    clockMinutes++;
    if (clockMinutes >= 60) {
      clockMinutes = 0;
      clockHours = (clockHours + 1) % 24;
    }
  }

  // Shaker Timer update
  if (systemState == STATE_RUNNING || systemState == STATE_RAMP_UP) {
    animFrame = (animFrame + 1) % 4;

    if (shakerMode == MODE_TIMER) {
      if (remainingSeconds > 0) {
        remainingSeconds--;
        if (remainingSeconds == 0) {
          stopShaker();
        }
      }
    } else {
      elapsedSeconds++;
    }
  }

  if (eepromDirty && (now - lastUserInteractionMillis >= 5000)) {
    saveSettingsToEEPROM();
  }
}

// ============================================================================
// LCD KEYPAD SHIELD BUTTON HANDLER (A0 ANALOG RESISTOR LADDER)
// ============================================================================
#if USE_LCD_KEYPAD_SHIELD
ShieldButton readShieldButton() {
  int adc = analogRead(PIN_SHIELD_BUTTONS);
  if (adc < 60)   return BTN_RIGHT;
  if (adc < 200)  return BTN_UP;
  if (adc < 400)  return BTN_DOWN;
  if (adc < 600)  return BTN_LEFT;
  if (adc < 800)  return BTN_SELECT;
  return BTN_NONE;
}

void handleShieldButtons() {
  unsigned long now = millis();
  if (now - lastBtnScanMillis < 180) return; // 180ms debounce rate

  ShieldButton btn = readShieldButton();
  if (btn != BTN_NONE) {
    lastBtnScanMillis = now;
    markSettingsChanged();
    triggerBeep(20);

    switch (btn) {
      case BTN_UP:
        // Increase RPM
        targetRpm += RPM_STEP_SIZE;
        if (targetRpm > MAX_RPM) targetRpm = MAX_RPM;
        break;

      case BTN_DOWN:
        // Decrease RPM
        targetRpm -= RPM_STEP_SIZE;
        if (targetRpm < MIN_RPM) targetRpm = MIN_RPM;
        break;

      case BTN_SELECT:
        // START / STOP Toggle
        if (systemState == STATE_IDLE || systemState == STATE_PAUSED) {
          startShaker();
        } else {
          stopShaker();
        }
        break;

      case BTN_LEFT:
        // Toggle Continuous vs Timer Mode
        if (systemState == STATE_IDLE) {
          shakerMode = (shakerMode == MODE_CONTINUOUS) ? MODE_TIMER : MODE_CONTINUOUS;
        }
        break;

      case BTN_RIGHT:
        // Increase Timer duration (+5 minutes)
        if (systemState == STATE_IDLE) {
          timerMinutes += 5;
          if (timerMinutes > 120) timerMinutes = 5;
          shakerMode = MODE_TIMER;
        }
        break;

      default:
        break;
    }
  }
}
#endif

// ============================================================================
// SERIAL COMMAND HANDLER
// ============================================================================
void handleSerialCommands() {
  if (Serial.available()) {
    String cmd = Serial.readStringUntil('\n');
    cmd.trim();
    if (cmd.startsWith("TIME=") || cmd.startsWith("TIME:")) {
      String timePart = cmd.substring(5);
      int h = timePart.substring(0, 2).toInt();
      int m = timePart.substring(3, 5).toInt();
      int s = timePart.substring(6, 8).toInt();
      if (h >= 0 && h < 24 && m >= 0 && m < 60 && s >= 0 && s < 60) {
        clockHours = h;
        clockMinutes = m;
        clockSeconds = s;
        Serial.print(F("[SYNC] Clock set to: "));
        Serial.println(timePart);
        triggerBeep(80);
      }
    } else if (cmd.startsWith("RPM=")) {
      int r = cmd.substring(4).toInt();
      if (r >= MIN_RPM && r <= MAX_RPM) {
        targetRpm = r;
        markSettingsChanged();
      }
    } else if (cmd.equalsIgnoreCase("START")) {
      startShaker();
    } else if (cmd.equalsIgnoreCase("STOP")) {
      stopShaker();
    }
  }
}

// ============================================================================
// LCD 1602 DISPLAY RENDERING
// ============================================================================
void updateLcd() {
  unsigned long now = millis();
  if (now - lastLcdUpdateMillis < 150) return;
  lastLcdUpdateMillis = now;

  char line1[17];
  char line2[17];

  if (systemState == STATE_IDLE) {
    if (shakerMode == MODE_TIMER) {
      snprintf(line1, sizeof(line1), "RPM:%3d  TMR:%2dm", targetRpm, timerMinutes);
    } else {
      snprintf(line1, sizeof(line1), "RPM:%3d  STANDBY", targetRpm);
    }
    snprintf(line2, sizeof(line2), "JAM  %02d:%02d:%02d WIB", clockHours, clockMinutes, clockSeconds);
  } else {
    const char* stateStr = "IDLE";
    if (systemState == STATE_RAMP_UP)   stateStr = "ACC";
    if (systemState == STATE_RUNNING)   stateStr = "RUN";
    if (systemState == STATE_RAMP_DOWN) stateStr = "DEC";
    if (systemState == STATE_PAUSED)    stateStr = "PAUS";

    char animChars[] = {'|', '/', '-', '\\'};
    char spinChar = (systemState == STATE_RUNNING || systemState == STATE_RAMP_UP) ? animChars[animFrame] : ' ';

    snprintf(line1, sizeof(line1), "RPM:%3d/%3d %-4s%c", (int)(currentRpm + 0.5f), targetRpm, stateStr, spinChar);

    if (shakerMode == MODE_TIMER) {
      long sec = remainingSeconds;
      int m = sec / 60;
      int s = sec % 60;
      snprintf(line2, sizeof(line2), "%02d:%02d |TMR %02d:%02d", clockHours, clockMinutes, m, s);
    } else {
      long sec = elapsedSeconds;
      int m = (sec % 3600) / 60;
      int s = sec % 60;
      snprintf(line2, sizeof(line2), "%02d:%02d |RUN %02d:%02d", clockHours, clockMinutes, m, s);
    }
  }

  lcd.setCursor(0, 0);
  lcd.print(line1);
  lcd.setCursor(0, 1);
  lcd.print(line2);
}

// ============================================================================
// MAIN SETUP & LOOP
// ============================================================================
void setup() {
  Serial.begin(115200);
  Serial.println(F("========================================"));
  Serial.println(F("DIY Orbital Shaker Initializing"));
  Serial.println(F("========================================"));

  pinMode(PIN_STEP, OUTPUT);
  pinMode(PIN_DIR, OUTPUT);
  pinMode(PIN_ENABLE, OUTPUT);
  pinMode(PIN_BUZZER, OUTPUT);

#if USE_LCD_KEYPAD_SHIELD
  pinMode(PIN_SHIELD_BACKLIGHT, OUTPUT);
  digitalWrite(PIN_SHIELD_BACKLIGHT, HIGH);
  lcd.begin(16, 2);
#else
  Wire.begin();
  lcd.init();
  lcd.backlight();
#endif

  digitalWrite(PIN_STEP, LOW);
  digitalWrite(PIN_DIR, HIGH);
  enableMotorDriver(false);
  digitalWrite(PIN_BUZZER, LOW);

  initTimer1();
  initClockFromCompileTime();
  loadSettingsFromEEPROM();

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("DIY ORBITAL");
  lcd.setCursor(0, 1);
  lcd.print("SHAKER + CLOCK");

  triggerBeep(150);
  delay(1200);
  lcd.clear();
  Serial.println(F("System Ready."));
}

void loop() {
  handleSerialCommands();
#if USE_LCD_KEYPAD_SHIELD
  handleShieldButtons();
#endif
  updateRamp();
  updateSecondTick();
  updateBuzzer();
  updateLcd();
}
