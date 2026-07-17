#!/bin/bash
set -e

echo "Setting up ESP-IDF environment..."
. $IDF_PATH/export.sh

echo "Building ESP32 QEMU tests..."
cd /workspace/tests/esp32

# Set target to esp32 only if it hasn't been set yet
# (Running set-target touches the config and forces a full 900+ files rebuild!)
if [ ! -f sdkconfig ]; then
    idf.py set-target esp32
fi

# Build the project
idf.py build

cd build
echo "Merging binaries for QEMU..."
esptool.py --chip esp32 merge_bin --fill-flash-size 4MB -o merged.bin @flash_args

echo "Running QEMU (will timeout after 10 seconds since it's a POC)..."
# We use timeout to stop QEMU after it prints Hello World
timeout 10 qemu-system-xtensa -nographic -machine esp32 -drive file=merged.bin,if=mtd,format=raw || true
cd ..

echo "POC execution completed."
