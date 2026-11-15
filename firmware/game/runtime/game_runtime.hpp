#pragma once

#include <cstddef>
#include <cstdint>

#include "game/library/game_library.hpp"
#include "game/package/game_package.hpp"
#include "hal/storage/storage.hpp"

// Controla a execução de um jogo por vez. O ciclo é explícito: `launch()` abre e
// valida o pacote, `update()` avança a sessão e `stop()` libera os recursos.
//
// O Runtime não conhece telas, não lê Input, não desenha e não imprime: falhas
// são reportadas por `failure()` e o Console decide o que fazer. Ele também não
// interpreta JavaScript; a engine entra em `update()` quando existir.
//
// A instância mantém um GamePackage próprio (~9 KB) e por isso deve ter vida
// longa: membro do Console, nunca variável local em `loop()`.
class GameRuntime {
public:
    enum class State : uint8_t { Idle, Running, Failed };

    enum class Failure : uint8_t {
        None,
        InvalidArgument,
        StorageUnavailable,
        PackageOpenFailed,
    };

    static constexpr size_t MAX_INSTANCE_BYTES = 9600;

    explicit GameRuntime(Storage& storage);

    GameRuntime(const GameRuntime&) = delete;
    GameRuntime& operator=(const GameRuntime&) = delete;

    // Abre e valida `game.packagePath`. Com uma sessão ativa a chamada é
    // rejeitada sem efeito: o jogo atual não é substituído.
    bool launch(const GameInfo& game);

    // Avança a sessão. Fora de `Running` é um no-op.
    void update();

    // Encerra a sessão e libera o pacote. Idempotente.
    void stop();

    State state() const;
    Failure failure() const;
    bool isRunning() const;
    uint32_t frameCount() const;

private:
    Storage& storage_;
    GamePackage package_;
    State state_;
    Failure failure_;
    uint32_t frameCount_;
};
