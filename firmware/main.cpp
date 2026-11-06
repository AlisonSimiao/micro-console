#include "boot/boot.hpp"
#include "config/config.hpp"
#include "hal/input/input.hpp"

Config config;
Input input(config);
Boot boot(config);

[[noreturn]] void haltOnBootFailure(const char* message) {
    Serial.println(message);
    while (true) {
        delay(1000);
    }
}

void setup() {
    if (!boot.addModule(input)) {
        Serial.begin(115200);
        haltOnBootFailure("Boot failed: could not register Input module");
    }

    if (!boot.begin()) {
        haltOnBootFailure("Boot failed; system halted");
    }
}

void loop() {
    input.update();
}
