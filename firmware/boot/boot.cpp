#include "boot/boot.hpp"

#include <Arduino.h>

Boot::Boot(Config& config) : config(config), modules{}, moduleCount(0) {}

bool Boot::addModule(hal_module& module)
{
    if (moduleCount >= MAX_MODULES) {
        Serial.println("Boot: maximum number of modules reached");
        return false;
    }

    modules[moduleCount++] = &module;
    return true;
}

bool Boot::claimPins()
{
    PinClaims claims;
    for (uint8_t i = 0; i < moduleCount; ++i) {
        if (!modules[i]->claimPins(claims)) {
            return false;
        }
    }

    return true;
}

bool Boot::initializeHardware()
{
    for (uint8_t i = 0; i < moduleCount; ++i) {
        if (!modules[i]->begin()) {
            Serial.print("Boot: module failed to initialize: ");
            Serial.println(i);
            return false;
        }
    }

    return true;
}

bool Boot::begin()
{
    Serial.begin(115200);
    Serial.println("Boot: starting");

    if (!config.begin()) {
        Serial.println("Boot: config initialization failed");
        return false;
    }

    if (!claimPins()) {
        Serial.println("Boot: pin validation failed");
        return false;
    }

    if (!initializeHardware()) {
        Serial.println("Boot: hardware initialization failed");
        return false;
    }

    Serial.println("Boot: ready");
    return true;
}
