#include <cassert>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <vector>

#include "game/package/game_package.hpp"
#include "test/support/build_package.hpp"

namespace {

const char* const PACKAGE_PATH = "/games/fixture.ali";

std::vector<uint8_t> readFixture(const std::string& directory, const char* name)
{
    const std::string path = directory + "/" + name;
    std::ifstream input(path.c_str(), std::ios::binary);
    assert(input.is_open());
    return std::vector<uint8_t>(
        std::istreambuf_iterator<char>(input),
        std::istreambuf_iterator<char>());
}

std::vector<uint8_t> readAll(PackageFile& file, size_t chunkSize)
{
    std::vector<uint8_t> bytes;
    std::vector<uint8_t> buffer(chunkSize);
    while (file.available() > 0) {
        const size_t count = file.read(buffer.data(), buffer.size());
        assert(count > 0);
        bytes.insert(bytes.end(), buffer.begin(), buffer.begin() + count);
    }
    return bytes;
}

void testValidFixture(const std::string& directory)
{
    Storage storage;
    storage.addFile(PACKAGE_PATH, readFixture(directory, "valid.ali"));
    GamePackage package(storage);

    assert(package.open(PACKAGE_PATH));
    assert(package.isValid());
    assert(storage.openFileCount() == 0);
    assert(std::string(package.manifest().id) == "fixture-game");
    assert(std::string(package.manifest().name) == "Fixture Game");
    assert(std::string(package.manifest().entrypoint) == "Codes/main.js");
    assert(std::string(package.manifest().cover) == "Capa.bin");
    assert(package.exists("Codes/main.js"));
    assert(package.exists("Capa.bin"));
    assert(package.exists("Assets/large.bin"));

    PackageFile entrypoint;
    assert(package.openFile(package.manifest().entrypoint, entrypoint));
    const std::vector<uint8_t> code = readAll(entrypoint, 64);
    assert(code.size() == 2048 + std::strlen("// fixture entrypoint\n"));
    assert(std::string(code.begin(), code.begin() + std::strlen("// fixture entrypoint\n")) ==
           "// fixture entrypoint\n");
    entrypoint.close();

    PackageFile cover;
    assert(package.openFile("Capa.bin", cover));
    const std::vector<uint8_t> coverBytes = readAll(cover, 16);
    const std::vector<uint8_t> expectedCover = {
        0x89, 'P', 'N', 'G', '\r', '\n', 0x1a, '\n',
        'f', 'i', 'x', 't', 'u', 'r', 'e'};
    assert(coverBytes == expectedCover);
    cover.close();

    PackageFile large;
    assert(package.openFile("Assets/large.bin", large));
    assert(large.size() == 16 * 1024);
    const std::vector<uint8_t> largeBytes = readAll(large, 64);
    assert(largeBytes.size() == 16 * 1024);
    for (size_t i = 0; i < largeBytes.size(); ++i) {
        assert(largeBytes[i] == static_cast<uint8_t>(i % 256));
    }
    large.close();
    assert(storage.openFileCount() == 0);
}

void testInvalidFixtures(const std::string& directory)
{
    const char* const fixtures[] = {
        "missing-manifest.ali",
        "invalid-manifest.ali",
        "unsupported-version.ali",
        "missing-entrypoint.ali",
        "missing-cover.ali",
        "invalid-path.ali",
        "inconsistent-size.ali",
        "overlapping-entries.ali",
        "truncated-index.ali",
        "truncated-payload.ali",
        "oversized-manifest.ali",
        "too-many-entries.ali",
        "bad-magic.ali"};

    Storage storage;
    GamePackage package(storage);
    for (size_t i = 0; i < sizeof(fixtures) / sizeof(fixtures[0]); ++i) {
        storage.addFile(PACKAGE_PATH, readFixture(directory, fixtures[i]));
        assert(!package.open(PACKAGE_PATH));
        assert(!package.isValid());
        assert(storage.openFileCount() == 0);
    }
}

} // namespace

int main(int argc, char** argv)
{
    assert(argc == 2);
    const std::string directory = argv[1];
    testValidFixture(directory);
    testInvalidFixtures(directory);
    std::cout << "Game Package fixture tests passed\n";
}
