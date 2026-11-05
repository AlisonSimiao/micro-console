#pragma once

#include <Arduino.h>
#include "config/config.hpp"
#include "hal/core/hal_updatable.hpp"
#include "hal/input/button.hpp"

class Input : public hal_updatable {
public:
    explicit Input(Config& config);

    bool begin() override;
    void update() override;

    bool addKey(const char* key, int pin);
    const char* const* getKeys() const;
    uint8_t getKeyCount() const;
    bool isPressed(const char* key) const;

private:
    static constexpr uint8_t MAX_BUTTONS = 8;

    Config& config;
    Button state[MAX_BUTTONS];
    const char* keys[MAX_BUTTONS];
    uint8_t buttonCount;
};
