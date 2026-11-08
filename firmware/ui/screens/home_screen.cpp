#include "ui/screens/home_screen.hpp"

#include <cstdio>
#include <cstring>

namespace {

constexpr uint16_t COLOR_BACKGROUND = 0x0841;
constexpr uint16_t COLOR_TITLE = 0xFFFF;
constexpr uint16_t COLOR_SUBTITLE = 0x8C71;
constexpr uint16_t COLOR_DIVIDER = 0x4208;
constexpr uint16_t COLOR_OPTION_BACKGROUND = 0x18E3;
constexpr uint16_t COLOR_OPTION_TEXT = 0xE71C;
constexpr uint16_t COLOR_SELECTED_BACKGROUND = 0x05FF;
constexpr uint16_t COLOR_SELECTED_TEXT = 0x0000;

// A fonte embutida do Display é ASCII 6x8; o tamanho N multiplica ambos.
constexpr uint8_t GLYPH_WIDTH = 6;
constexpr uint8_t GLYPH_HEIGHT = 8;
constexpr uint8_t OPTION_TEXT_SIZE = 2;
constexpr int DIVIDER_THICKNESS = 2;
constexpr int FOOTER_HEIGHT = 8;

const char* const OPTIONS[] = {
    "Jogos",
    "Configuracoes",
    "Sobre",
};

constexpr uint8_t MENU_OPTION_COUNT = sizeof(OPTIONS) / sizeof(OPTIONS[0]);
static_assert(MENU_OPTION_COUNT == 3, "HomeScreen expects three menu options");

struct Layout {
    int margin;
    int headerTitleY;
    int headerSubtitleY;
    int dividerY;
    int optionX;
    int optionWidth;
    int optionHeight;
    int optionGap;
    int firstOptionY;
    int footerY;
    int textPadding;
};

Layout computeLayout(const Display& display)
{
    const int width = display.width();
    const int height = display.height();

    int margin = width / 20;
    if (margin < 8) {
        margin = 8;
    } else if (margin > 20) {
        margin = 20;
    }

    int optionHeight = height / 7;
    if (optionHeight < 28) {
        optionHeight = 28;
    } else if (optionHeight > 56) {
        optionHeight = 56;
    }

    int optionGap = optionHeight / 3;
    if (optionGap < 8) {
        optionGap = 8;
    }

    Layout layout{};
    layout.margin = margin;
    layout.headerTitleY = margin;
    layout.headerSubtitleY = layout.headerTitleY + GLYPH_HEIGHT * 2 + 2;
    layout.dividerY = layout.headerSubtitleY + GLYPH_HEIGHT + 6;
    layout.optionX = margin;
    layout.optionWidth = width - 2 * margin;
    layout.optionHeight = optionHeight;
    layout.optionGap = optionGap;
    layout.textPadding = optionHeight / 3;
    layout.footerY = height - margin - FOOTER_HEIGHT;

    const int optionsTop = layout.dividerY + DIVIDER_THICKNESS + margin;
    const int optionsBottom = layout.footerY - margin;
    const int groupHeight =
        MENU_OPTION_COUNT * optionHeight + (MENU_OPTION_COUNT - 1) * optionGap;
    const int freeSpace = optionsBottom - optionsTop - groupHeight;
    layout.firstOptionY = optionsTop + (freeSpace > 0 ? freeSpace / 2 : 0);

    return layout;
}

int textWidth(const char* text, uint8_t size)
{
    return static_cast<int>(strlen(text)) * GLYPH_WIDTH * size;
}

void drawCenteredText(Display& display, int y, const char* text, uint16_t color, uint8_t size)
{
    int x = (display.width() - textWidth(text, size)) / 2;
    if (x < 0) {
        x = 0;
    }

    display.drawText(x, y, text, color, size);
}

void drawOption(Display& display, const Layout& layout, uint8_t index, const char* text,
                bool selected)
{
    const int optionY = layout.firstOptionY + index * (layout.optionHeight + layout.optionGap);
    const uint16_t background =
        selected ? COLOR_SELECTED_BACKGROUND : COLOR_OPTION_BACKGROUND;
    const uint16_t foreground = selected ? COLOR_SELECTED_TEXT : COLOR_OPTION_TEXT;

    display.fillRect(layout.optionX, optionY, layout.optionWidth, layout.optionHeight, background);

    char label[24];
    snprintf(label, sizeof(label), "%c %s", selected ? '>' : ' ', text);

    const int textHeight = GLYPH_HEIGHT * OPTION_TEXT_SIZE;
    const int textY = optionY + (layout.optionHeight - textHeight) / 2;
    display.drawText(layout.optionX + layout.textPadding, textY, label, foreground,
                     OPTION_TEXT_SIZE);
}

} // namespace

HomeScreen::HomeScreen(Display& display, Input& input)
    : display_(display), input_(input), selectedIndex_(0) {}

void HomeScreen::begin()
{
    selectedIndex_ = 0;
    draw();
}

void HomeScreen::update()
{
    const uint8_t previousIndex = selectedIndex_;

    if (input_.wasPressed("UP")) {
        selectedIndex_ =
            static_cast<uint8_t>((selectedIndex_ + OPTION_COUNT - 1) % OPTION_COUNT);
    } else if (input_.wasPressed("DOWN")) {
        selectedIndex_ = static_cast<uint8_t>((selectedIndex_ + 1) % OPTION_COUNT);
    }

    if (selectedIndex_ == previousIndex) {
        return;
    }

    const Layout layout = computeLayout(display_);
    drawOption(display_, layout, previousIndex, OPTIONS[previousIndex], false);
    drawOption(display_, layout, selectedIndex_, OPTIONS[selectedIndex_], true);
}

void HomeScreen::draw()
{
    static_assert(MENU_OPTION_COUNT == OPTION_COUNT,
                  "HomeScreen option array does not match OPTION_COUNT");

    const Layout layout = computeLayout(display_);

    display_.clear(COLOR_BACKGROUND);
    drawCenteredText(display_, layout.headerTitleY, "MICRO CONSOLE", COLOR_TITLE, 2);
    drawCenteredText(display_, layout.headerSubtitleY, "MENU PRINCIPAL", COLOR_SUBTITLE, 1);
    display_.fillRect(layout.margin, layout.dividerY, display_.width() - 2 * layout.margin,
                      DIVIDER_THICKNESS, COLOR_DIVIDER);

    for (uint8_t i = 0; i < MENU_OPTION_COUNT; ++i) {
        drawOption(display_, layout, i, OPTIONS[i], i == selectedIndex_);
    }

    drawCenteredText(display_, layout.footerY, "UP/DOWN: NAVEGAR", COLOR_SUBTITLE, 1);
}
