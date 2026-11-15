#pragma once

#include <cstdint>

#include "game/library/game_library.hpp"
#include "hal/display/display.hpp"
#include "hal/input/input.hpp"

class GamesScreen {
public:
    GamesScreen(Display& display, Input& input, GameLibrary& gameLibrary);

    void begin();
    void update();
    bool isVisible() const;
    void close();

private:
    static constexpr uint8_t MAX_VISIBLE_ROWS = 5;

    void draw();

    Display& display_;
    Input& input_;
    GameLibrary& gameLibrary_;
    uint8_t selectedIndex_;
    bool visible_;
};
