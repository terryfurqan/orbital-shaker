/**
 * ============================================================================
 * DIY ORBITAL SHAKER FIRMWARE - CONFIGURATION
 * ============================================================================
 * Project      : DIY Digital Orbital Shaker (25x30 cm, 1.5 kg, 30-300 RPM)
 * Target Board : Arduino Uno R3 (ATmega328P @ 16 MHz)
 * ============================================================================
 */

#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// ============================================================================
// 1. HARDWARE DISPLAY CONFIGURATION
// ============================================================================
// Set to TRUE if using LCD Keypad Shield HW-555 (yang sedang terpasang di Arduino)
// Set to FALSE jika nanti beralih ke LCD I2C 4-pin (SDA/SCL) + CNC Shield V3
#define USE_LCD_KEYPAD_SHIELD     true

// --- LCD Keypad Shield Pinout (HW-555 / DFR0009) ---
#define PIN_SHIELD_RS             8
#define PIN_SHIELD_EN             9
#define PIN_SHIELD_D4             4
#define PIN_SHIELD_D5             5
#define PIN_SHIELD_D6             6
#define PIN_SHIELD_D7             7
#define PIN_SHIELD_BACKLIGHT      10
#define PIN_SHIELD_BUTTONS        A0

// --- I2C LCD Pinout (Ketika menggunakan I2C Backpack 4-pin) ---
#define LCD_I2C_ADDR              0x27  // atau 0x3F
#define LCD_COLS                  16
#define LCD_ROWS                  2

// ============================================================================
// 2. STEPPER MOTOR & MOTION (TMC2209 / CNC Shield V3)
// ============================================================================
#if USE_LCD_KEYPAD_SHIELD
  // Saat LCD Keypad Shield terpasang, pin 4,5,6,7,8,9,10,A0 dipakai oleh Shield LCD.
  // Pin stepper & kontrol dialihkan ke pin bebas (D2, D3, A1-A5):
  #define PIN_STEP                2   // Pulse Output (Pin D2 - PORTD bit 2)
  #define PIN_DIR                 3   // Direction Output (Pin D3)
  #define PIN_ENABLE              A1  // Active LOW Enable (Pin Analog A1)
  #define PIN_BUZZER              A2  // Audio feedback buzzer (Pin Analog A2)
#else
  // Standar CNC Shield V3
  #define PIN_STEP                2   // X.STEP
  #define PIN_DIR                 5   // X.DIR
  #define PIN_ENABLE              8   // CNC Shield EN
  #define PIN_BUZZER              A2  // Buzzer
#endif

// ============================================================================
// 3. KINEMATICS & MOTOR CONFIGURATION
// ============================================================================
#define MOTOR_FULL_STEPS_PER_REV  200     // 1.8 deg NEMA 17 (17HS4401)
#define MICROSTEPPING             16      // 1/16 Microstepping on TMC2209
#define PULLEY_MOTOR_TEETH        20      // 20T GT2 Pulley
#define PULLEY_PLATFORM_TEETH     20      // 20T GT2 Pulley (1:1 Ratio)

#define STEPS_PER_PLATFORM_REV    ((uint32_t)MOTOR_FULL_STEPS_PER_REV * MICROSTEPPING * PULLEY_PLATFORM_TEETH / PULLEY_MOTOR_TEETH)

// ============================================================================
// 4. MOTION & SPEED LIMITS (RPM)
// ============================================================================
#define MIN_RPM                   30      // RPM Minimum
#define MAX_RPM                   300     // RPM Maksimum
#define DEFAULT_RPM               120     // Default RPM awal
#define RPM_STEP_SIZE             5       // Kelipatan RPM tiap klik tombol

// ============================================================================
// 5. ACCELERATION RAMP PROFILES
// ============================================================================
#define RAMP_UP_DURATION_MS       2000    // Akselerasi lembut 2 detik
#define RAMP_DOWN_DURATION_MS     1500    // Deselerasi lembut 1.5 detik
#define RAMP_INTERVAL_MS          20      // Update kalkulasi ramp 50 Hz

#define AUTO_DISABLE_DRIVER       true
#define IDLE_DISABLE_TIMEOUT_MS   3000    // Matikan arus driver setelah 3 detik diam

// ============================================================================
// 6. TIMER & EEPROM
// ============================================================================
#define DEFAULT_TIMER_MINUTES     15
#define MAX_TIMER_MINUTES         999
#define BUZZER_BEEP_MS            80

#define EEPROM_MAGIC_VAL          0x4F53
#define EEPROM_ADDR_MAGIC         0
#define EEPROM_ADDR_SAVED_RPM     2
#define EEPROM_ADDR_SAVED_TIMER   4
#define EEPROM_ADDR_SAVED_MODE    6

#endif // CONFIG_H
