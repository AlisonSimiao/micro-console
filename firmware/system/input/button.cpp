#include <stdio.h>
#include <Arduino.h>
#include "button.hpp"

Button::Button()
{
    this->name = nullptr;
    this->pin = 255;
    this->pressed = false;
};

Button::Button(const char *name, uint8_t pin)
{
    this->name = name;
    this->pin = pin;
    this->pressed = false;
}

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

    this->setInstanciated(true);
    snprintf(buffer, sizeof(buffer), "Button: %s on pin %d initialized", name, pin);
    Serial.println(buffer);
    return true;
};

const char * Button::getName() const { return name; };

uint8_t Button::getPin() const { return pin; };

void Button::update()
{
    if (!this->isInstanciated()) {
        Serial.println("Button not initialized");
        return;
    }

    this->pressed = digitalRead(pin) == LOW;
};

bool Button::isPressed() const
{
    return this->pressed;
};