#include "hal/input/input.hpp"

#include <cstring>

namespace {

constexpr uint8_t MAX_GPIO_PIN = 39;

struct KeyDefinition {
    const char* name;
    const char* configKey;
    int defaultPin;
};

const KeyDefinition KEY_DEFINITIONS[] = {
    {"A", "input.a", 13},
    {"B", "input.b", 14},
    {"X", "input.x", 16},
    {"Y", "input.y", 17},
    {"UP", "input.up", 18},
    {"DOWN", "input.down", 19},
    {"LEFT", "input.left", 21},
    {"RIGHT", "input.right", 22},
};

constexpr uint8_t KEY_COUNT = sizeof(KEY_DEFINITIONS) / sizeof(KEY_DEFINITIONS[0]);
static_assert(KEY_COUNT <= 8, "Input supports at most 8 buttons");

} // namespace

Input::Input(Config& config) : config(config), state{}, keys{}, buttonCount(0) {}

const char* const* Input::getKeys() const
{
    return keys;
}

uint8_t Input::getKeyCount() const
{
    return buttonCount;
}

bool Input::addKey(const char* key, int pin)
{
    if (key == nullptr) {
        Serial.println("Button name cannot be null");
        return false;
    }

    if (pin < 0 || pin > MAX_GPIO_PIN) {
        Serial.print("Invalid GPIO pin: ");
        Serial.println(pin);
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
    buttonCount = 0;

    for (uint8_t i = 0; i < KEY_COUNT; ++i) {
        const int pin = config.getInt(KEY_DEFINITIONS[i].configKey, KEY_DEFINITIONS[i].defaultPin);
        if (!addKey(KEY_DEFINITIONS[i].name, pin)) {
            Serial.print("Failed to add key ");
            Serial.println(KEY_DEFINITIONS[i].name);
            return false;
        }
    }

    for (uint8_t i = 0; i < buttonCount; ++i) {
        if (!state[i].begin()) {
            return false;
        }
    }

    setInstantiated(true);
    Serial.println("Input module initialized successfully");
    return true;
}

bool Input::claimPins(PinClaims& claims) const
{
    for (uint8_t i = 0; i < KEY_COUNT; ++i) {
        const int pin = config.getInt(KEY_DEFINITIONS[i].configKey, KEY_DEFINITIONS[i].defaultPin);
        if (!claims.add(pin, KEY_DEFINITIONS[i].configKey)) {
            return false;
        }
    }

    return true;
}

void Input::update()
{
    for (uint8_t i = 0; i < buttonCount; ++i) {
        state[i].update();
    }
}

bool Input::isPressed(const char* key) const
{
    const Button* button = findButton(key);
    return button != nullptr && button->isPressed();
}

bool Input::wasPressed(const char* key) const
{
    const Button* button = findButton(key);
    return button != nullptr && button->wasPressed();
}

bool Input::wasReleased(const char* key) const
{
    const Button* button = findButton(key);
    return button != nullptr && button->wasReleased();
}

const Button* Input::findButton(const char* key) const
{
    if (key == nullptr) {
        return nullptr;
    }

    for (uint8_t i = 0; i < buttonCount; ++i) {
        if (strcmp(state[i].getName(), key) == 0) {
            return &state[i];
        }
    }

    return nullptr;
}
