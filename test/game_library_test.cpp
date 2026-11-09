#include <cassert>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>

#include "game/library/game_library.hpp"
#include "test/support/build_package.hpp"

namespace {

const char* const GAMES_DIRECTORY = "/games";

std::vector<uint8_t> makeGame(
    const std::string& id,
    const std::string& name,
    const std::string& version = "1.0.0",
    const std::string& author = "Test Author")
{
    const std::string manifest =
        "{\"id\":\"" + id + "\",\"name\":\"" + name + "\",\"version\":\"" + version +
        "\",\"packageVersion\":1,\"author\":\"" + author +
        "\",\"entrypoint\":\"main.js\",\"cover\":\"cover.bin\"}";

    std::vector<TestEntry> entries;
    entries.push_back(TestEntry("manifest.json", byteVector(manifest)));
    entries.push_back(TestEntry("main.js", byteVector("console.log('game');")));
    entries.push_back(TestEntry("cover.bin", byteVector("cover")));
    return buildPackage(entries);
}

std::vector<uint8_t> makeInvalidManifestGame()
{
    const std::string manifest =
        "{\"id\":\"broken\",\"name\":\"Broken\",\"version\":\"1\","
        "\"packageVersion\":2,\"author\":\"Test\","
        "\"entrypoint\":\"main.js\",\"cover\":\"cover.bin\"}";

    std::vector<TestEntry> entries;
    entries.push_back(TestEntry("manifest.json", byteVector(manifest)));
    entries.push_back(TestEntry("main.js", byteVector("code")));
    entries.push_back(TestEntry("cover.bin", byteVector("cover")));
    return buildPackage(entries);
}

const GameInfo* find(const GameLibrary& library, const std::string& id)
{
    for (size_t i = 0; i < library.count(); ++i) {
        const GameInfo* info = library.get(i);
        if (info != nullptr && std::string(info->id) == id) {
            return info;
        }
    }
    return nullptr;
}

void testStorageNotMounted()
{
    Storage storage;
    storage.setMounted(false);
    storage.addDirectory(GAMES_DIRECTORY);

    GameLibrary library(storage);
    assert(!library.scan());
    assert(library.count() == 0);
    assert(!library.isTruncated());
    assert(storage.openFileCount() == 0);
}

void testMissingDirectory()
{
    Storage storage;
    storage.addFile("/games/game.ali", makeGame("game", "Game"));

    GameLibrary library(storage);
    assert(!library.scan());
    assert(library.count() == 0);
    assert(storage.openFileCount() == 0);
}

void testEmptyDirectory()
{
    Storage storage;
    storage.addDirectory(GAMES_DIRECTORY);

    GameLibrary library(storage);
    assert(library.scan());
    assert(library.count() == 0);
    assert(!library.isTruncated());
    assert(storage.openFileCount() == 0);
}

void testDiscoveryAndFilter()
{
    Storage storage;
    storage.addDirectory(GAMES_DIRECTORY);
    storage.addFile("/games/space.ali", makeGame("space", "Space Shooter", "2.0.1", "Ada"));
    storage.addFile("/games/puzzle.ali", makeGame("puzzle", "Puzzle"));
    storage.addFile("/games/GAMMA.ALI", makeGame("gamma", "Gamma"));
    storage.addFile("/games/readme.txt", byteVector("not a game"));
    storage.addFile("/games/game.zip", makeGame("zipped", "Zipped"));
    storage.addDirectory("/games/nested");
    storage.addFile("/games/nested/deep.ali", makeGame("deep", "Deep"));

    GameLibrary library(storage);
    assert(library.scan());
    assert(library.count() == 2);
    assert(!library.isTruncated());
    assert(storage.openFileCount() == 0);

    const GameInfo* space = find(library, "space");
    assert(space != nullptr);
    assert(std::string(space->name) == "Space Shooter");
    assert(std::string(space->version) == "2.0.1");
    assert(std::string(space->author) == "Ada");
    assert(std::string(space->packagePath) == "/games/space.ali");
    assert(std::string(space->coverPath) == "cover.bin");

    assert(find(library, "puzzle") != nullptr);
    assert(find(library, "gamma") == nullptr);
    assert(find(library, "zipped") == nullptr);
    assert(find(library, "deep") == nullptr);
}

void testInvalidPackagesAreIgnored()
{
    Storage storage;
    storage.addDirectory(GAMES_DIRECTORY);
    storage.addFile("/games/valid.ali", makeGame("valid", "Valid"));
    storage.addFile("/games/corrupt.ali", byteVector("not a package"));
    storage.addFile("/games/bad-manifest.ali", makeInvalidManifestGame());

    std::vector<TestEntry> missingEntrypoint;
    const std::string manifest =
        "{\"id\":\"missing\",\"name\":\"Missing\",\"version\":\"1\","
        "\"packageVersion\":1,\"author\":\"Test\","
        "\"entrypoint\":\"absent.js\",\"cover\":\"cover.bin\"}";
    missingEntrypoint.push_back(TestEntry("manifest.json", byteVector(manifest)));
    missingEntrypoint.push_back(TestEntry("main.js", byteVector("code")));
    missingEntrypoint.push_back(TestEntry("cover.bin", byteVector("cover")));
    storage.addFile("/games/missing.ali", buildPackage(missingEntrypoint));

    GameLibrary library(storage);
    assert(library.scan());
    assert(library.count() == 1);
    assert(find(library, "valid") != nullptr);
    assert(storage.openFileCount() == 0);
}

void testLimit()
{
    Storage storage;
    storage.addDirectory(GAMES_DIRECTORY);
    for (size_t i = 0; i < GameLibrary::MAX_GAMES + 2; ++i) {
        const std::string id = "game" + std::to_string(i);
        storage.addFile(
            "/games/" + id + ".ali",
            makeGame(id, "Game " + std::to_string(i)));
    }

    GameLibrary library(storage);
    assert(library.scan());
    assert(library.count() == GameLibrary::MAX_GAMES);
    assert(library.isTruncated());
    assert(library.get(GameLibrary::MAX_GAMES) == nullptr);
    assert(storage.openFileCount() == 0);
}

void testRefreshClearsPreviousResults()
{
    Storage storage;
    storage.addDirectory(GAMES_DIRECTORY);
    storage.addFile("/games/one.ali", makeGame("one", "One"));
    storage.addFile("/games/two.ali", makeGame("two", "Two"));

    GameLibrary library(storage);
    assert(library.scan());
    assert(library.count() == 2);

    storage.clearFiles();
    assert(library.refresh());
    assert(library.count() == 0);
    assert(library.get(0) == nullptr);
    assert(storage.openFileCount() == 0);
}

void testDirectoryNameIsCaseSensitive()
{
    Storage storage;
    storage.addDirectory("/Games");
    storage.addFile("/Games/game.ali", makeGame("game", "Game"));

    GameLibrary library(storage);
    assert(!library.scan());
    assert(library.count() == 0);
    assert(storage.openFileCount() == 0);
}

} // namespace

int main()
{
    testStorageNotMounted();
    testMissingDirectory();
    testEmptyDirectory();
    testDiscoveryAndFilter();
    testInvalidPackagesAreIgnored();
    testLimit();
    testRefreshClearsPreviousResults();
    testDirectoryNameIsCaseSensitive();
    std::cout << "Game Library tests passed\n";
    return 0;
}
