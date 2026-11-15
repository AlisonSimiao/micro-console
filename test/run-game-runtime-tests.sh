#!/usr/bin/env bash
set -euo pipefail

json_include=".pio/libdeps/esp32/ArduinoJson/src"
if [[ ! -f "$json_include/ArduinoJson.h" ]]; then
    echo "ArduinoJson is not installed; run ~/.platformio/penv/bin/pio run first." >&2
    exit 1
fi

temporary_directory="$(mktemp -d)"
trap 'rm -rf "$temporary_directory"' EXIT

sanitizer_flags=()
if [[ "${GAME_RUNTIME_SANITIZERS:-1}" != "0" ]]; then
    sanitizer_flags=(-fsanitize=address,undefined -fno-omit-frame-pointer)
fi

common_flags=(
    -std=c++11
    -g
    -O1
    -Wall
    -Wextra
    -Werror
    -Wno-maybe-uninitialized
    "${sanitizer_flags[@]}"
    -Itest/fakes
    -Ifirmware
    -I.
    -I"$json_include"
)

runtime_sources=(
    firmware/game/runtime/game_runtime.cpp
    firmware/game/package/game_manifest.cpp
    firmware/game/package/game_package.cpp
    firmware/game/package/package_format.cpp
    firmware/game/package/package_reader.cpp
)

g++ "${common_flags[@]}" \
    test/game_runtime_test.cpp \
    "${runtime_sources[@]}" \
    -o "$temporary_directory/runtime-tests"

"$temporary_directory/runtime-tests"
