#include "boot/boot.hpp"

Boot::Boot() {}

void Boot::initializeHardware() {
   bool inputInitialized = input.begin();
}

bool Boot::begin() {
    initializeHardware();

    return true;
}