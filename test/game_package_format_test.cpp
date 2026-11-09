#include <cassert>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <string>

#include "game/package/game_manifest.hpp"
#include "game/package/package_format.hpp"
#include "test/support/build_package.hpp"

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

    writeHeader(bytes, 1, 1, 0);
    assert(!decodePackageHeader(bytes, sizeof(bytes), ALI_HEADER_SIZE, header));

    writeHeader(bytes, 1, 1, ALI_MAX_INDEX_SIZE);
    assert(decodePackageHeader(
        bytes,
        sizeof(bytes),
        ALI_HEADER_SIZE + ALI_MAX_INDEX_SIZE,
        header));
    writeHeader(bytes, 1, 1, ALI_MAX_INDEX_SIZE + 1);
    assert(!decodePackageHeader(
        bytes,
        sizeof(bytes),
        ALI_HEADER_SIZE + ALI_MAX_INDEX_SIZE + 1,
        header));

    writeHeader(bytes, 1, 64, 64 * 10);
    assert(decodePackageHeader(
        bytes, sizeof(bytes), ALI_HEADER_SIZE + 64 * 10, header));
    writeHeader(bytes, 1, 0, 0);
    assert(!decodePackageHeader(bytes, sizeof(bytes), ALI_HEADER_SIZE, header));
    writeHeader(bytes, 1, 65, 65 * 10);
    assert(!decodePackageHeader(bytes, sizeof(bytes), ALI_HEADER_SIZE, header));
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

    std::string longestPath(ALI_MAX_PATH_LENGTH, 'a');
    std::vector<uint8_t> longestRecord;
    longestRecord.push_back(static_cast<uint8_t>(longestPath.size()));
    longestRecord.insert(longestRecord.end(), longestPath.begin(), longestPath.end());
    appendUint32(longestRecord, 64);
    appendUint32(longestRecord, 0);
    assert(decodePackageEntry(
        longestRecord.data(),
        longestRecord.size(),
        malformed));

    std::string tooLongPath(128, 'a');
    std::vector<uint8_t> tooLongRecord;
    tooLongRecord.push_back(128);
    tooLongRecord.insert(tooLongRecord.end(), tooLongPath.begin(), tooLongPath.end());
    appendUint32(tooLongRecord, 64);
    appendUint32(tooLongRecord, 0);
    assert(!decodePackageEntry(
        tooLongRecord.data(),
        tooLongRecord.size(),
        malformed));
    uint8_t zeroPathRecord[9] = {};
    assert(!decodePackageEntry(zeroPathRecord, sizeof(zeroPathRecord), malformed));

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
    assert(!isValidPackagePath("Assets/image/", 13));
    const char nulPath[] = {'A', 's', 's', 'e', 't', 's', '/', '\0', 'x'};
    assert(!isValidPackagePath(nulPath, sizeof(nulPath)));
    const char controlPath[] = {'A', 's', 's', 'e', 't', 's', '/', '\x1f'};
    assert(!isValidPackagePath(controlPath, sizeof(controlPath)));
    const char nonAsciiPath[] = {'A', 's', 's', 'e', 't', 's', '/', '\x7f'};
    assert(!isValidPackagePath(nonAsciiPath, sizeof(nonAsciiPath)));

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

    other.offset = entry.offset;
    other.size = entry.size;
    std::strcpy(other.path, "Assets/data.bin");
    assert(packageEntriesConflict(entry, other));
    other.offset = entry.offset + 2;
    other.size = 2;
    assert(packageEntriesConflict(entry, other));
    other.offset = entry.offset - 2;
    other.size = 4;
    assert(packageEntriesConflict(entry, other));
    other.offset = entry.offset + entry.size;
    other.size = 1;
    assert(!packageEntriesConflict(entry, other));
    other.offset = entry.offset;
    other.size = 0;
    assert(!packageEntriesConflict(entry, other));
}

std::string manifestJson(
    const std::string& id = "test-game",
    const std::string& name = "Test",
    const std::string& version = "1.0",
    const std::string& author = "Author",
    const std::string& entrypoint = "main.js",
    const std::string& cover = "cover.bin",
    const std::string& packageVersion = "1")
{
    return "{\"id\":\"" + id + "\",\"name\":\"" + name +
           "\",\"version\":\"" + version + "\",\"packageVersion\":" +
           packageVersion + ",\"author\":\"" + author +
           "\",\"entrypoint\":\"" + entrypoint + "\",\"cover\":\"" +
           cover + "\"}";
}

std::string removeJsonField(const std::string& json, const std::string& key)
{
    const std::string marker = std::string("\"") + key + "\":";
    const size_t keyStart = json.find(marker);
    assert(keyStart != std::string::npos);
    size_t valueEnd = keyStart + marker.size();
    if (json[valueEnd] == '"') {
        ++valueEnd;
        bool escaped = false;
        while (valueEnd < json.size()) {
            const char character = json[valueEnd++];
            if (escaped) {
                escaped = false;
            } else if (character == '\\') {
                escaped = true;
            } else if (character == '"') {
                break;
            }
        }
    } else {
        while (valueEnd < json.size() && json[valueEnd] != ',' && json[valueEnd] != '}') {
            ++valueEnd;
        }
    }

    size_t eraseStart = keyStart;
    if (valueEnd < json.size() && json[valueEnd] == ',') {
        ++valueEnd;
    } else if (eraseStart > 0 && json[eraseStart - 1] == ',') {
        --eraseStart;
    }

    std::string result = json;
    result.erase(eraseStart, valueEnd - eraseStart);
    return result;
}

