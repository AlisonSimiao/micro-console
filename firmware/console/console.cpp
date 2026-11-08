#include "console/console.hpp"

#include <Arduino.h>
#include <cstdio>

namespace {

constexpr uint16_t COLOR_BACKGROUND = 0x0000;
constexpr uint16_t COLOR_FOREGROUND = 0xFFFF;
constexpr uint16_t COLOR_TITLE = 0xFFE0;
constexpr uint16_t COLOR_BORDER = 0xFFFF;
constexpr int BORDER_THICKNESS = 3;
constexpr int TEXT_MARGIN = BORDER_THICKNESS + 6;

const uint16_t BAR_COLORS[] = {
    0xF800, // vermelho
    0x07E0, // verde
    0x001F, // azul
    0xFFE0, // amarelo
    0x07FF, // ciano
    0xF81F, // magenta
    0xFFFF, // branco
    0x8410, // cinza
};
constexpr int BAR_COUNT = sizeof(BAR_COLORS) / sizeof(BAR_COLORS[0]);

} // namespace

Console::Console()
    : config_(), input_(config_), display_(config_), boot_(config_)
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

    if (!boot_.begin()) {
        return false;
    }

    drawTestScreen();

    Serial.print("Display: test screen ILI9341 ");
    Serial.print(display_.width());
    Serial.print("x");
    Serial.print(display_.height());
    Serial.print(", rotation=");
    Serial.print(display_.getRotation());
    Serial.print(", brightness=");
    Serial.println(display_.getBrightness());

    return true;
}

void Console::update()
{
    input_.update();
}

Input& Console::input()
{
    return input_;
}

Display& Console::display()
{
    return display_;
}

void Console::drawTestScreen()
{
    const int width = display_.width();
    const int height = display_.height();
    const int rotation = display_.getRotation();

    display_.clear(COLOR_BACKGROUND);

    display_.fillRect(0, 0, width, BORDER_THICKNESS, COLOR_BORDER);
    display_.fillRect(0, height - BORDER_THICKNESS, width, BORDER_THICKNESS, COLOR_BORDER);
    display_.fillRect(0, 0, BORDER_THICKNESS, height, COLOR_BORDER);
    display_.fillRect(width - BORDER_THICKNESS, 0, BORDER_THICKNESS, height, COLOR_BORDER);

    const int barTop = height / 3;
    const int barHeight = height / 6;
    const int barWidth = width / BAR_COUNT;
    int barX = 0;
    for (int i = 0; i < BAR_COUNT; ++i) {
        const bool lastBar = (i == BAR_COUNT - 1);
        const int currentWidth = lastBar ? width - barX : barWidth;
        display_.fillRect(barX, barTop, currentWidth, barHeight, BAR_COLORS[i]);
        barX += currentWidth;
    }

    display_.drawText(TEXT_MARGIN, height / 16, "Micro Console", COLOR_TITLE, 2);
    display_.drawText(TEXT_MARGIN, height / 16 + 24, "Painel ILI9341", COLOR_FOREGROUND, 1);

    char geometry[32];
    snprintf(geometry, sizeof(geometry), "%dx%d rot %d", width, height, rotation);
    display_.drawText(TEXT_MARGIN, height / 16 + 38, geometry, COLOR_FOREGROUND, 1);
}
