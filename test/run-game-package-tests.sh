#!/usr/bin/env bash
set -euo pipefail

json_include=".pio/libdeps/esp32/ArduinoJson/src"
if [[ ! -f "$json_include/ArduinoJson.h" ]]; then
    echo "ArduinoJson is not installed; run ~/.platformio/penv/bin/pio run first." >&2
    exit 1
fi

temporary_directory="$(mktemp -d)"
trap 'rm -rf "$temporary_directory"' EXIT
fixture_directory="$temporary_directory/fixtures"
mkdir -p "$fixture_directory"
python3 test/make_game_package_fixtures.py "$fixture_directory"

sanitizer_flags=()
if [[ "${GAME_PACKAGE_SANITIZERS:-1}" != "0" ]]; then
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

package_sources=(
    firmware/game/package/game_manifest.cpp
    firmware/game/package/game_package.cpp
    firmware/game/package/package_format.cpp
    firmware/game/package/package_reader.cpp
)

g++ "${common_flags[@]}" \
    test/game_package_format_test.cpp \
    firmware/game/package/package_format.cpp \
    firmware/game/package/game_manifest.cpp \
    -o "$temporary_directory/format-tests"

g++ "${common_flags[@]}" \
    test/game_package_reader_test.cpp \
    "${package_sources[@]}" \
    -o "$temporary_directory/reader-tests"

g++ "${common_flags[@]}" \
    test/game_package_fixtures_test.cpp \
    "${package_sources[@]}" \
    -o "$temporary_directory/fixture-tests"

g++ "${common_flags[@]}" \
    test/game_library_test.cpp \
    firmware/game/library/game_library.cpp \
    "${package_sources[@]}" \
    -o "$temporary_directory/game-library-tests"

"$temporary_directory/format-tests"
"$temporary_directory/reader-tests"
"$temporary_directory/fixture-tests" "$fixture_directory"
"$temporary_directory/game-library-tests"
