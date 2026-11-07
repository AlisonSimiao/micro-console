#pragma once

#include <cstdint>

#include "config/config.hpp"
#include "hal/core/hal_module.hpp"

class Display : public hal_module {
public:
    explicit Display(Config& config);

    bool begin() override;

    int width() const;
    int height() const;
    void clear(uint16_t color);
    void fillRect(int x, int y, int w, int h, uint16_t color);
    void drawText(int x, int y, const char* text, uint16_t color, uint8_t size);
    void setBrightness(uint8_t level);

private:
    Config& config;
    int backlightPin;
    uint8_t rotation;
    uint8_t brightness;
};
