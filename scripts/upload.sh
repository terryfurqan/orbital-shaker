#!/usr/bin/env bash
set -e
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(dirname "$SCRIPT_DIR")"
SKETCH_DIR="$ROOT_DIR/Arduino code/orbital_shaker"
PORT="${1:-/dev/ttyACM0}"

echo "Uploading sketch to $PORT..."
arduino-cli upload -p "$PORT" --fqbn arduino:avr:uno "$SKETCH_DIR"
echo "✅ Upload complete!"
