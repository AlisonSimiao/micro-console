#pragma once

#include "boot/boot.hpp"
#include "config/config.hpp"
#include "game/library/game_library.hpp"
#include "hal/display/display.hpp"
#include "hal/input/input.hpp"
#include "hal/storage/storage.hpp"
#include "system/system.hpp"
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
    System& system();

private:
    Config config_;
    Input input_;
    Display display_;
    Storage storage_;
    GameLibrary gameLibrary_;
    System system_;
    Boot boot_;
    HomeScreen homeScreen_;
};
