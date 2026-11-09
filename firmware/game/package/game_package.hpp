#pragma once

#include <cstdint>

#include "game/package/game_manifest.hpp"
#include "game/package/package_reader.hpp"

// Abre e valida um pacote `.ali`, mantendo o índice completo e o manifesto em
// memória. O objeto é grande (~9 KB), então deve ter vida longa: mantenha-o como
// membro do Console/Game Library, nunca como variável local em `loop()` ou em
// uma task com stack pequena. `open()` usa ainda um buffer de manifesto de
// MAX_MANIFEST_SIZE + 1 bytes na stack.
class GamePackage {
public:
    static constexpr size_t MAX_MANIFEST_SIZE = GAME_MANIFEST_MAX_SIZE;
    static constexpr size_t MAX_INSTANCE_BYTES = 9500;

    explicit GamePackage(Storage& storage);

    GamePackage(const GamePackage&) = delete;
    GamePackage& operator=(const GamePackage&) = delete;

    bool open(const char* path);
    void close();

    bool isValid() const;
    const GameManifest& manifest() const;
    bool exists(const char* path) const;
    bool openFile(const char* path, PackageFile& file) const;

private:
    const PackageEntry* findEntry(const char* path) const;

    PackageReader reader_;
    char packagePath_[128];
    PackageEntry entries_[PackageReader::MAX_ENTRIES];
    uint8_t entryCount_;
    uint32_t packageSize_;
    GameManifest manifest_;
    bool valid_;
};
