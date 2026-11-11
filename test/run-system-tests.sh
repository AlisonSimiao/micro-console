#!/usr/bin/env bash
set -euo pipefail

temporary_directory="$(mktemp -d)"
trap 'rm -rf "$temporary_directory"' EXIT

g++ \
    -std=c++11 \
    -Wall \
    -Wextra \
    -Werror \
    -Itest/fakes \
    -Ifirmware \
    test/system_test.cpp \
    firmware/system/system.cpp \
    -o "$temporary_directory/system-tests"

"$temporary_directory/system-tests"
