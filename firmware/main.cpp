#include <Arduino.h>

#include "console/console.hpp"

Console console;

void setup()
{
    if (!console.begin()) {
        Serial.begin(115200);
        Serial.println("Console: boot failed; system halted");

        while (true) {
            delay(1000);
        }
    }
}

void loop()
{
    console.update();
}
