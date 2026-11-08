#pragma once

#include "boot/boot.hpp"
#include "config/config.hpp"
#include "hal/display/display.hpp"
#include "hal/input/input.hpp"
#include "ui/screens/home_screen.hpp"

class Console {
public:
    Console();

    bool begin();
    void update();

    Input& input();
    Display& display();

private:
    Config config_;
    Input input_;
    Display display_;
    Boot boot_;
    HomeScreen homeScreen_;
};
