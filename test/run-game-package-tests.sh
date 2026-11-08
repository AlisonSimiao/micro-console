#!/usr/bin/env bash
set -euo pipefail

json_include=".pio/libdeps/esp32/ArduinoJson/src"
if [[ ! -f "$json_include/ArduinoJson.h" ]]; then
    echo "ArduinoJson is not installed; run ~/.platformio/penv/bin/pio run first." >&2
    exit 1
fi

binary="$(mktemp)"
trap 'rm -f "$binary"' EXIT

g++ -std=c++11 \
    -Ifirmware \
    -I"$json_include" \
    test/game_package_format_test.cpp \
    firmware/game/package/package_format.cpp \
    firmware/game/package/game_manifest.cpp \
    -o "$binary"
"$binary"
