#pragma once

#include <cstdint>

#include "hal/display/display.hpp"
#include "hal/input/input.hpp"

class HomeScreen {
public:
    HomeScreen(Display& display, Input& input);

    void begin();
    void update();
    bool isSelected(uint8_t index) const;

private:
    static constexpr uint8_t OPTION_COUNT = 3;

    void draw();

    Display& display_;
    Input& input_;
    uint8_t selectedIndex_;
};
