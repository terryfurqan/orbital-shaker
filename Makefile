# DIY Orbital Shaker - Arduino CLI Makefile
SKETCH_DIR = Arduino\ code/orbital_shaker
FQBN = arduino:avr:uno
PORT ?= /dev/ttyACM0
BAUD ?= 115200

.PHONY: all compile upload monitor deps list-boards clean

all: compile

deps:
	@echo "Installing Arduino AVR core and required libraries..."
	arduino-cli core update-index
	arduino-cli core install arduino:avr
	arduino-cli lib install "LiquidCrystal"
	arduino-cli lib install "LiquidCrystal I2C"
	arduino-cli lib install "Keypad"
	arduino-cli lib install "AccelStepper"

compile:
	@echo "Compiling sketch in $(SKETCH_DIR) for $(FQBN)..."
	arduino-cli compile --fqbn $(FQBN) "$(SKETCH_DIR)"

upload:
	@echo "Uploading to $(PORT)..."
	arduino-cli upload -p $(PORT) --fqbn $(FQBN) "$(SKETCH_DIR)"

monitor:
	@echo "Opening Serial Monitor on $(PORT) at $(BAUD) baud..."
	arduino-cli monitor -p $(PORT) -c baudrate=$(BAUD)

list-boards:
	@echo "Detecting connected boards..."
	arduino-cli board list

clean:
	@echo "Cleaning build cache..."
	arduino-cli cache clean
