#!/usr/bin/env bash
set -euo pipefail

PIO="$HOME/.platformio/penv/bin/pio"
ESPTOOL="$HOME/.platformio/packages/tool-esptool/esptool"
BUILD_DIR=".pio/build/esp32"

"$PIO" run
"$PIO" run -t buildfs

"$ESPTOOL" --chip esp32 merge-bin \
  -o "$BUILD_DIR/merged.bin" --flash-size 4MB --fill-flash-size 4MB \
  0x1000 "$BUILD_DIR/bootloader.bin" \
  0x8000 "$BUILD_DIR/partitions.bin" \
  0xe000 "$HOME/.platformio/packages/framework-arduinoespressif32/tools/partitions/boot_app0.bin" \
  0x10000 "$BUILD_DIR/firmware.bin" \
  0x290000 "$BUILD_DIR/littlefs.bin"