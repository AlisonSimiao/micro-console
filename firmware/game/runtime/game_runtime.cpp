#include "game/runtime/game_runtime.hpp"

namespace {

// Garante que o objeto continue cabendo no orçamento documentado. Se este
// static_assert falhar, revise o consumo de memória e a documentação antes de
// aumentar o limite.
static_assert(
    sizeof(GameRuntime) <= GameRuntime::MAX_INSTANCE_BYTES,
    "GameRuntime grew beyond its documented memory budget");

bool hasText(const char* text)
{
    return text != nullptr && text[0] != '\0';
}

} // namespace

GameRuntime::GameRuntime(Storage& storage)
    : storage_(storage),
      package_(storage),
      state_(State::Idle),
      failure_(Failure::None),
      frameCount_(0)
{
}

bool GameRuntime::launch(const GameInfo& game)
{
    // A checagem de sessão ativa vem primeiro para que uma chamada rejeitada não
    // altere estado, falha nem contagem de frames do jogo em execução.
    if (isRunning()) {
        return false;
    }

    // Uma tentativa de launch sempre inicia uma nova sessão: frames anteriores
    // não pertencem a ela.
    frameCount_ = 0;

    if (!hasText(game.packagePath)) {
        state_ = State::Failed;
        failure_ = Failure::InvalidArgument;
        return false;
    }

    if (!storage_.isMounted()) {
        state_ = State::Failed;
        failure_ = Failure::StorageUnavailable;
        return false;
    }

    if (!package_.open(game.packagePath)) {
        state_ = State::Failed;
        failure_ = Failure::PackageOpenFailed;
        return false;
    }

    state_ = State::Running;
    failure_ = Failure::None;
    return true;
}

void GameRuntime::update()
{
    if (state_ != State::Running) {
        return;
    }

    // A JavaScript Engine entra aqui quando existir. Uma falha dela deve levar a
    // State::Failed e fechar o pacote na mesma chamada.
    ++frameCount_;
}

void GameRuntime::stop()
{
    package_.close();
    frameCount_ = 0;
    failure_ = Failure::None;
    state_ = State::Idle;
}

GameRuntime::State GameRuntime::state() const
{
    return state_;
}

GameRuntime::Failure GameRuntime::failure() const
{
    return failure_;
}

bool GameRuntime::isRunning() const
{
    return state_ == State::Running;
}

uint32_t GameRuntime::frameCount() const
{
    return frameCount_;
}
