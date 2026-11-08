#include "boot/boot.hpp"

#include <Arduino.h>

Boot::Boot(Config& config) : config(config), modules{}, moduleCount(0) {}

bool Boot::addModule(hal_module& module)
{
    return registerModule(module, true);
}

bool Boot::addOptionalModule(hal_module& module)
{
    return registerModule(module, false);
}

bool Boot::registerModule(hal_module& module, bool required)
{
    if (moduleCount >= MAX_MODULES) {
        Serial.println("Boot: maximum number of modules reached");
        return false;
    }

    modules[moduleCount++] = {&module, required, true};
    return true;
}

bool Boot::claimPins()
{
    PinClaims claims;

    for (uint8_t i = 0; i < moduleCount; ++i) {
        modules[i].pinsClaimed = true;
    }

    // Required hardware gets priority over optional modules when pins conflict.
    for (uint8_t pass = 0; pass < 2; ++pass) {
        const bool required = pass == 0;
        for (uint8_t i = 0; i < moduleCount; ++i) {
            ModuleEntry& entry = modules[i];
            if (entry.required != required) {
                continue;
            }

            PinClaims candidate = claims;
            if (!entry.module->claimPins(candidate)) {
                if (entry.required) {
                    return false;
                }

                entry.pinsClaimed = false;
                entry.module->setInstantiated(false);
                Serial.print("Boot: optional module skipped after pin validation failure: ");
                Serial.println(i);
                continue;
            }

            claims = candidate;
        }
    }

    return true;
}

bool Boot::initializeHardware()
{
    for (uint8_t i = 0; i < moduleCount; ++i) {
        ModuleEntry& entry = modules[i];
        if (!entry.pinsClaimed) {
            continue;
        }

        if (!entry.module->begin()) {
            if (!entry.required) {
                entry.module->setInstantiated(false);
                Serial.print("Boot: optional module unavailable: ");
                Serial.println(i);
                continue;
            }

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
