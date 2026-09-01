#!/usr/bin/env bash
set -e
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(dirname "$SCRIPT_DIR")"
SKETCH_DIR="$ROOT_DIR/Arduino code/orbital_shaker"

echo "Compiling sketch: $SKETCH_DIR"
arduino-cli compile --fqbn arduino:avr:uno "$SKETCH_DIR"
echo "✅ Compilation successful!"
