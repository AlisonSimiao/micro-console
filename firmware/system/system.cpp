#include "system/system.hpp"

#include <Arduino.h>

uint32_t System::getUptimeMs() const
{
    return millis();
}

uint32_t System::getFreeHeap() const
{
    return ESP.getFreeHeap();
}
