#pragma once

#include "boot/boot.hpp"
#include "config/config.hpp"
#include "hal/display/display.hpp"
#include "hal/input/input.hpp"

class Console {
public:
    Console();

    bool begin();
    void update();

    Input& input();
    Display& display();

private:
    void drawTestScreen();

    Config config_;
    Input input_;
    Display display_;
    Boot boot_;
};
