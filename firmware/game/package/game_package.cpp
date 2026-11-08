#include "game/package/game_package.hpp"

#include <cstring>

namespace {

bool isAliPath(const char* path)
{
    if (path == nullptr) {
        return false;
    }

    size_t length = 0;
    while (length < 128 && path[length] != '\0') {
        ++length;
    }

    return length > 4 && length < 128 &&
           std::strcmp(path + length - 4, ".ali") == 0;
}

} // namespace

GamePackage::GamePackage(Storage& storage)
    : reader_(storage),
      packagePath_{},
      entries_{},
      entryCount_(0),
      packageSize_(0),
      manifest_{},
      valid_(false)
{
}

bool GamePackage::open(const char* path)
{
    close();
    if (!isAliPath(path)) {
        return false;
    }

    size_t pathLength = 0;
    while (pathLength < sizeof(packagePath_) && path[pathLength] != '\0') {
        ++pathLength;
    }
    if (pathLength >= sizeof(packagePath_)) {
        return false;
    }
    std::memcpy(packagePath_, path, pathLength + 1);

    if (!reader_.readIndex(
            packagePath_,
            entries_,
            PackageReader::MAX_ENTRIES,
            entryCount_,
            packageSize_)) {
        close();
        return false;
    }

    const PackageEntry* manifestEntry = findEntry("manifest.json");
    if (manifestEntry == nullptr || manifestEntry->size == 0 ||
        manifestEntry->size > MAX_MANIFEST_SIZE) {
        close();
        return false;
    }

    PackageFile manifestFile;
    if (!reader_.openFile(packagePath_, *manifestEntry, packageSize_, manifestFile)) {
        close();
        return false;
    }

    char json[MAX_MANIFEST_SIZE + 1];
    size_t bytesRead = 0;
    while (bytesRead < manifestEntry->size) {
        const size_t count = manifestFile.read(
            reinterpret_cast<uint8_t*>(json + bytesRead),
            manifestEntry->size - bytesRead);
        if (count == 0) {
            close();
            return false;
        }
        bytesRead += count;
    }
    json[bytesRead] = '\0';

    if (!parseGameManifest(json, bytesRead, manifest_) ||
        findEntry(manifest_.entrypoint) == nullptr ||
        findEntry(manifest_.cover) == nullptr) {
        close();
        return false;
    }

    valid_ = true;
    return true;
}

void GamePackage::close()
{
    packagePath_[0] = '\0';
    std::memset(entries_, 0, sizeof(entries_));
    entryCount_ = 0;
    packageSize_ = 0;
    manifest_ = {};
    valid_ = false;
}

bool GamePackage::isValid() const
{
    return valid_;
}

const GameManifest& GamePackage::manifest() const
{
    return manifest_;
}

bool GamePackage::exists(const char* path) const
{
    if (!valid_ || path == nullptr) {
        return false;
    }

    size_t length = 0;
    while (length <= PackageReader::MAX_PATH_LENGTH && path[length] != '\0') {
        ++length;
    }
    if (length == 0 || length > PackageReader::MAX_PATH_LENGTH ||
        !PackageReader::isValidPath(path, length)) {
        return false;
    }

    return findEntry(path) != nullptr;
}

bool GamePackage::openFile(const char* path, PackageFile& file) const
{
    file.close();
    if (!valid_ || path == nullptr) {
        return false;
    }

    size_t length = 0;
    while (length <= PackageReader::MAX_PATH_LENGTH && path[length] != '\0') {
        ++length;
    }
    if (length == 0 || length > PackageReader::MAX_PATH_LENGTH ||
        !PackageReader::isValidPath(path, length)) {
        return false;
    }

    const PackageEntry* entry = findEntry(path);
    return entry != nullptr &&
           reader_.openFile(packagePath_, *entry, packageSize_, file);
}

const PackageEntry* GamePackage::findEntry(const char* path) const
{
    if (path == nullptr) {
        return nullptr;
    }

    for (uint8_t i = 0; i < entryCount_; ++i) {
        if (std::strcmp(entries_[i].path, path) == 0) {
            return &entries_[i];
        }
    }

    return nullptr;
}
