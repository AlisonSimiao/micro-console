#pragma once

#include <cstdint>
#include "config/config.hpp"
#include "hal/core/hal_module.hpp"

class Boot {
public:
    explicit Boot(Config& config);

    bool begin();
    bool addModule(hal_module& module);
    bool addOptionalModule(hal_module& module);

private:
    static constexpr uint8_t MAX_MODULES = 8;

    struct ModuleEntry {
        hal_module* module;
        bool required;
        bool pinsClaimed;
    };

    Config& config;
    ModuleEntry modules[MAX_MODULES];
    uint8_t moduleCount;

    bool registerModule(hal_module& module, bool required);
    bool claimPins();
    bool initializeHardware();
};
