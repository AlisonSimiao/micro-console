#include "boot/boot.hpp"
#include "config/config.hpp"
#include "hal/input/input.hpp"

Config config;
Input input(config);
Boot boot(config);

void setup() {
    boot.addModule(input);
    boot.begin();
}

void loop() {
    input.update();
}
