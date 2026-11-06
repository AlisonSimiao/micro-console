#include "hal/input/button.hpp"

#include <cstdio>

namespace {

constexpr uint32_t DEBOUNCE_MS = 25;

}

Button::Button()
    : name(nullptr), pin(255), pressed(false), pressedThisUpdate(false), releasedThisUpdate(false),
      candidatePressed(false), candidateChangedAt(0) {}

Button::Button(const char* name, uint8_t pin)
    : name(name), pin(pin), pressed(false), pressedThisUpdate(false), releasedThisUpdate(false),
      candidatePressed(false), candidateChangedAt(0) {}

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
    candidatePressed = digitalRead(pin) == LOW;
    candidateChangedAt = millis();
    pressed = candidatePressed;
    pressedThisUpdate = false;
    releasedThisUpdate = false;

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
    pressedThisUpdate = false;
    releasedThisUpdate = false;

    if (!isInstantiated()) {
        Serial.println("Button not initialized");
        return;
    }

    const uint32_t now = millis();
    const bool samplePressed = digitalRead(pin) == LOW;

    if (samplePressed != candidatePressed) {
        candidatePressed = samplePressed;
        candidateChangedAt = now;
    } else if (candidatePressed != pressed && now - candidateChangedAt >= DEBOUNCE_MS) {
        pressed = candidatePressed;
        pressedThisUpdate = pressed;
        releasedThisUpdate = !pressed;
    }
}

bool Button::isPressed() const
{
    return pressed;
}

bool Button::wasPressed() const
{
    return pressedThisUpdate;
}

bool Button::wasReleased() const
{
    return releasedThisUpdate;
}
