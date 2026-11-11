#pragma once

#include <cstdint>

class System {
public:
    uint32_t getUptimeMs() const;
    uint32_t getFreeHeap() const;
};
