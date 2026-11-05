#pragma once

#include <cstdint>
#include "config/config.hpp"
#include "hal/core/hal_module.hpp"

class Boot {
public:
    explicit Boot(Config& config);

    bool begin();
    bool addModule(hal_module& module);

private:
    static constexpr uint8_t MAX_MODULES = 8;

    Config& config;
    hal_module* modules[MAX_MODULES];
    uint8_t moduleCount;

    bool initializeHardware();
};
