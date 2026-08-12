#!/bin/bash
set -e

if [ -z "$WOKWI_CLI_TOKEN" ]; then
    echo "Error: WOKWI_CLI_TOKEN environment variable is not set."
    echo "Please run Docker with: -e WOKWI_CLI_TOKEN=your_token"
    exit 1
fi

echo "Setting up ESP-IDF environment..."
. $IDF_PATH/export.sh

echo "Building ESP32 Wokwi benchmark..."
cd /workspace/benchmarks/esp32

# Set target to esp32 only if it hasn't been set yet
if [ ! -f sdkconfig ]; then
    idf.py set-target esp32
fi

if [ "$FASTCRON_MISRA_MODE" = "ON" ]; then
    echo "Enabling STRICT MISRA via EXTRA_CFLAGS"
    export EXTRA_CFLAGS="-DFASTCRON_STRICT_MISRA"
fi

# Build the project
idf.py build

echo "Running Wokwi CLI..."
# The installation script places wokwi-cli in ~/.wokwi/bin/wokwi-cli
~/.wokwi/bin/wokwi-cli --expect-text "--- END OF BENCHMARK ---" --timeout 600000 --serial-log-file serial.log

echo "Wokwi execution completed."
