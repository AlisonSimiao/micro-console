#include "boot/boot.hpp"

Boot::Boot() {}

void Boot::initializeHardware() {
   bool inputInitialized = input.begin();
}

bool Boot::begin() {
    Serial.begin(115200);
    initializeHardware();

    return true;
}