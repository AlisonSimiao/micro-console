#include "hal/updatable.hpp"
#include <Arduino.h>

class Button: public hal_updatable {
    private:
        const char* name;
        uint8_t pin;
        bool pressed;
    public:
        Button();
        Button(const char* name, uint8_t pin);
        bool begin();
        const char* getName() const;
        uint8_t getPin() const;
        void update();
        bool isPressed() const;
};