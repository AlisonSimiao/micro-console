#pragma once

#include <cstddef>

#include "game/package/game_package.hpp"
#include "hal/storage/storage.hpp"

// Metadados leves de um jogo descoberto em `/games`. As strings são cópias dos
// campos do manifesto; `coverPath` é o caminho da capa dentro do pacote, não um
// caminho no cartão. Valores maiores que o campo são truncados por `copyText()`;
// os limites de GameInfo e GameManifest ainda não têm contrato comum.
struct GameInfo {
    char id[64];
    char name[64];
    char version[32];
    char author[64];
    char packagePath[128];
    char coverPath[128];
};

// Descobre, valida e representa os jogos disponíveis em `/games`. A biblioteca
// não executa jogos e não mantém arquivos abertos após `scan()`.
//
// A instância inclui um GamePackage de vida longa (~9 KB) e o array de
// metadados; por isso deve ser membro do Console, nunca uma variável local em
// `loop()` ou em uma task com stack pequena.
class GameLibrary {
public:
    static constexpr const char* DIRECTORY = "/games";
    static constexpr size_t MAX_GAMES = 16;
    static constexpr size_t MAX_INSTANCE_BYTES = 17500;

    explicit GameLibrary(Storage& storage);

    GameLibrary(const GameLibrary&) = delete;
    GameLibrary& operator=(const GameLibrary&) = delete;

    // Limpa a lista atual e percorre `/games` em ordem do sistema de arquivos.
    // Retorna false apenas se o Storage não estiver montado ou o diretório não
    // puder ser aberto; pacotes inválidos são ignorados.
    //
    // O diretório é percorrido por inteiro mesmo depois de a lista atingir
    // MAX_GAMES, pois `isTruncated()` depende de saber se ainda existe outro
    // pacote válido; os excedentes não são armazenados.
    bool scan();
    bool refresh();

    size_t count() const;
    const GameInfo* get(size_t index) const;

    // Verdadeiro quando `/games` contém mais pacotes `.ali` válidos do que
    // MAX_GAMES. Pacotes inválidos em excesso não contam.
    bool isTruncated() const;

private:
    Storage& storage_;
    GamePackage package_;
    GameInfo games_[MAX_GAMES];
    size_t count_;
    bool truncated_;
};
