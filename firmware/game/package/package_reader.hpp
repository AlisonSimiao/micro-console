#pragma once

#include <cstddef>
#include <cstdint>

#include "game/package/package_format.hpp"
#include "hal/storage/storage.hpp"

class PackageFile {
public:
    PackageFile();
    ~PackageFile();

    PackageFile(const PackageFile&) = delete;
    PackageFile& operator=(const PackageFile&) = delete;

    PackageFile(PackageFile&& other) noexcept;
    PackageFile& operator=(PackageFile&& other) noexcept;

    size_t read(uint8_t* buffer, size_t length);
    size_t available() const;
    size_t size() const;
    bool isOpen() const;
    void close();

private:
    friend class PackageReader;

    void attach(StorageFile&& file, uint32_t size);

    StorageFile file_;
    uint32_t size_;
    uint32_t remaining_;
};

class PackageReader {
public:
    static constexpr uint8_t MAX_ENTRIES = ALI_MAX_ENTRIES;
    static constexpr size_t MAX_PATH_LENGTH = ALI_MAX_PATH_LENGTH;
    static constexpr size_t HEADER_SIZE = ALI_HEADER_SIZE;
    static constexpr size_t MAX_INDEX_SIZE = ALI_MAX_INDEX_SIZE;

    explicit PackageReader(Storage& storage);

    bool readIndex(
        const char* packagePath,
        PackageEntry* entries,
        size_t capacity,
        uint8_t& entryCount,
        uint32_t& packageSize) const;

    bool openFile(
        const char* packagePath,
        const PackageEntry& entry,
        uint32_t packageSize,
        PackageFile& file) const;

    static bool isValidPath(const char* path, size_t length);

private:
    Storage& storage_;
};
