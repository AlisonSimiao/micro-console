#include "hal/input/button.hpp"

#include <cstdio>

Button::Button() : name(nullptr), pin(255), pressed(false) {}

Button::Button(const char* name, uint8_t pin) : name(name), pin(pin), pressed(false) {}

bool Button::begin()
{
    char buffer[50];
    snprintf(buffer, sizeof(buffer), "Initializing button: %s on pin %d", name, pin);
    Serial.println(buffer);

    if (pin == 255) {
        Serial.println("Invalid pin number");
        return false;
    }

    pinMode(pin, INPUT_PULLUP);

    setInstantiated(true);
    snprintf(buffer, sizeof(buffer), "Button: %s on pin %d initialized", name, pin);
    Serial.println(buffer);
    return true;
}

const char* Button::getName() const
{
    return name;
}

uint8_t Button::getPin() const
{
    return pin;
}

void Button::update()
{
    if (!isInstantiated()) {
        Serial.println("Button not initialized");
        return;
    }

    pressed = digitalRead(pin) == LOW;
}

bool Button::isPressed() const
{
    return pressed;
}
