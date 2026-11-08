#pragma once

#include <cstddef>
#include <cstdint>

constexpr size_t GAME_MANIFEST_MAX_SIZE = 4096;

struct GameManifest {
    char id[64];
    char name[64];
    char version[32];
    uint16_t packageVersion;
    char author[64];
    char entrypoint[128];
    char cover[128];
};

bool parseGameManifest(const char* json, size_t length, GameManifest& manifest);
