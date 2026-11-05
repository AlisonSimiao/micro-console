#include "Arduino.h"
#include "input.hpp"

enum { PIN_A = 13, PIN_B = 14, PIN_X = 16, PIN_Y = 17,
       PIN_UP = 18, PIN_DOWN = 19, PIN_LEFT = 21, PIN_RIGHT = 22 };

Input::Input() : state{}, keys{}, buttonCount(0) {}

const char* const* Input::getKeys() const
{
    return keys;
}

uint8_t Input::getKeyCount() const
{
    return buttonCount;
}

bool Input::addKey(const char *key, int pin)
{
    if (key == nullptr) {
        Serial.println("Button name cannot be null");
        return false;
    }

    if (pin < 0 || pin >= 255) {
        Serial.println("Pin number must be between 0 and 254");
        return false;
    }

    if (buttonCount >= MAX_BUTTONS) {
        Serial.println("Maximum number of buttons reached");
        return false;
    }

    for (uint8_t i = 0; i < buttonCount; ++i) {
        if (state[i].getPin() == pin) {
            Serial.println("Pin number already in use");
            return false;
        }
    }

    state[buttonCount] = Button(key, static_cast<uint8_t>(pin));
    keys[buttonCount] = key;
    ++buttonCount;
    return true;
}

bool Input::begin()
{
    Serial.println("Initializing Input module...");

    if (!addKey("A", PIN_A)) {
        Serial.println("Failed to add key A");
        return false;
    }
    if (!addKey("B", PIN_B)){
        Serial.println("Failed to add key B");
        return false;
    }
    if (!addKey("X", PIN_X)){
        Serial.println("Failed to add key X");
        return false;
    }
    if (!addKey("Y", PIN_Y)){
        Serial.println("Failed to add key Y");
        return false;
    }
    if (!addKey("UP", PIN_UP)){
        Serial.println("Failed to add key UP");
        return false;
    }
    if (!addKey("DOWN", PIN_DOWN)){
        Serial.println("Failed to add key DOWN");
        return false;
    }
    if (!addKey("LEFT", PIN_LEFT)){
        Serial.println("Failed to add key LEFT");
        return false;
    }
    if (!addKey("RIGHT", PIN_RIGHT)){
        Serial.println("Failed to add key RIGHT");
        return false;
    }

    for (uint8_t i = 0; i < buttonCount; ++i) {
        if (!state[i].begin())
            return false;
    }
    
    Serial.println("Input module initialized successfully");

    return true;
}

bool Input::isPressed(char* key) const
{
    for (uint8_t i = 0; i < buttonCount; ++i) {
        if (strcmp(state[i].getName(), key) == 0) {
            return state[i].isPressed();
        }
    }

    Serial.print("Key not found: ");
    Serial.println(key);
    return false;
}