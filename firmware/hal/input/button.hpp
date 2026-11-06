#pragma once

#include <Arduino.h>
#include "hal/core/hal_updatable.hpp"

class Button : public hal_updatable {
public:
    Button();
    Button(const char* name, uint8_t pin);

    bool begin() override;
    void update() override;

    const char* getName() const;
    uint8_t getPin() const;
    bool isPressed() const;
    bool wasPressed() const;
    bool wasReleased() const;

private:
    const char* name;
    uint8_t pin;
    bool pressed;
    bool pressedThisUpdate;
    bool releasedThisUpdate;
    bool candidatePressed;
    uint32_t candidateChangedAt;
};
