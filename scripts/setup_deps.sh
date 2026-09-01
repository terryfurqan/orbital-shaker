#!/usr/bin/env bash
set -e
echo "Updating core index..."
arduino-cli core update-index
echo "Installing Arduino AVR core..."
arduino-cli core install arduino:avr
echo "Installing required libraries..."
arduino-cli lib install "LiquidCrystal"
arduino-cli lib install "LiquidCrystal I2C"
arduino-cli lib install "Keypad"
arduino-cli lib install "AccelStepper"
echo "✅ All dependencies installed successfully!"
