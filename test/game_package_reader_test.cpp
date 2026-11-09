#include <cassert>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include "game/package/game_package.hpp"
#include "test/support/build_package.hpp"

static_assert(!std::is_copy_constructible<GamePackage>::value, "GamePackage must be non-copyable");
static_assert(!std::is_copy_assignable<GamePackage>::value, "GamePackage must be non-copyable");
static_assert(!std::is_copy_constructible<PackageFile>::value, "PackageFile must be non-copyable");
static_assert(!std::is_copy_assignable<PackageFile>::value, "PackageFile must be non-copyable");

namespace {

const char* const PACKAGE_PATH = "/games/test.ali";

std::vector<uint8_t> packageBytes()
{
    return buildPackage(validPackageEntries());
}

void addValidPackage(Storage& storage, const char* path = PACKAGE_PATH)
{
    storage.addFile(path, packageBytes());
}

std::vector<uint8_t> readAll(PackageFile& file)
{
    std::vector<uint8_t> result;
    uint8_t buffer[256];
    while (file.available() > 0) {
        const size_t count = file.read(buffer, sizeof(buffer));
        assert(count > 0);
        result.insert(result.end(), buffer, buffer + count);
    }
    return result;
}

void testGamePackageLifecycle()
{
    Storage storage;
    addValidPackage(storage);
    GamePackage package(storage);

    assert(!package.exists("Codes/main.js"));
    assert(!package.open(nullptr));
    assert(!package.isValid());
    assert(!package.open(""));
    assert(!package.open("game"));
    assert(!package.open("/games/test.ALI"));
    assert(!package.open("/games/missing.ali"));
    assert(!package.isValid());

    assert(package.open(PACKAGE_PATH));
    assert(package.isValid());
    assert(storage.openFileCount() == 0);
    assert(std::strcmp(package.manifest().id, "test-game") == 0);
    assert(std::strcmp(package.manifest().name, "Test") == 0);
    assert(std::strcmp(package.manifest().entrypoint, "Codes/main.js") == 0);
    assert(std::strcmp(package.manifest().cover, "cover.bin") == 0);
    assert(package.exists("manifest.json"));
    assert(package.exists("Codes/main.js"));
    assert(!package.exists("codes/main.js"));
    assert(!package.exists("missing.js"));
    assert(!package.exists("../escape"));
    assert(!package.exists(""));
    assert(!package.exists(nullptr));

    PackageFile entrypoint;
    assert(package.openFile("Codes/main.js", entrypoint));
    assert(entrypoint.isOpen());
    const std::vector<uint8_t> mainBytes = readAll(entrypoint);
    assert(std::string(mainBytes.begin(), mainBytes.end()) == "main-content");
    entrypoint.close();
    assert(storage.openFileCount() == 0);
}

void testGamePackageFilesAndClose()
{
    Storage storage;
    const std::vector<uint8_t> bytes = packageBytes();
    storage.addFile(PACKAGE_PATH, bytes);
    GamePackage package(storage);
    assert(package.open(PACKAGE_PATH));

    PackageFile manifest;
    assert(package.openFile("manifest.json", manifest));
    const std::string expectedManifest =
        "{\"id\":\"test-game\",\"name\":\"Test\",\"version\":\"1\","
        "\"packageVersion\":1,\"author\":\"Test\","
        "\"entrypoint\":\"Codes/main.js\",\"cover\":\"cover.bin\"}";
    const std::vector<uint8_t> manifestBytes = readAll(manifest);
    assert(std::string(manifestBytes.begin(), manifestBytes.end()) == expectedManifest);
    manifest.close();
    assert(!manifest.isOpen());
    assert(manifest.read(nullptr, 20) == 0);
    uint8_t oneByte = 0;
    assert(manifest.read(&oneByte, 1) == 0);

    PackageFile empty;
    assert(package.openFile("empty.bin", empty));
    assert(empty.isOpen());
    assert(empty.size() == 0);
    assert(empty.available() == 0);
    assert(empty.read(&oneByte, 1) == 0);
    empty.close();

    PackageFile first;
    PackageFile second;
    assert(package.openFile("Codes/main.js", first));
    assert(package.openFile("cover.bin", second));
    assert(storage.openFileCount() == 2);
    uint8_t firstBuffer[4];
    uint8_t secondBuffer[5];
    assert(first.read(firstBuffer, sizeof(firstBuffer)) == 4);
    assert(second.read(secondBuffer, sizeof(secondBuffer)) == 5);
    assert(std::memcmp(firstBuffer, "main", 4) == 0);
    assert(std::memcmp(secondBuffer, "cover", 5) == 0);

    PackageFile survivesClose;
    assert(package.openFile("cover.bin", survivesClose));
    package.close();
    package.close();
    assert(!package.isValid());
    assert(!package.exists("cover.bin"));
    assert(storage.openFileCount() == 3);
    assert(survivesClose.read(secondBuffer, sizeof(secondBuffer)) == 5);
    assert(std::memcmp(secondBuffer, "cover", 5) == 0);
    survivesClose.close();
    first.close();
    second.close();
    assert(storage.openFileCount() == 0);

    assert(package.open(PACKAGE_PATH));
    assert(package.isValid());
    PackageFile reusedOutput;
    assert(package.openFile("cover.bin", reusedOutput));
    assert(storage.openFileCount() == 1);
    assert(!package.openFile("../invalid", reusedOutput));
    assert(!reusedOutput.isOpen());
    assert(storage.openFileCount() == 0);
    assert(!package.open("/games/not-found.ali"));
    assert(!package.isValid());
    assert(!package.exists("Codes/main.js"));
    assert(storage.openFileCount() == 0);
}

void testOpenPathBoundaries()
{
    Storage storage;
    GamePackage package(storage);

    const std::string path127 = std::string(123, 'p') + ".ali";
    assert(path127.size() == 127);
    addValidPackage(storage, path127.c_str());
    assert(package.open(path127.c_str()));

    package.close();
    const std::string path128 = std::string(124, 'p') + ".ali";
    assert(path128.size() == 128);
    storage.addFile(path128, packageBytes());
    assert(!package.open(path128.c_str()));
    assert(!package.isValid());
}

void testPackageReadsOnlyIndexedMetadataAndPayload()
{
    Storage storage;
    const std::vector<uint8_t> bytes = packageBytes();
    storage.addFile(PACKAGE_PATH, bytes);
    GamePackage package(storage);
    assert(package.open(PACKAGE_PATH));
    const uint32_t indexSize =
        static_cast<uint32_t>(bytes[8]) |
        (static_cast<uint32_t>(bytes[9]) << 8) |
        (static_cast<uint32_t>(bytes[10]) << 16) |
        (static_cast<uint32_t>(bytes[11]) << 24);
    const std::string expectedManifest =
        "{\"id\":\"test-game\",\"name\":\"Test\",\"version\":\"1\","
        "\"packageVersion\":1,\"author\":\"Test\","
        "\"entrypoint\":\"Codes/main.js\",\"cover\":\"cover.bin\"}";
    assert(storage.bytesRead() == 2 * (ALI_HEADER_SIZE + indexSize) +
                                      expectedManifest.size());
    assert(storage.openFileCount() == 0);

    PackageFile file;
    assert(!package.openFile("missing.bin", file));
    assert(!file.isOpen());
    assert(!package.openFile("../secret", file));
    assert(!file.isOpen());
    assert(package.openFile("Assets/large.bin", file));
    assert(file.size() == 16 * 1024);
    assert(file.available() == 16 * 1024);

    uint8_t buffer[64];
    size_t index = 0;
    while (file.available() != 0) {
        const size_t count = file.read(buffer, sizeof(buffer));
        assert(count == sizeof(buffer));
        for (size_t i = 0; i < count; ++i) {
            assert(buffer[i] == static_cast<uint8_t>(index % 256));
            ++index;
        }
    }
    assert(index == 16 * 1024);
    assert(file.available() == 0);
    assert(file.read(buffer, sizeof(buffer)) == 0);
    file.close();
    assert(storage.openFileCount() == 0);
}

void testPackageFileReadAndMoves()
{
    Storage storage;
    addValidPackage(storage);
    GamePackage package(storage);
    assert(package.open(PACKAGE_PATH));

    PackageFile file;
    assert(package.openFile("Codes/main.js", file));
    assert(file.size() == std::strlen("main-content"));
    assert(file.available() == file.size());

    uint8_t buffer[32] = {};
    assert(file.read(nullptr, 1) == 0);
    assert(file.read(buffer, 0) == 0);
    assert(file.available() == file.size());
    assert(file.read(buffer, 5) == 5);
    assert(std::memcmp(buffer, "main-", 5) == 0);
    assert(file.available() == 7);

    PackageFile moved(std::move(file));
    assert(!file.isOpen());
    assert(file.size() == 0);
    assert(moved.isOpen());
    assert(moved.available() == 7);

    PackageFile assigned;
    assert(package.openFile("cover.bin", assigned));
    PackageFile source;
    assert(package.openFile("cover.bin", source));
    assert(source.read(buffer, 2) == 2);
    assert(std::memcmp(buffer, "co", 2) == 0);
    assigned = std::move(source);
    assert(!source.isOpen());
    assert(source.size() == 0);
    assert(assigned.isOpen());
    assert(assigned.available() == 3);
    assert(assigned.read(buffer, sizeof(buffer)) == 3);
    assert(std::memcmp(buffer, "ver", 3) == 0);
    assert(assigned.read(buffer, sizeof(buffer)) == 0);
    assigned.close();
    assert(!assigned.isOpen());
    assert(assigned.read(buffer, sizeof(buffer)) == 0);

    assert(moved.read(buffer, sizeof(buffer)) == 7);
    assert(std::memcmp(buffer, "content", 7) == 0);
    assert(moved.available() == 0);
    assert(moved.read(buffer, 1) == 0);
    moved.close();
    assert(storage.openFileCount() == 0);

    PackageFile oneByteReads;
    assert(package.openFile("cover.bin", oneByteReads));
    const char expected[] = "cover";
    for (size_t i = 0; i < sizeof(expected) - 1; ++i) {
        assert(oneByteReads.read(buffer, 1) == 1);
        assert(buffer[0] == static_cast<uint8_t>(expected[i]));
    }
    assert(oneByteReads.read(buffer, 1) == 0);
    oneByteReads.close();

    PackageFile overread;
    assert(package.openFile("cover.bin", overread));
    assert(overread.read(buffer, sizeof(buffer)) == sizeof(expected) - 1);
    assert(std::memcmp(buffer, expected, sizeof(expected) - 1) == 0);
    assert(overread.read(buffer, sizeof(buffer)) == 0);
    overread.close();

    {
        PackageFile scoped;
        assert(package.openFile("cover.bin", scoped));
        assert(storage.openFileCount() == 1);
    }
    assert(storage.openFileCount() == 0);
}

void testPackageFilePrematureEndCloses()
{
    Storage storage;
    addValidPackage(storage);
    GamePackage package(storage);
    assert(package.open(PACKAGE_PATH));

    PackageFile file;
    const size_t bytesBeforeOpen = storage.bytesRead();
    assert(package.openFile("Assets/large.bin", file));
    const size_t payloadOffset = storage.bytesRead() - bytesBeforeOpen;
    assert(file.size() == 16 * 1024);
    storage.setFailAt(PACKAGE_PATH, payloadOffset + 3);

    uint8_t bytes[8];
    assert(file.read(bytes, sizeof(bytes)) == 3);
    assert(file.available() == file.size() - 3);
    assert(file.read(bytes, sizeof(bytes)) == 0);
    assert(!file.isOpen());
    assert(file.available() == 0);
    assert(storage.openFileCount() == 0);
}

void testPackageFileSupportsShortReads()
{
    Storage storage;
    addValidPackage(storage);
    GamePackage package(storage);
    assert(package.open(PACKAGE_PATH));

    storage.setMaxReadChunk(2);
    PackageFile file;
    assert(package.openFile("Codes/main.js", file));
    const std::vector<uint8_t> contents = readAll(file);
    assert(std::string(contents.begin(), contents.end()) == "main-content");
    file.close();
    storage.setMaxReadChunk(static_cast<size_t>(-1));
    assert(storage.openFileCount() == 0);
}

void testStorageFailuresAndPackageReader()
{
    Storage storage;
    const std::vector<TestEntry> entries = validPackageEntries();
    const std::vector<uint8_t> bytes = buildPackage(entries);
    storage.addFile(PACKAGE_PATH, bytes);
    PackageReader reader(storage);
    PackageEntry decoded[ALI_MAX_ENTRIES] = {};
    uint8_t entryCount = 0;
    uint32_t packageSize = 0;

    assert(reader.readIndex(
        PACKAGE_PATH,
        decoded,
        ALI_MAX_ENTRIES,
        entryCount,
        packageSize));
    assert(entryCount == entries.size());
    assert(packageSize == bytes.size());
    assert(storage.openFileCount() == 0);

    storage.setMaxReadChunk(2);
    assert(reader.readIndex(
        PACKAGE_PATH,
        decoded,
        ALI_MAX_ENTRIES,
        entryCount,
        packageSize));
    storage.setMaxReadChunk(static_cast<size_t>(-1));

    PackageEntry* target = nullptr;
    for (uint8_t i = 0; i < entryCount; ++i) {
        if (std::strcmp(decoded[i].path, "Assets/large.bin") == 0) {
            target = &decoded[i];
        }
    }
    assert(target != nullptr);

    PackageFile file;
    storage.replaceFile(PACKAGE_PATH, std::vector<uint8_t>(bytes.size() + 1, 0));
    assert(!reader.openFile(PACKAGE_PATH, *target, packageSize, file));
    storage.replaceFile(PACKAGE_PATH, bytes);

    storage.setFailAt(PACKAGE_PATH, ALI_HEADER_SIZE + 2);
    GamePackage failed(storage);
    assert(!failed.open(PACKAGE_PATH));
    assert(!failed.isValid());
    assert(storage.openFileCount() == 0);
    storage.setFailAt(PACKAGE_PATH, static_cast<size_t>(-1));

    PackageEntry invalid = *target;
    invalid.offset = 0;
    assert(!reader.openFile(PACKAGE_PATH, invalid, packageSize, file));

    assert(!reader.readIndex(PACKAGE_PATH, decoded, 0, entryCount, packageSize));
    assert(!reader.readIndex(PACKAGE_PATH, decoded, 1, entryCount, packageSize));
    assert(!reader.readIndex(PACKAGE_PATH, nullptr, ALI_MAX_ENTRIES, entryCount, packageSize));

    storage.setMounted(false);
    assert(!reader.readIndex(
        PACKAGE_PATH,
        decoded,
        ALI_MAX_ENTRIES,
        entryCount,
        packageSize));
    assert(storage.openFileCount() == 0);
}

void testReaderSequentialSkipAndFailure()
{
    const std::string manifest =
        "{\"id\":\"test\",\"name\":\"Test\",\"version\":\"1\","
        "\"packageVersion\":1,\"author\":\"A\","
        "\"entrypoint\":\"main.js\",\"cover\":\"cover.bin\"}";
    std::vector<uint8_t> padding(140, 0x5A);
    const std::vector<uint8_t> manifestBytes = byteVector(manifest);
    const std::vector<uint8_t> codeBytes = byteVector("main-body");
    const std::vector<uint8_t> coverBytes = byteVector("cover");
    std::vector<TestEntry> entries;
    entries.push_back(TestEntry("manifest.json", manifestBytes));
    entries.push_back(TestEntry("padding.bin", padding));
    entries.push_back(TestEntry("main.js", codeBytes));
    entries.push_back(TestEntry("cover.bin", coverBytes));

    const uint32_t indexSize = static_cast<uint32_t>(
        1 + std::strlen("manifest.json") + 8 +
        1 + std::strlen("padding.bin") + 8 +
        1 + std::strlen("main.js") + 8 +
        1 + std::strlen("cover.bin") + 8);
    entries[2].overrideOffset = true;
    entries[2].offset = ALI_HEADER_SIZE + indexSize +
                        static_cast<uint32_t>(manifestBytes.size() + padding.size());

    const std::vector<uint8_t> bytes = buildPackage(entries);
    Storage storage;
    storage.addFile(PACKAGE_PATH, bytes);
    GamePackage package(storage);
    assert(package.open(PACKAGE_PATH));

    PackageFile mainFile;
    assert(package.openFile("main.js", mainFile));
    uint8_t output[16] = {};
    assert(mainFile.read(output, sizeof(output)) == std::strlen("main-body"));
    assert(std::memcmp(output, "main-body", std::strlen("main-body")) == 0);
    mainFile.close();

    storage.setFailAt(PACKAGE_PATH, entries[2].offset - 10);
    assert(!package.openFile("main.js", mainFile));
    assert(!mainFile.isOpen());
    assert(storage.openFileCount() == 0);
}

void testMalformedContainersAreRejected()
{
    std::vector<TestEntry> entries = validPackageEntries();
    const std::vector<uint8_t> valid = buildPackage(entries);

    Storage storage;
    GamePackage package(storage);
    storage.addFile(PACKAGE_PATH, std::vector<uint8_t>(ALI_HEADER_SIZE - 1, 0));
    assert(!package.open(PACKAGE_PATH));

    storage.addFile(PACKAGE_PATH, buildPackage(entries, 0));
    assert(!package.open(PACKAGE_PATH));
    storage.addFile(PACKAGE_PATH, buildPackage(entries, 2));
    assert(!package.open(PACKAGE_PATH));
    storage.addFile(PACKAGE_PATH, buildPackage(entries, 1, true, 0));
    assert(!package.open(PACKAGE_PATH));
    storage.addFile(PACKAGE_PATH, buildPackage(entries, 1, true, 65));
    assert(!package.open(PACKAGE_PATH));
    storage.addFile(PACKAGE_PATH, buildPackage(entries, 1, false, 0, true, 1));
    assert(!package.open(PACKAGE_PATH));
    storage.addFile(PACKAGE_PATH, buildPackage(entries, 1, false, 0, true, 500));
    assert(!package.open(PACKAGE_PATH));
    storage.addFile(PACKAGE_PATH, buildPackage(entries, 1, false, 0, true, 9000));
    assert(!package.open(PACKAGE_PATH));
    storage.addFile(
        PACKAGE_PATH,
        buildPackage(entries, 1, true, static_cast<uint16_t>(entries.size() - 1)));
    assert(!package.open(PACKAGE_PATH));
    storage.addFile(
        PACKAGE_PATH,
        buildPackage(entries, 1, true, static_cast<uint16_t>(entries.size() + 1)));
    assert(!package.open(PACKAGE_PATH));
    storage.addFile(PACKAGE_PATH, buildPackage(entries, 1, false, 0, false, 0, true));
    assert(!package.open(PACKAGE_PATH));

    std::vector<uint8_t> truncated = valid;
    truncated.resize(ALI_HEADER_SIZE + 3);
    storage.addFile(PACKAGE_PATH, truncated);
    assert(!package.open(PACKAGE_PATH));

    entries = validPackageEntries();
    entries[1].overrideSize = true;
    entries[1].size = static_cast<uint32_t>(entries[1].data.size() + 50);
    storage.addFile(PACKAGE_PATH, buildPackage(entries));
    assert(!package.open(PACKAGE_PATH));

    entries = validPackageEntries();
    const uint32_t indexSize = static_cast<uint32_t>(
        1 + std::strlen("manifest.json") + 8 +
        1 + std::strlen("Codes/main.js") + 8 +
        1 + std::strlen("cover.bin") + 8 +
        1 + std::strlen("Assets/large.bin") + 8 +
        1 + std::strlen("empty.bin") + 8);
    entries[2].overrideOffset = true;
    entries[2].offset = ALI_HEADER_SIZE + indexSize + entries[1].data.size() - 1;
    storage.addFile(PACKAGE_PATH, buildPackage(entries));
    assert(!package.open(PACKAGE_PATH));
    assert(storage.openFileCount() == 0);
}

void testEntryCountBoundariesAndDuplicatePaths()
{
    std::vector<TestEntry> entries;
    for (uint8_t i = 0; i < ALI_MAX_ENTRIES; ++i) {
        entries.push_back(TestEntry(
            std::string("f") + static_cast<char>('a' + (i % 26)) +
                static_cast<char>('A' + (i / 26)) + ".bin",
            std::vector<uint8_t>()));
    }
    const std::vector<uint8_t> sixtyFour = buildPackage(entries);
    Storage storage;
    storage.addFile(PACKAGE_PATH, sixtyFour);
    PackageReader reader(storage);
    PackageEntry decoded[ALI_MAX_ENTRIES] = {};
    uint8_t count = 0;
    uint32_t size = 0;
    assert(reader.readIndex(PACKAGE_PATH, decoded, ALI_MAX_ENTRIES, count, size));
    assert(count == ALI_MAX_ENTRIES);

    entries.push_back(TestEntry("extra.bin", std::vector<uint8_t>()));
    storage.addFile(PACKAGE_PATH, buildPackage(entries));
    GamePackage package(storage);
    assert(!package.open(PACKAGE_PATH));

    std::vector<TestEntry> duplicate;
    duplicate.push_back(TestEntry("same.bin", std::vector<uint8_t>()));
    duplicate.push_back(TestEntry("same.bin", std::vector<uint8_t>()));
    storage.addFile(PACKAGE_PATH, buildPackage(duplicate));
    assert(!reader.readIndex(PACKAGE_PATH, decoded, ALI_MAX_ENTRIES, count, size));
}

} // namespace

int main()
{
    testGamePackageLifecycle();
    testGamePackageFilesAndClose();
    testOpenPathBoundaries();
    testPackageReadsOnlyIndexedMetadataAndPayload();
    testPackageFileReadAndMoves();
    testPackageFilePrematureEndCloses();
    testPackageFileSupportsShortReads();
    testStorageFailuresAndPackageReader();
    testReaderSequentialSkipAndFailure();
    testMalformedContainersAreRejected();
    testEntryCountBoundariesAndDuplicatePaths();
    assert(true);
    std::cout << "Game Package reader tests passed\n";
}
