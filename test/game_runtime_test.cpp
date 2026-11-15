#include <cassert>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>

#include "game/runtime/game_runtime.hpp"
#include "test/support/build_package.hpp"

namespace {

const char* const GAME_PATH = "/games/space.ali";
const char* const OTHER_PATH = "/games/puzzle.ali";

std::vector<uint8_t> makeGame(const std::string& id)
{
    const std::string manifest =
        "{\"id\":\"" + id + "\",\"name\":\"Game " + id + "\",\"version\":\"1.0.0\","
        "\"packageVersion\":1,\"author\":\"Test\","
        "\"entrypoint\":\"main.js\",\"cover\":\"cover.bin\"}";

    std::vector<TestEntry> entries;
    entries.push_back(TestEntry("manifest.json", byteVector(manifest)));
    entries.push_back(TestEntry("main.js", byteVector("console.log('game');")));
    entries.push_back(TestEntry("cover.bin", byteVector("cover")));
    return buildPackage(entries);
}

std::vector<uint8_t> makeGameWithManifest(const std::string& manifest,
                                          const std::string& entrypointFile)
{
    std::vector<TestEntry> entries;
    entries.push_back(TestEntry("manifest.json", byteVector(manifest)));
    entries.push_back(TestEntry(entrypointFile, byteVector("code")));
    entries.push_back(TestEntry("cover.bin", byteVector("cover")));
    return buildPackage(entries);
}

GameInfo gameInfo(const char* path)
{
    GameInfo info = {};
    if (path != nullptr) {
        std::strncpy(info.packagePath, path, sizeof(info.packagePath) - 1);
    }
    return info;
}

void testInitialState()
{
    Storage storage;
    GameRuntime runtime(storage);

    assert(runtime.state() == GameRuntime::State::Idle);
    assert(!runtime.isRunning());
    assert(runtime.failure() == GameRuntime::Failure::None);
    assert(runtime.frameCount() == 0);
}

void testEmptyPath()
{
    Storage storage;
    storage.addFile(GAME_PATH, makeGame("space"));

    GameRuntime runtime(storage);
    const GameInfo info = {};

    assert(!runtime.launch(info));
    assert(runtime.state() == GameRuntime::State::Failed);
    assert(runtime.failure() == GameRuntime::Failure::InvalidArgument);
    assert(!runtime.isRunning());
    assert(storage.openFileCount() == 0);
}

void testStorageUnavailable()
{
    Storage storage;
    storage.setMounted(false);

    GameRuntime runtime(storage);

    assert(!runtime.launch(gameInfo(GAME_PATH)));
    assert(runtime.state() == GameRuntime::State::Failed);
    assert(runtime.failure() == GameRuntime::Failure::StorageUnavailable);
    assert(storage.openFileCount() == 0);
}

void testMissingPackage()
{
    Storage storage;
    GameRuntime runtime(storage);

    assert(!runtime.launch(gameInfo(GAME_PATH)));
    assert(runtime.state() == GameRuntime::State::Failed);
    assert(runtime.failure() == GameRuntime::Failure::PackageOpenFailed);
    assert(storage.openFileCount() == 0);
}

void testCorruptPackage()
{
    Storage storage;
    storage.addFile(GAME_PATH, byteVector("not a package"));

    GameRuntime runtime(storage);

    assert(!runtime.launch(gameInfo(GAME_PATH)));
    assert(runtime.state() == GameRuntime::State::Failed);
    assert(runtime.failure() == GameRuntime::Failure::PackageOpenFailed);
    assert(storage.openFileCount() == 0);
}

void testInvalidManifest()
{
    Storage storage;
    storage.addFile(
        GAME_PATH,
        makeGameWithManifest(
            "{\"id\":\"broken\",\"name\":\"Broken\",\"version\":\"1\","
            "\"packageVersion\":2,\"author\":\"Test\","
            "\"entrypoint\":\"main.js\",\"cover\":\"cover.bin\"}",
            "main.js"));

    GameRuntime runtime(storage);

    assert(!runtime.launch(gameInfo(GAME_PATH)));
    assert(runtime.state() == GameRuntime::State::Failed);
    assert(runtime.failure() == GameRuntime::Failure::PackageOpenFailed);
    assert(storage.openFileCount() == 0);
}

void testMissingEntrypoint()
{
    Storage storage;
    storage.addFile(
        GAME_PATH,
        makeGameWithManifest(
            "{\"id\":\"missing\",\"name\":\"Missing\",\"version\":\"1\","
            "\"packageVersion\":1,\"author\":\"Test\","
            "\"entrypoint\":\"absent.js\",\"cover\":\"cover.bin\"}",
            "main.js"));

    GameRuntime runtime(storage);

    assert(!runtime.launch(gameInfo(GAME_PATH)));
    assert(runtime.state() == GameRuntime::State::Failed);
    assert(runtime.failure() == GameRuntime::Failure::PackageOpenFailed);
    assert(storage.openFileCount() == 0);
}

void testInvalidExtension()
{
    Storage storage;
    storage.addFile("/games/game.zip", makeGame("zipped"));

    GameRuntime runtime(storage);

    assert(!runtime.launch(gameInfo("/games/game.zip")));
    assert(runtime.state() == GameRuntime::State::Failed);
    assert(runtime.failure() == GameRuntime::Failure::PackageOpenFailed);
}

void testSuccessfulLaunch()
{
    Storage storage;
    storage.addFile(GAME_PATH, makeGame("space"));

    GameRuntime runtime(storage);

    assert(runtime.launch(gameInfo(GAME_PATH)));
    assert(runtime.state() == GameRuntime::State::Running);
    assert(runtime.isRunning());
    assert(runtime.failure() == GameRuntime::Failure::None);
    assert(runtime.frameCount() == 0);
    assert(storage.openFileCount() == 0);
}

void testUpdateOnlyAdvancesRunning()
{
    Storage storage;
    storage.addFile(GAME_PATH, makeGame("space"));

    GameRuntime runtime(storage);

    runtime.update();
    runtime.update();
    assert(runtime.frameCount() == 0);

    assert(runtime.launch(gameInfo(GAME_PATH)));
    runtime.update();
    runtime.update();
    runtime.update();
    assert(runtime.frameCount() == 3);

    runtime.stop();
    runtime.update();
    assert(runtime.frameCount() == 0);
}

void testLaunchWhileRunningIsRejected()
{
    Storage storage;
    storage.addFile(GAME_PATH, makeGame("space"));
    storage.addFile(OTHER_PATH, makeGame("puzzle"));

    GameRuntime runtime(storage);

    assert(runtime.launch(gameInfo(GAME_PATH)));
    runtime.update();
    runtime.update();

    assert(!runtime.launch(gameInfo(OTHER_PATH)));
    assert(runtime.state() == GameRuntime::State::Running);
    assert(runtime.isRunning());
    assert(runtime.failure() == GameRuntime::Failure::None);
    assert(runtime.frameCount() == 2);
    assert(storage.openFileCount() == 0);

    runtime.stop();
    assert(runtime.launch(gameInfo(OTHER_PATH)));
    assert(runtime.isRunning());
}

void testStopIsIdempotent()
{
    Storage storage;
    storage.addFile(GAME_PATH, makeGame("space"));

    GameRuntime runtime(storage);

    runtime.stop();
    assert(runtime.state() == GameRuntime::State::Idle);

    assert(runtime.launch(gameInfo(GAME_PATH)));
    runtime.update();

    runtime.stop();
    assert(runtime.state() == GameRuntime::State::Idle);
    assert(runtime.failure() == GameRuntime::Failure::None);
    assert(runtime.frameCount() == 0);
    assert(storage.openFileCount() == 0);

    runtime.stop();
    assert(runtime.state() == GameRuntime::State::Idle);
}

void testLaunchAfterFailure()
{
    Storage storage;
    GameRuntime runtime(storage);

    assert(!runtime.launch(gameInfo(GAME_PATH)));
    assert(runtime.state() == GameRuntime::State::Failed);

    storage.addFile(GAME_PATH, makeGame("space"));

    assert(runtime.launch(gameInfo(GAME_PATH)));
    assert(runtime.state() == GameRuntime::State::Running);
    assert(runtime.failure() == GameRuntime::Failure::None);
    assert(runtime.frameCount() == 0);
}

void testStopFromFailedClearsDiagnosis()
{
    Storage storage;
    GameRuntime runtime(storage);

    assert(!runtime.launch(gameInfo(GAME_PATH)));
    assert(runtime.state() == GameRuntime::State::Failed);

    runtime.stop();
    assert(runtime.state() == GameRuntime::State::Idle);
    assert(runtime.failure() == GameRuntime::Failure::None);
}

void testSequentialGames()
{
    Storage storage;
    storage.addFile(GAME_PATH, makeGame("space"));
    storage.addFile(OTHER_PATH, makeGame("puzzle"));

    GameRuntime runtime(storage);

    assert(runtime.launch(gameInfo(GAME_PATH)));
    runtime.update();
    runtime.update();
    runtime.update();
    runtime.update();
    assert(runtime.frameCount() == 4);
    runtime.stop();

    assert(runtime.launch(gameInfo(OTHER_PATH)));
    assert(runtime.state() == GameRuntime::State::Running);
    assert(runtime.frameCount() == 0);
    assert(storage.openFileCount() == 0);
}

void testFailedAttemptResetsFrames()
{
    Storage storage;
    storage.addFile(GAME_PATH, makeGame("space"));

    GameRuntime runtime(storage);

    assert(runtime.launch(gameInfo(GAME_PATH)));
    runtime.update();
    runtime.update();
    assert(runtime.frameCount() == 2);
    runtime.stop();

    assert(!runtime.launch(gameInfo(OTHER_PATH)));
    assert(runtime.state() == GameRuntime::State::Failed);
    assert(runtime.frameCount() == 0);
}

} // namespace

int main()
{
    testInitialState();
    testEmptyPath();
    testStorageUnavailable();
    testMissingPackage();
    testCorruptPackage();
    testInvalidManifest();
    testMissingEntrypoint();
    testInvalidExtension();
    testSuccessfulLaunch();
    testUpdateOnlyAdvancesRunning();
    testLaunchWhileRunningIsRejected();
    testStopIsIdempotent();
    testLaunchAfterFailure();
    testStopFromFailedClearsDiagnosis();
    testSequentialGames();
    testFailedAttemptResetsFrames();
    std::cout << "Game Runtime tests passed\n";
    return 0;
}