void testManifestValidation()
{
    const std::string valid = manifestJson(
        "example-game",
        "Example Game",
        "1.0.0",
        "Example Author",
        "Codes/main.js",
        "Capa.bin");
    GameManifest manifest{};
    assert(parseGameManifest(valid.c_str(), valid.size(), manifest));
    assert(std::strcmp(manifest.id, "example-game") == 0);
    assert(std::strcmp(manifest.entrypoint, "Codes/main.js") == 0);
    assert(manifest.packageVersion == 1);

    const char* const fields[] = {
        "id", "name", "version", "packageVersion", "author", "entrypoint", "cover"};
    for (size_t i = 0; i < sizeof(fields) / sizeof(fields[0]); ++i) {
        const std::string missing = removeJsonField(valid, fields[i]);
        assert(!parseGameManifest(missing.c_str(), missing.size(), manifest));
    }

    const char* const invalidVersions[] = {"0", "2", "65536", "\"1\"", "1.0", "-1"};
    for (size_t i = 0; i < sizeof(invalidVersions) / sizeof(invalidVersions[0]); ++i) {
        const std::string json = manifestJson(
            "test-game", "Test", "1.0", "Author", "main.js", "cover.bin",
            invalidVersions[i]);
        assert(!parseGameManifest(json.c_str(), json.size(), manifest));
    }

    const std::string uppercaseId = manifestJson("Test-game");
    const std::string spacedId = manifestJson("test game");
    assert(!parseGameManifest(uppercaseId.c_str(), uppercaseId.size(), manifest));
    assert(!parseGameManifest(spacedId.c_str(), spacedId.size(), manifest));
    const std::string maxId(63, 'a');
    const std::string maxIdJson = manifestJson(maxId);
    assert(parseGameManifest(maxIdJson.c_str(), maxIdJson.size(), manifest));
    const std::string tooLongId(64, 'a');
    const std::string tooLongIdJson = manifestJson(tooLongId);
    assert(!parseGameManifest(tooLongIdJson.c_str(), tooLongIdJson.size(), manifest));

    const std::string maxMetadata = manifestJson(
        "id", std::string(63, 'n'), std::string(31, 'v'), std::string(63, 'a'));
    assert(parseGameManifest(maxMetadata.c_str(), maxMetadata.size(), manifest));
    const std::string tooLongName = manifestJson("id", std::string(64, 'n'));
    const std::string tooLongVersion = manifestJson(
        "id", "Test", std::string(32, 'v'));
    const std::string tooLongAuthor = manifestJson(
        "id", "Test", "1", std::string(64, 'a'));
    assert(!parseGameManifest(tooLongName.c_str(), tooLongName.size(), manifest));
    assert(!parseGameManifest(tooLongVersion.c_str(), tooLongVersion.size(), manifest));
    assert(!parseGameManifest(tooLongAuthor.c_str(), tooLongAuthor.size(), manifest));

    const std::string invalidEntry = manifestJson(
        "id", "Test", "1", "Author", "../main.js");
    const std::string invalidCover = manifestJson(
        "id", "Test", "1", "Author", "main.js", "../cover.bin");
    assert(!parseGameManifest(invalidEntry.c_str(), invalidEntry.size(), manifest));
    assert(!parseGameManifest(invalidCover.c_str(), invalidCover.size(), manifest));

    const std::string nulFields[] = {
        manifestJson("a\\u0000b"), manifestJson("id", "a\\u0000b"),
        manifestJson("id", "Test", "a\\u0000b"),
        manifestJson("id", "Test", "1", "a\\u0000b"),
        manifestJson("id", "Test", "1", "Author", "a\\u0000b"),
        manifestJson("id", "Test", "1", "Author", "main.js", "a\\u0000b")};
    for (size_t i = 0; i < sizeof(nulFields) / sizeof(nulFields[0]); ++i) {
        assert(!parseGameManifest(nulFields[i].c_str(), nulFields[i].size(), manifest));
    }

    std::string maxManifest = valid;
    maxManifest.append(GAME_MANIFEST_MAX_SIZE - maxManifest.size(), ' ');
    assert(maxManifest.size() == GAME_MANIFEST_MAX_SIZE);
    assert(parseGameManifest(maxManifest.c_str(), maxManifest.size(), manifest));
    maxManifest.push_back(' ');
    assert(!parseGameManifest(maxManifest.c_str(), maxManifest.size(), manifest));

    const char nonObject[] = "[]";
    assert(!parseGameManifest(nonObject, sizeof(nonObject) - 1, manifest));
    const char tooDeep[] = "{\"a\":{\"b\":{\"c\":{\"d\":{\"e\":{\"f\":{\"g\":{\"h\":{\"i\":1}}}}}}}}}";
    assert(!parseGameManifest(tooDeep, sizeof(tooDeep) - 1, manifest));
    assert(!parseGameManifest("{", 1, manifest));
    assert(!parseGameManifest(valid.c_str(), 0, manifest));

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
