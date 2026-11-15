#include "console/console.hpp"

#include <Arduino.h>
#include <cstring>

namespace {

const char* runtimeFailureText(GameRuntime::Failure failure)
{
    switch (failure) {
    case GameRuntime::Failure::InvalidArgument:
        return "invalid package path";
    case GameRuntime::Failure::StorageUnavailable:
        return "storage unavailable";
    case GameRuntime::Failure::PackageOpenFailed:
        return "package open failed";
    case GameRuntime::Failure::None:
        break;
    }

    return "unknown failure";
}

int centeredTextX(Display& display, const char* text, uint8_t size)
{
    const int width = static_cast<int>(std::strlen(text)) * 6 * size;
    const int x = (display.width() - width) / 2;
    return x < 0 ? 0 : x;
}

} // namespace

Console::Console()
    : config_(),
      input_(config_),
      display_(config_),
      storage_(config_),
      gameLibrary_(storage_),
      gameRuntime_(storage_),
      system_(),
      boot_(config_),
      homeScreen_(display_, input_),
      gamesScreen_(display_, input_, gameLibrary_)
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

    if (gameRuntime_.isRunning()) {
        gameRuntime_.update();

        // Provisório na V1: sem engine/SDK, B encerra a sessão. Quando o jogo
        // tiver controles próprios, a saída migra para o menu do sistema.
        if (input_.wasPressed("B")) {
            gameRuntime_.stop();
            homeScreen_.begin();
        }

        return;
    }

    // Alcançado quando uma sessão em execução falhar no futuro. Falhas de
    // launch() são tratadas na própria tela de jogos.
    if (gameRuntime_.state() == GameRuntime::State::Failed) {
        reportRuntimeFailure();
        gameRuntime_.stop();
        homeScreen_.begin();
        return;
    }

    if (gamesScreen_.isVisible()) {
        if (input_.wasPressed("B")) {
            gamesScreen_.close();
            homeScreen_.begin();
            return;
        }

        if (input_.wasPressed("A")) {
            const GameInfo* game = gamesScreen_.selected();
            if (game != nullptr && gameRuntime_.launch(*game)) {
                gamesScreen_.close();
                showGamePlaceholder();
            } else if (gameRuntime_.state() == GameRuntime::State::Failed) {
                reportRuntimeFailure();
                gameRuntime_.stop(); // permanece na lista
            }
            return;
        }

        gamesScreen_.update();
        return;
    }

    homeScreen_.update();

    if (homeScreen_.isSelected(0) && input_.wasPressed("A")) {
        gamesScreen_.begin();
    }
}

void Console::reportRuntimeFailure()
{
    Serial.print("Console: game launch failed: ");
    Serial.println(runtimeFailureText(gameRuntime_.failure()));
}

void Console::showGamePlaceholder()
{
    // Aviso temporário enquanto o GameRuntime não executa gameplay. Não é uma
    // tela nem um estado de UI: some ao encerrar a sessão.
    display_.clear(0x0000);
    display_.drawText(centeredTextX(display_, "JOGO EM EXECUCAO", 2),
                      display_.height() / 2 - 20, "JOGO EM EXECUCAO", 0xFFFF, 2);
    display_.drawText(centeredTextX(display_, "B: SAIR", 1),
                      display_.height() / 2 + 16, "B: SAIR", 0x8C71, 1);
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

System& Console::system()
{
    return system_;
}
