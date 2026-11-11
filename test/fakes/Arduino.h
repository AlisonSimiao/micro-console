#pragma once

#include <cstdint>

uint32_t millis();

class EspClass {
public:
    uint32_t getFreeHeap() const;
};

extern EspClass ESP;
