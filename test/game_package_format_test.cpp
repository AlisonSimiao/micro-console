#include <cassert>
#include <cstdint>
#include <cstring>
#include <iostream>

#include "game/package/game_manifest.hpp"
#include "game/package/package_format.hpp"

namespace {

void writeUint16(uint8_t* bytes, uint16_t value)
{
    bytes[0] = static_cast<uint8_t>(value);
    bytes[1] = static_cast<uint8_t>(value >> 8);
}

void writeUint32(uint8_t* bytes, uint32_t value)
{
    bytes[0] = static_cast<uint8_t>(value);
    bytes[1] = static_cast<uint8_t>(value >> 8);
    bytes[2] = static_cast<uint8_t>(value >> 16);
    bytes[3] = static_cast<uint8_t>(value >> 24);
}

void writeHeader(uint8_t* bytes, uint16_t version, uint16_t count, uint32_t indexSize)
{
    const uint8_t magic[] = {'A', 'L', 'I', 'P'};
    std::memcpy(bytes, magic, sizeof(magic));
    writeUint16(bytes + 4, version);
    writeUint16(bytes + 6, count);
    writeUint32(bytes + 8, indexSize);
}

void testHeaderValidation()
{
    uint8_t bytes[ALI_HEADER_SIZE];
    PackageHeader header{};

    writeHeader(bytes, 1, 2, 64);
    assert(decodePackageHeader(bytes, sizeof(bytes), 256, header));
    assert(header.entryCount == 2 && header.indexSize == 64);
    assert(!decodePackageHeader(bytes, sizeof(bytes) - 1, 256, header));

    writeHeader(bytes, 2, 1, 16);
    assert(!decodePackageHeader(bytes, sizeof(bytes), 256, header));
    writeHeader(bytes, 1, ALI_MAX_ENTRIES + 1, 16);
    assert(!decodePackageHeader(bytes, sizeof(bytes), 256, header));
    writeHeader(bytes, 1, 1, ALI_MAX_INDEX_SIZE + 1);
    assert(!decodePackageHeader(bytes, sizeof(bytes), 256, header));
    writeHeader(bytes, 1, 1, 32);
    assert(!decodePackageHeader(bytes, sizeof(bytes), 32, header));

    bytes[0] = 'X';
    assert(!decodePackageHeader(bytes, sizeof(bytes), 256, header));
}

void testEntryValidation()
{
    const char* path = "Codes/main.js";
    const size_t pathLength = std::strlen(path);
    uint8_t record[1 + ALI_MAX_PATH_LENGTH + 8] = {};
    record[0] = static_cast<uint8_t>(pathLength);
    std::memcpy(record + 1, path, pathLength);
    writeUint32(record + 1 + pathLength, 64);
    writeUint32(record + 1 + pathLength + 4, 12);

    PackageEntry entry{};
    const size_t recordLength = 1 + pathLength + 8;
    assert(decodePackageEntry(record, recordLength, entry));
    assert(std::strcmp(entry.path, path) == 0);
    assert(entry.offset == 64 && entry.size == 12);

    assert(isPackageEntryInRange(entry, 64, 76));
    assert(!isPackageEntryInRange(entry, 65, 76));
    assert(!isPackageEntryInRange(entry, 64, 75));
    PackageEntry malformed{};
    assert(!decodePackageEntry(record, recordLength - 1, malformed));
    entry.offset = UINT32_MAX - 2;
    entry.size = 8;
    assert(!isPackageEntryInRange(entry, 64, UINT32_MAX));

    assert(isValidPackagePath("Assets/image.bin", 16));
    assert(!isValidPackagePath("../secret", 9));
    assert(!isValidPackagePath("Assets/../secret", 16));
    assert(!isValidPackagePath("/absolute", 9));
    assert(!isValidPackagePath("Assets//image", 13));
    assert(!isValidPackagePath("Assets\\image", 12));
    assert(!isValidPackagePath("Assets/./image", 14));

    PackageEntry other{};
    std::strcpy(other.path, "Assets/data.bin");
    other.offset = 70;
    other.size = 4;
    entry.offset = 64;
    entry.size = 12;
    assert(packageEntriesConflict(entry, other));
    other.offset = 76;
    assert(!packageEntriesConflict(entry, other));
    std::strcpy(other.path, "Codes/main.js");
    assert(packageEntriesConflict(entry, other));
}

void testManifestValidation()
{
    const char valid[] =
        "{\"id\":\"example-game\",\"name\":\"Example Game\","
        "\"version\":\"1.0.0\",\"packageVersion\":1,"
        "\"author\":\"Example Author\",\"entrypoint\":\"Codes/main.js\","
        "\"cover\":\"Capa.bin\"}";
    GameManifest manifest{};
    assert(parseGameManifest(valid, sizeof(valid) - 1, manifest));
    assert(std::strcmp(manifest.id, "example-game") == 0);
    assert(std::strcmp(manifest.entrypoint, "Codes/main.js") == 0);
    assert(manifest.packageVersion == 1);

    const char missingField[] =
        "{\"id\":\"example-game\",\"name\":\"Game\",\"version\":\"1\","
        "\"packageVersion\":1,\"author\":\"Author\","
        "\"entrypoint\":\"main.js\"}";
    assert(!parseGameManifest(missingField, sizeof(missingField) - 1, manifest));

    const char incompatibleVersion[] =
        "{\"id\":\"example-game\",\"name\":\"Game\",\"version\":\"1\","
        "\"packageVersion\":2,\"author\":\"Author\","
        "\"entrypoint\":\"main.js\",\"cover\":\"cover.bin\"}";
    assert(!parseGameManifest(
        incompatibleVersion,
        sizeof(incompatibleVersion) - 1,
        manifest));

    const char invalidPath[] =
        "{\"id\":\"example-game\",\"name\":\"Game\",\"version\":\"1\","
        "\"packageVersion\":1,\"author\":\"Author\","
        "\"entrypoint\":\"../main.js\",\"cover\":\"cover.bin\"}";
    assert(!parseGameManifest(invalidPath, sizeof(invalidPath) - 1, manifest));

    const char invalidId[] =
        "{\"id\":\"Example Game\",\"name\":\"Game\",\"version\":\"1\","
        "\"packageVersion\":1,\"author\":\"Author\","
        "\"entrypoint\":\"main.js\",\"cover\":\"cover.bin\"}";
    assert(!parseGameManifest(invalidId, sizeof(invalidId) - 1, manifest));
    assert(!parseGameManifest("{", 1, manifest));
    assert(!parseGameManifest(valid, 0, manifest));

    const char embeddedNul[] =
        "{\"id\":\"example\\u0000game\",\"name\":\"Game\",\"version\":\"1\","
        "\"packageVersion\":1,\"author\":\"Author\","
        "\"entrypoint\":\"main.js\",\"cover\":\"cover.bin\"}";
    assert(!parseGameManifest(embeddedNul, sizeof(embeddedNul) - 1, manifest));

    char oversized[GAME_MANIFEST_MAX_SIZE + 1];
    std::memset(oversized, ' ', sizeof(oversized));
    assert(!parseGameManifest(oversized, sizeof(oversized), manifest));
}

} // namespace

int main()
{
    testHeaderValidation();
    testEntryValidation();
    testManifestValidation();
    std::cout << "Game Package format tests passed\n";
}
