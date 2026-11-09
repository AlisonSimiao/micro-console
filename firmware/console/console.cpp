#include "console/console.hpp"

#include <Arduino.h>

Console::Console()
    : config_(),
      input_(config_),
      display_(config_),
      storage_(config_),
      gameLibrary_(storage_),
      boot_(config_),
      homeScreen_(display_, input_)
{
}

bool Console::begin()
{
    if (!boot_.addModule(display_)) {
        Serial.begin(115200);
        Serial.println("Console: could not register Display module");
        return false;
    }

    if (!boot_.addModule(input_)) {
        Serial.begin(115200);
        Serial.println("Console: could not register Input module");
        return false;
    }

    if (!boot_.addOptionalModule(storage_)) {
        Serial.begin(115200);
        Serial.println("Console: could not register optional Storage module");
    }

    if (!boot_.begin()) {
        return false;
    }

    if (storage_.isMounted()) {
        if (gameLibrary_.scan()) {
            Serial.print("Console: game library found ");
            Serial.print(gameLibrary_.count());
            Serial.println(" game(s)");
        } else {
            Serial.println("Console: game library directory unavailable");
        }
    }

    homeScreen_.begin();

    Serial.print("Console: home screen ready ");
    Serial.print(display_.width());
    Serial.print("x");
    Serial.println(display_.height());

    return true;
}

void Console::update()
{
    input_.update();
    homeScreen_.update();
}

Input& Console::input()
{
    return input_;
}

Display& Console::display()
{
    return display_;
}

Storage& Console::storage()
{
    return storage_;
}

GameLibrary& Console::gameLibrary()
{
    return gameLibrary_;
}
