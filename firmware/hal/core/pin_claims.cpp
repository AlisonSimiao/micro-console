#include "hal/core/pin_claims.hpp"

#include <Arduino.h>

PinClaims::PinClaims() : claims{}, count(0) {}

bool PinClaims::add(int pin, const char* configKey)
{
    for (size_t i = 0; i < count; ++i) {
        if (claims[i].pin == pin) {
            Serial.print("Boot: pin conflict on GPIO ");
            Serial.print(pin);
            Serial.print(" between ");
            Serial.print(claims[i].configKey);
            Serial.print(" and ");
            Serial.println(configKey);
            return false;
        }
    }

    if (count >= MAX_CLAIMS) {
        Serial.println("Boot: maximum number of pin claims reached");
        return false;
    }

    claims[count++] = {pin, configKey};
    return true;
}
