#include "ui/screens/games_screen.hpp"

#include <cstdio>
#include <cstring>

namespace {

constexpr uint16_t COLOR_BACKGROUND = 0x0841;
constexpr uint16_t COLOR_TITLE = 0xFFFF;
constexpr uint16_t COLOR_SUBTITLE = 0x8C71;
constexpr uint16_t COLOR_DIVIDER = 0x4208;
constexpr uint16_t COLOR_ROW_BACKGROUND = 0x18E3;
constexpr uint16_t COLOR_ROW_TEXT = 0xE71C;
constexpr uint16_t COLOR_SELECTED_BACKGROUND = 0x05FF;
constexpr uint16_t COLOR_SELECTED_TEXT = 0x0000;

constexpr uint8_t GLYPH_WIDTH = 6;
constexpr uint8_t GLYPH_HEIGHT = 8;
constexpr uint8_t TEXT_SIZE = 1;
constexpr int HEADER_TOP_MARGIN = 10;
constexpr int ROW_WIDTH_MARGIN = 18;

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

void drawRow(Display& display, int x, int y, int width, int height, const char* label,
             bool selected)
{
    const uint16_t background =
        selected ? COLOR_SELECTED_BACKGROUND : COLOR_ROW_BACKGROUND;
    const uint16_t foreground = selected ? COLOR_SELECTED_TEXT : COLOR_ROW_TEXT;

    display.fillRect(x, y, width, height, background);

    char rowText[48];
    std::snprintf(rowText, sizeof(rowText), "%c %s", selected ? '>' : ' ', label);

    display.drawText(x + 8, y + 6, rowText, foreground, TEXT_SIZE);
}

} // namespace

GamesScreen::GamesScreen(Display& display, Input& input, GameLibrary& gameLibrary)
    : display_(display), input_(input), gameLibrary_(gameLibrary), selectedIndex_(0), visible_(false)
{
}

void GamesScreen::begin()
{
    selectedIndex_ = 0;
    visible_ = true;
    draw();
}

void GamesScreen::update()
{
    if (!visible_) {
        return;
    }

    const size_t count = gameLibrary_.count();
    if (count == 0) {
        return;
    }

    if (input_.wasPressed("UP")) {
        if (selectedIndex_ == 0) {
            selectedIndex_ = static_cast<uint8_t>(count - 1);
        } else {
            --selectedIndex_;
        }
    } else if (input_.wasPressed("DOWN")) {
        selectedIndex_ = static_cast<uint8_t>((selectedIndex_ + 1) % count);
    } else {
        // Iniciar e encerrar pertencem ao Console; a tela só reage à navegação.
        return;
    }

    draw();
}

bool GamesScreen::isVisible() const
{
    return visible_;
}

const GameInfo* GamesScreen::selected() const
{
    return gameLibrary_.get(selectedIndex_);
}

void GamesScreen::close()
{
    visible_ = false;
}

void GamesScreen::draw()
{
    display_.clear(COLOR_BACKGROUND);

    const int width = display_.width();
    const int height = display_.height();
    const int rowWidth = width - ROW_WIDTH_MARGIN * 2;
    const int rowHeight = 24;
    const int startY = HEADER_TOP_MARGIN + 26;

    drawCenteredText(display_, HEADER_TOP_MARGIN, "JOGOS", COLOR_TITLE, 2);
    display_.fillRect(18, 42, width - 36, 2, COLOR_DIVIDER);

    const size_t count = gameLibrary_.count();
    if (count == 0) {
        drawCenteredText(display_, height / 2 - 8, "NENHUM JOGO", COLOR_SUBTITLE, 2);
        drawCenteredText(display_, height - 26, "B: VOLTAR", COLOR_SUBTITLE, 1);
        return;
    }

    const size_t maxRows = count < MAX_VISIBLE_ROWS ? count : MAX_VISIBLE_ROWS;
    for (size_t row = 0; row < maxRows; ++row) {
        const size_t index = (row + selectedIndex_) % count;
        const GameInfo* game = gameLibrary_.get(index);
        if (game == nullptr) {
            continue;
        }

        const int y = startY + static_cast<int>(row) * (rowHeight + 6);
        drawRow(display_, ROW_WIDTH_MARGIN, y, rowWidth, rowHeight,
                game->name[0] != '\0' ? game->name : game->id, row == 0);
    }

    drawCenteredText(display_, height - 26, "UP/DOWN: NAVEGAR  A: JOGAR  B: VOLTAR",
                     COLOR_SUBTITLE, 1);
}
