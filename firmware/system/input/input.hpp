#include <Arduino.h>
#include "button.hpp"
#include "hal/module.hpp"

class Input: hal_module {
    private:
        enum { MAX_BUTTONS = 8 };
        Button state[MAX_BUTTONS];
        const char* keys[MAX_BUTTONS];
        uint8_t buttonCount;
    public:
        Input();
        bool addKey(const char* key, int pin);
        const char* const* getKeys() const;
        uint8_t getKeyCount() const;
        bool begin() override;
        bool isPressed(char* key) const;
};