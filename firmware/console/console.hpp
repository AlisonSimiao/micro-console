#pragma once

#include "boot/boot.hpp"
#include "config/config.hpp"
#include "game/library/game_library.hpp"
#include "hal/display/display.hpp"
#include "hal/input/input.hpp"
#include "hal/storage/storage.hpp"
#include "ui/screens/home_screen.hpp"

class Console {
public:
    Console();

    bool begin();
    void update();

    Input& input();
    Display& display();
    Storage& storage();
    GameLibrary& gameLibrary();

private:
    Config config_;
    Input input_;
    Display display_;
    Storage storage_;
    GameLibrary gameLibrary_;
    Boot boot_;
    HomeScreen homeScreen_;
};
