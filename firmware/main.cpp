#include <cstdio>

#include "boot/boot.hpp"
#include "config/config.hpp"
#include "hal/display/display.hpp"
#include "hal/input/input.hpp"

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

void drawTestScreen(Display& display)
{
    const int width = display.width();
    const int height = display.height();
    const int rotation = display.getRotation();

    display.clear(COLOR_BACKGROUND);

    display.fillRect(0, 0, width, BORDER_THICKNESS, COLOR_BORDER);
    display.fillRect(0, height - BORDER_THICKNESS, width, BORDER_THICKNESS, COLOR_BORDER);
    display.fillRect(0, 0, BORDER_THICKNESS, height, COLOR_BORDER);
    display.fillRect(width - BORDER_THICKNESS, 0, BORDER_THICKNESS, height, COLOR_BORDER);

    const int barTop = height / 3;
    const int barHeight = height / 6;
    const int barWidth = width / BAR_COUNT;
    int barX = 0;
    for (int i = 0; i < BAR_COUNT; ++i) {
        const bool lastBar = (i == BAR_COUNT - 1);
        const int currentWidth = lastBar ? width - barX : barWidth;
        display.fillRect(barX, barTop, currentWidth, barHeight, BAR_COLORS[i]);
        barX += currentWidth;
    }

    display.drawText(TEXT_MARGIN, height / 16, "Micro Console", COLOR_TITLE, 2);
    display.drawText(TEXT_MARGIN, height / 16 + 24, "Painel ILI9341", COLOR_FOREGROUND, 1);

    char geometry[32];
    snprintf(geometry, sizeof(geometry), "%dx%d rot %d", width, height, rotation);
    display.drawText(TEXT_MARGIN, height / 16 + 38, geometry, COLOR_FOREGROUND, 1);
}

} // namespace

Config config;
Display display(config);
Input input(config);
Boot boot(config);

[[noreturn]] void haltOnBootFailure(const char* message) {
    Serial.println(message);
    while (true) {
        delay(1000);
    }
}

void setup() {
    if (!boot.addModule(display)) {
        Serial.begin(115200);
        haltOnBootFailure("Boot failed: could not register Display module");
    }

    if (!boot.addModule(input)) {
        Serial.begin(115200);
        haltOnBootFailure("Boot failed: could not register Input module");
    }

    if (!boot.begin()) {
        haltOnBootFailure("Boot failed; system halted");
    }

    const int rotation = display.getRotation();
    const int brightness = display.getBrightness();

    drawTestScreen(display);

    Serial.print("Display: test screen ILI9341 ");
    Serial.print(display.width());
    Serial.print("x");
    Serial.print(display.height());
    Serial.print(", rotation=");
    Serial.print(rotation);
    Serial.print(", brightness=");
    Serial.println(brightness);
}

void loop() {
    input.update();
}
