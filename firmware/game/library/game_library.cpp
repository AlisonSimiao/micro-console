#include "game/library/game_library.hpp"

#include <cstring>

namespace {

static_assert(
    sizeof(GameInfo) <= 480,
    "GameInfo grew beyond its documented memory budget");

static_assert(
    sizeof(GameLibrary) <= GameLibrary::MAX_INSTANCE_BYTES,
    "GameLibrary grew beyond its documented memory budget");

bool hasAliExtension(const char* name)
{
    const size_t length = std::strlen(name);
    return length > 4 && std::strcmp(name + length - 4, ".ali") == 0;
}

bool buildPackagePath(const char* name, char (&path)[128])
{
    const size_t directoryLength = std::strlen(GameLibrary::DIRECTORY);
    const size_t nameLength = std::strlen(name);
    if (directoryLength + 1 + nameLength >= sizeof(path)) {
        return false;
    }

    std::memcpy(path, GameLibrary::DIRECTORY, directoryLength);
    path[directoryLength] = '/';
    std::memcpy(path + directoryLength + 1, name, nameLength + 1);
    return true;
}

void copyText(char* destination, size_t capacity, const char* source)
{
    size_t length = std::strlen(source);
    if (length >= capacity) {
        length = capacity - 1;
    }
    std::memcpy(destination, source, length);
    destination[length] = '\0';
}

} // namespace

constexpr const char* GameLibrary::DIRECTORY;

GameLibrary::GameLibrary(Storage& storage)
    : storage_(storage),
      package_(storage),
      games_{},
      count_(0),
      truncated_(false)
{
}

bool GameLibrary::scan()
{
    count_ = 0;
    truncated_ = false;
    package_.close();

    if (!storage_.isMounted()) {
        return false;
    }

    StorageDirectory directory;
    if (!storage_.openDirectory(DIRECTORY, directory)) {
        return false;
    }

    StorageEntry entry;
    while (directory.next(entry)) {
        if (entry.isDirectory || !hasAliExtension(entry.name)) {
            continue;
        }

        if (count_ == MAX_GAMES) {
            truncated_ = true;
            break;
        }

        char packagePath[128];
        if (!buildPackagePath(entry.name, packagePath)) {
            continue;
        }

        if (!package_.open(packagePath)) {
            continue;
        }

        const GameManifest& manifest = package_.manifest();
        GameInfo& info = games_[count_];
        copyText(info.id, sizeof(info.id), manifest.id);
        copyText(info.name, sizeof(info.name), manifest.name);
        copyText(info.version, sizeof(info.version), manifest.version);
        copyText(info.author, sizeof(info.author), manifest.author);
        copyText(info.packagePath, sizeof(info.packagePath), packagePath);
        copyText(info.coverPath, sizeof(info.coverPath), manifest.cover);
        ++count_;
    }

    directory.close();
    package_.close();
    return true;
}

bool GameLibrary::refresh()
{
    return scan();
}

size_t GameLibrary::count() const
{
    return count_;
}

const GameInfo* GameLibrary::get(size_t index) const
{
    return index < count_ ? &games_[index] : nullptr;
}

bool GameLibrary::isTruncated() const
{
    return truncated_;
}
