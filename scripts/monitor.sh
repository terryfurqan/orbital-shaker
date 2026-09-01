#!/usr/bin/env bash
set -e
PORT="${1:-/dev/ttyACM0}"
BAUD="${2:-115200}"

echo "Starting Serial Monitor on $PORT at $BAUD baud (Press Ctrl+C to exit)..."
arduino-cli monitor -p "$PORT" -c baudrate="$BAUD"
