#pragma once

#include <cstdint>

#include "game/package/game_manifest.hpp"
#include "game/package/package_reader.hpp"

class GamePackage {
public:
    static constexpr size_t MAX_MANIFEST_SIZE = GAME_MANIFEST_MAX_SIZE;

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
